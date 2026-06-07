#include <string.h>
#include <ap_int.h>

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const short CENTER = 0;
const short NORTH = 1;
const short NORTH_WEST = 2;
const short WEST = 3;

// #define N 32
// #define M 40

#define N 32
#define M 65536

extern "C" {

void compute_matrices(
    ap_uint<3> *string1, ap_uint<3> *string2,
    int *max_index,
    ap_uint<512> *similarity_matrix,
    ap_uint<512> *direction_matrix)
{
    // Declare the interfaces
    #pragma HLS INTERFACE m_axi port=string1 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=string2 offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=similarity_matrix offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi port=direction_matrix offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi port=max_index offset=slave bundle=gmem4
    
    #pragma HLS INTERFACE s_axilite port=string2 bundle=control
    #pragma HLS INTERFACE s_axilite port=string1 bundle=control
    #pragma HLS INTERFACE s_axilite port=similarity_matrix bundle=control
    #pragma HLS INTERFACE s_axilite port=direction_matrix bundle=control
    #pragma HLS INTERFACE s_axilite port=max_index bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    // Complete partition so that each data can be read at the same time
    ap_uint<3> local_string1[N];
    #pragma HLS ARRAY_PARTITION variable=local_string1 complete dim=1

    LOAD_S1: for (int p = 0; p < N; p++) {
        #pragma HLS UNROLL
        local_string1[p] = string1[p];
    }

    // Shift register fot the database (string2)
    ap_uint<3> local_string2_window[N];
    #pragma HLS ARRAY_PARTITION variable=local_string2_window complete dim=1

    INIT_S2: for (int s = 0; s < N; s++) {
        #pragma HLS UNROLL
        local_string2_window[s] = 4; // meaning 'P'
    }

    int l, i, j, k, r;
    short north, west, northwest;
    short val;
    short dir;
    short match_score;
    short local_max_val = -1;
    int local_max_index = 0;
    short test_n, test_nw, test_w;

    short buf_prev_prev[N + 1];
    short buf_prev[N + 1];
    short buf_curr[N + 1];
    short local_dir_diag[N + 1];

    #pragma HLS ARRAY_PARTITION variable=buf_prev_prev complete dim=1
    #pragma HLS ARRAY_PARTITION variable=buf_prev complete dim=1
    #pragma HLS ARRAY_PARTITION variable=buf_curr complete dim=1
    #pragma HLS ARRAY_PARTITION variable=local_dir_diag complete dim=1


    INIT_BUF: for (l = 0; l <= N; l++) {
        #pragma HLS UNROLL
        buf_prev_prev[l] = 0;
        buf_prev[l] = 0;
        buf_curr[l] = 0;
    }

    // Local buffers for the reduction tree
    short tree_val[32];
    int tree_idx[32];
    #pragma HLS ARRAY_PARTITION variable=tree_val complete
    #pragma HLS ARRAY_PARTITION variable=tree_idx complete

    LOOP_K: for (k = 2; k <= (N + M); k++) {
        #pragma HLS PIPELINE II=1

        // Shifting the local_string2_window
        SHIFT_S2: for (int s = N - 1; s > 0; s--) {
            #pragma HLS UNROLL
            local_string2_window[s] = local_string2_window[s - 1];
        }

        // Read only the next char from the DRAM
        ap_uint<3> next_char = 0;
        if (k - 2 < M) {
            next_char = string2[k - 2];
        }
        local_string2_window[0] = next_char;

        LOOP_I: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL

            buf_curr[i] = 0;
            local_dir_diag[i] = CENTER;

			north     = buf_prev[i];
			northwest = buf_prev_prev[i - 1];
			west      = buf_prev[i - 1];

			// Check if the querry and database match, via their local buffer
			match_score = (local_string1[i - 1] == local_string2_window[i - 1]) ? MATCH : MISS_MATCH;

			test_nw = northwest + match_score;
			test_n  = north + GAP_i;
			test_w = west + GAP_d;
			val = 0;
			dir = CENTER;

            // Pararrel MUX with priotry
			if (test_nw >= test_n && test_nw >= test_w && test_nw > 0) {
				val = test_nw;
				dir = NORTH_WEST;
			} else if (test_n >= test_w && test_n > 0) {
				val = test_n;
				dir = NORTH;
			} else if (test_w > 0) {
				val = test_w;
				dir = WEST;
			} else {
				val = 0;
				dir = CENTER;
			}

			buf_curr[i] = val;
			local_dir_diag[i] = dir;
        }//LOOP_I END


        // Pack the whole diagonal into 1 reg
        ap_uint<512> packed_similarity = 0;
        ap_uint<512> packed_direction = 0;

        PACK_LOOP: for (int p = 1; p <= N; p++) {
            #pragma HLS UNROLL
            // Mapping the values in the correct bit position
            packed_similarity((p-1)*16 + 15, (p-1)*16) = buf_curr[p];
            packed_direction((p-1)*16 + 15, (p-1)*16) = local_dir_diag[p];
        }

        // Only 1 writing in the DRAM per cycle
        similarity_matrix[k - 2] = packed_similarity;
        direction_matrix[k - 2] = packed_direction;

        // Gather the 32 values from the current diagonal, filtering out-of-bounds cells
        GATHER_MAX: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL
            j = k - i;
            
            // Strictly check if the current cell falls within the valid database bounds
            if (j >= 1 && j <= M) {
                tree_val[i-1] = buf_curr[i];
                tree_idx[i-1] = j * (N + 1) + i; // Correct, valid global index
            } else {
                // For out-of-bounds cells, assign -1 so they are discarded by the Reduction Tree
                tree_val[i-1] = -1;
                tree_idx[i-1] = 0;
            }
        }

        // Parallel Reduction Tree (Steps: 32 -> 16 -> 8 -> 4 -> 2 -> 1)
		REDUCTION_32_16: for (int s = 0; s < 16; s++) {
			#pragma HLS UNROLL
			if (tree_val[s + 16] > tree_val[s]) {
				tree_val[s] = tree_val[s + 16];
				tree_idx[s] = tree_idx[s + 16];
			}
		}
		REDUCTION_16_8: for (int s = 0; s < 8; s++) {
			#pragma HLS UNROLL
			if (tree_val[s + 8] > tree_val[s]) {
				tree_val[s] = tree_val[s + 8];
				tree_idx[s] = tree_idx[s + 8];
			}
		}
		REDUCTION_8_4: for (int s = 0; s < 4; s++) {
			#pragma HLS UNROLL
			if (tree_val[s + 4] > tree_val[s]) {
				tree_val[s] = tree_val[s + 4];
				tree_idx[s] = tree_idx[s + 4];
			}
		}
		REDUCTION_4_2: for (int s = 0; s < 2; s++) {
			#pragma HLS UNROLL
			if (tree_val[s + 2] > tree_val[s]) {
				tree_val[s] = tree_val[s + 2];
				tree_idx[s] = tree_idx[s + 2];
			}
		}
		REDUCTION_2_1: {
			if (tree_val[1] > tree_val[0]) {
				tree_val[0] = tree_val[1];
				tree_idx[0] = tree_idx[1];
			}
		}

        // Update the global maximum score and index only if the winning value is valid
        if (tree_val[0] > local_max_val && tree_val[0] != -1) {
            local_max_val = tree_val[0];
            local_max_index = tree_idx[0];
        }

        DATA_ROTATION: for (r = 0; r <= N; r++) {
            #pragma HLS UNROLL
            buf_prev_prev[r] = buf_prev[r];
		    buf_prev[r]      = buf_curr[r];
        }
    }
    *max_index = local_max_index;
}

}
