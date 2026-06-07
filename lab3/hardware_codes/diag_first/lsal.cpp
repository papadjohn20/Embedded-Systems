#include <string.h>

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const short CENTER = 0;
const short NORTH = 1;
const short NORTH_WEST = 2;
const short WEST = 3;

#define N 32
#define M 65536

extern "C" {

void compute_matrices(
    char *string1, char *string2,
    int *max_index,
    int *similarity_matrix,
    short *direction_matrix)
{
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

    char local_string1[N];
    #pragma HLS ARRAY_PARTITION variable=local_string1 complete dim=1

    LOAD_S1: for (int p = 0; p < N; p++) {
        #pragma HLS UNROLL
        local_string1[p] = string1[p];
    }

    char local_string2[N];
    #pragma HLS ARRAY_PARTITION variable=local_string2 complete dim=1

    int l, i, j, k, r, o;
    short north, west, northwest;
    int val;
    short dir;
    short match_score;
    int local_max_val = -1;
    int local_max_index = 0;
    int test_nw, test_n, test_w;
    int burst_offset;

    int buf_prev_prev[N + 1];
    int buf_prev[N + 1];
    int buf_curr[N + 1];
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


    LOOP_K: for (k = 2; k <= (N + M); k++) {
        #pragma HLS PIPELINE

        LOAD_DIAG: for (o = 0; o < N; o++) {
            j = k - (o + 1);
            if (j >= 1 && j <= M) {
                local_string2[o] = string2[j - 1];
            } else {
                local_string2[o] = 0;
            }
        }

        LOOP_I: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL

            buf_curr[i] = 0;
            local_dir_diag[i] = CENTER;

            j = k - i;

            if (j >= 1 && j <= M) {
                north     = buf_prev[i];
                northwest = buf_prev_prev[i - 1];
                west      = buf_prev[i - 1];

                match_score = (local_string1[i - 1] == local_string2[i - 1]) ? MATCH : MISS_MATCH;

                test_nw = northwest + match_score;
                test_n  = north + GAP_i;
                test_w  = west + GAP_d;
                val = 0;
                dir = CENTER;

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

            }
        }

        FIND_MAX_DIAG: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL
            j = k - i;
            if (j >= 1 && j <= M) {
                if (buf_curr[i] > local_max_val) {
                    local_max_val = buf_curr[i];
                    local_max_index = j * (N + 1) + i;
                }
            }
        }

        // Sending the diagonal. The transformation to 2D will happen in the host code
        burst_offset = (k - 2) * N;
        WRITE_DRAM: for (i = 1; i <= N; i++) {
            #pragma HLS UNROLL
            similarity_matrix[burst_offset + (i - 1)] = buf_curr[i];
            direction_matrix[burst_offset + (i - 1)] = local_dir_diag[i];
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
