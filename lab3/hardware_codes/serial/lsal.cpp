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

#define N 32
#define M 65536

extern "C" {

void compute_matrices(
    char *string1, char *string2,
    int *max_index, int *similarity_matrix, short *direction_matrix)
{
    #pragma HLS INTERFACE m_axi port=string1 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=string2 offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=similarity_matrix offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi port=direction_matrix offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi port=max_index offset=slave bundle=gmem4

    #pragma HLS INTERFACE s_axilite port=string1 bundle=control
    #pragma HLS INTERFACE s_axilite port=string2 bundle=control
    #pragma HLS INTERFACE s_axilite port=similarity_matrix bundle=control
    #pragma HLS INTERFACE s_axilite port=direction_matrix bundle=control
    #pragma HLS INTERFACE s_axilite port=max_index bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    char local_string1[N];
    #pragma HLS ARRAY_PARTITION variable=local_string1 complete

    LOAD_S1: for (int i = 0; i < N; i++) {
        #pragma HLS PIPELINE II=1
        local_string1[i] = string1[i];
    }


    short local_dir_row[N + 1];
    int local_west;
    int local_northwest;
    int prev_row[N + 1];

    int i, j;
    int north, west, northwest;
    int val;
    short dir;
    int match_score;
    int max_val = -1;
    int test_nw, test_n, test_w;
    int row_offset;
    char string2_temp;

    INIT_ROW: for(i=0; i<=N; i++) {
        #pragma HLS PIPELINE II=1
        prev_row[i] = 0;
    }

    ROW_LOOP: for (j = 1; j <= M; j++) {
        row_offset = j * (N+1);
        string2_temp = string2[j-1]; // Read the database character once per row

        local_west = 0;
        local_northwest = 0;

        COL_LOOP: for (i = 1; i <= N; i++) {
            // The PIPELINE pragma tells the tool to start a new iteration every 1 cycle (II=1)
            #pragma HLS PIPELINE II=1

            north = prev_row[i];
            west = local_west;
            northwest = local_northwest;

            val = 0;
            dir = CENTER;

            // Use local_string1 from the internal BRAM
            match_score = (local_string1[i-1] == string2_temp) ? MATCH : MISS_MATCH;
            test_nw = northwest + match_score;
            if (test_nw > val) {
                val = test_nw;
                dir = NORTH_WEST;
            }

            test_n = north + GAP_i;
            if (test_n > val) {
                val = test_n;
                dir = NORTH;
            }

            test_w = west + GAP_d;
            if (test_w > val) {
                val = test_w;
                dir = WEST;
            }

            local_west = val;
            local_northwest = north;
            prev_row[i] = val;

            local_dir_row[i] = dir;

            if (val > max_val) {
                max_val = val;
                *max_index = row_offset + i;
            }
        }

        memcpy(&similarity_matrix[row_offset + 1], &prev_row[1], N * sizeof(int));
        memcpy(&direction_matrix[row_offset + 1], &local_dir_row[1], N * sizeof(short));

    }
}

} // End of extern "C"
