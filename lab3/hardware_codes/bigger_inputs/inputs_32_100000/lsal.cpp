#include <string.h>
#include <ap_int.h>

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const ap_uint<2> CENTER = 0;
const ap_uint<2> NORTH = 1;
const ap_uint<2> NORTH_WEST = 2;
const ap_uint<2> WEST = 3;

#define N 32
#define M 100000

extern "C" {

void compute_matrices(
    ap_uint<512> *string1,
    ap_uint<512> *string2,
    int *max_index,
    ap_uint<64> *direction_matrix)
{
    // Declare the interfaces
    #pragma HLS INTERFACE m_axi port=string1 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=string2 offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=direction_matrix offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi port=max_index offset=slave bundle=gmem3
    
    #pragma HLS INTERFACE s_axilite port=string2 bundle=control
    #pragma HLS INTERFACE s_axilite port=string1 bundle=control
    #pragma HLS INTERFACE s_axilite port=direction_matrix bundle=control
    #pragma HLS INTERFACE s_axilite port=max_index bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    // Complete partition so that each data can be read at the same time
    ap_uint<3> local_string1[N];
    #pragma HLS ARRAY_PARTITION variable=local_string1 complete dim=1

    // Reading string1
    ap_uint<512> s1_block = string1[0]; 
    LOAD_S1: for (int p = 0; p < N; p++) {
        #pragma HLS UNROLL
        // Unpacking from the 512bit register
        local_string1[p] = s1_block((p * 3) + 2, p * 3);
    }

    // Shift register for the database (string2)
    ap_uint<3> local_string2_window[N];
    #pragma HLS ARRAY_PARTITION variable=local_string2_window complete dim=1

    INIT_S2: for (int s = 0; s < N; s++) {
        #pragma HLS UNROLL
        local_string2_window[s] = 4; // meaning 'P'
    }

    int l, i, j, k, r;
    short north, west, northwest;
    short val;
    ap_uint<2> dir;
    short match_score;
    short local_max_val = -1;
    int local_max_index = 0;
    short test_n, test_nw, test_w;

    short buf_prev_prev[N + 1];
    short buf_prev[N + 1];
    short buf_curr[N + 1];
    ap_uint<2>  local_dir_diag[N + 1];

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

    // Local buffer for 170 3bit-characters
    ap_uint<512> s2_current_block = 0;

    LOOP_K: for (k = 2; k <= (N + M); k++) {
        #pragma HLS PIPELINE

        // Shifting the local_string2_window
        SHIFT_S2: for (int s = N - 1; s > 0; s--) {
            #pragma HLS UNROLL
            local_string2_window[s] = local_string2_window[s - 1];
        }

        // Reading every 170 characters
        int idx_s2 = k - 2;
        ap_uint<3> next_char = 0;

        if (idx_s2 < M) {
            int block_idx = idx_s2 / 170; // Which 512-bit block from string2
            int bit_offset = (idx_s2 % 170) * 3; // Which character from this block

            // Checking for when to take the new 512-bit block from DRAM
            if (idx_s2 % 170 == 0) {
                s2_current_block = string2[block_idx];
            }
            // Unpacking character from local register in 1 cycle
            next_char = s2_current_block(bit_offset + 2, bit_offset);
        }
        local_string2_window[0] = next_char;

        LOOP_I: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL

            buf_curr[i] = 0;
            local_dir_diag[i] = CENTER;

            north     = buf_prev[i];
            northwest = buf_prev_prev[i - 1];
            west      = buf_prev[i - 1];

            // Check if the query and database match, via their local buffer
            match_score = (local_string1[i - 1] == local_string2_window[i - 1]) ? MATCH : MISS_MATCH;

            test_nw = northwest + match_score;
            test_n  = north + GAP_i;
            test_w = west + GAP_d;
            val = 0;
            dir = CENTER;

            // Parallel MUX with priority
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
        } // LOOP_I end

        // Pack the whole diagonal into 1 reg
        ap_uint<64> packed_direction = 0;
        PACK_LOOP: for (int p = 1; p <= N; p++) {
            #pragma HLS UNROLL
            packed_direction((p-1)*2 + 1, (p-1)*2) = local_dir_diag[p];
        }

        direction_matrix[k - 2] = packed_direction;

        // Gather the 32 values from the current diagonal, filtering out-of-bounds cells
        GATHER_MAX: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL
            j = k - i;
            
            if (j >= 1 && j <= M) {
                tree_val[i-1] = buf_curr[i];
                tree_idx[i-1] = j * (N + 1) + i; 
            } else {
                tree_val[i-1] = -1;
                tree_idx[i-1] = 0;
            }
        }

        // Parallel Reduction Tree
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
