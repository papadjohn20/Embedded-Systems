#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

// #define N 256
// #define M 2048

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const short CENTER = 0;
const short NORTH = 1;
const short NORTH_WEST = 2;
const short WEST = 3;
// static long int cnt_ops=0;
// static long int cnt_bytes=0;


/**********************************************************************************************
 * LSAL algorithm
 * Inputs:
 *          string1 is the query[N]
 *          string2 is the database[M]
 *          input sizes N, M
 * Outputs:
 *           max_index is the location of the highest similiarity score 
 *           similarity and direction matrices. Note that these two matrices are initialized with zeros.
 **********************************************************************************************/

void compute_matrices(
    char *string1, char *string2,
    int *max_index, int *similarity_matrix, short *direction_matrix, int N, int M)
{
    int i, j, index; // Loop indices and the linear index for accessing the similarity_matrix and direction_matrix
	int north, west, northwest; // Values from the neighboring cells in the similarity matrix
	int val; // The computed similarity score for the current cell
	short dir; // The direction of the optimal move for the current cell
	int match_score; // The score for a match or mismatch between characters
    int max_val = -1; // The maximum similarity score found so far, initialized to -1
    *max_index = 0; // The index of the maximum value in the similarity matrix, initialized to 0
	
	// Temporary variables for the scores of the three possible moves
	int test_nw, test_n, test_w;

    int row_offset; // For j*N
    char string2_temp; // For string2[j-1]
    
    // Outer loop iterates over the Database (Rows)
    for (j = 1; j <= M; j++) {
        row_offset = j * (N+1);
        string2_temp = string2[j-1];
        // Inner loop iterates over the Query (Columns)
        for (i = 1; i <= N; i++) {
            index = row_offset + i;

            // Retrieve values from neighbor cells with boundary checks
            north = similarity_matrix[index - N - 1];
            west =  similarity_matrix[index - 1];
            northwest = similarity_matrix[index - N - 2];

            // Initialize with CENTER
            val = 0;
            dir = CENTER;

            // Check NORTH_WEST
            match_score = (string1[i-1] == string2_temp) ? MATCH : MISS_MATCH;
            test_nw = northwest + match_score;
            if (test_nw > val) {
                val = test_nw;
                dir = NORTH_WEST;
            }

            // Check NORTH
            test_n = north + GAP_i;
            if (test_n > val) {
                val = test_n;
                dir = NORTH;
            }

            // Check WEST
            test_w = west + GAP_d;
            if (test_w > val) {
                val = test_w;
                dir = WEST;
            }

            // Store the results in the matrices
            similarity_matrix[index] = val;
            direction_matrix[index] = dir;

            // Track the maximum value
            if (val > max_val) {
                max_val = val;
                *max_index = index;
            }
        }
    }
}

/************************************************************************/

/*
 return a random number in [0, limit].
 */
int rand_lim(int limit) {

	int divisor = RAND_MAX / (limit + 1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval;
}

/*
 Fill the string with random values
 */
void fillRandom(char* string, int dimension) {
	//fill the string with random letters..
	static const char possibleLetters[] = "ATCG";

	string[0] = '-';

	int i;
	for (i = 0; i < dimension; i++) {
		int randomNum = rand_lim(3);
		string[i] = possibleLetters[randomNum];
	}

}

/*
 Print the similarity and direction matrices
 */
void print_matrices(int *similarity_matrix, short *direction_matrix, int N, int M, char *query, char *database) {
    printf("\nSimilarity Matrix:\n     ");
    for (int i = 0 ; i < N; i++) printf("%3c ", query[i]);
    printf("\n");

    for (int j = 1; j <= M; j++) {
        printf("%3c  ", database[j-1]);
        for (int i = 1; i <= N; i++) {
            printf("%3d ", similarity_matrix[j * (N+1) + i ]);
        }
        printf("\n");
    }

    printf("\nDirection Matrix:\n     ");
    for (int i = 0; i < N; i++) printf("%3c ", query[i]);
    printf("\n");

    const char* dir_symbols[] = {"C", "N", "NW", "W"};

    for (int j = 1; j <= M; j++) {
        printf("%3c  ", database[j-1]);
        for (int i = 1; i <= N; i++) {
            printf("%3s ", dir_symbols[direction_matrix[j *(N+1) + i]]);
        }
        printf("\n");
    }
}

/* ******************************************************************/
int main(int argc, char** argv) {

    clock_t t1, t2;

	if (argc != 3) {
		printf("%s <Query Size N> <DataBase Size M>\n", argv[0]);
		return EXIT_FAILURE;
	}
 
    printf("Starting Local Alignment Code \n");
	fflush(stdout);

	/* Typically, M >> N */
	int N = atoi(argv[1]); 
    int M = atoi(argv[2]);

    char *query = (char*) malloc(sizeof(char) * N);
	char *database = (char*) malloc(sizeof(char) * M);
	int *similarity_matrix = (int*) malloc(sizeof(int) * (N + 1) * (M + 1));
    short *direction_matrix = (short*) malloc(sizeof(short) * (N + 1) * (M + 1));
	int *max_index = (int *) malloc(sizeof(int));

/* Create the two input strings by calling a random number generator */
	fillRandom(query, N);
	fillRandom(database, M);
    
    // --- Example from the pdf ---
    // M = 9 AND N = 8
    //strncpy(query, "TGTTACGG", N);
    //strncpy(database, "GGTTGACTA", M);
    // --- End of example ---
    memset(similarity_matrix, 0, sizeof(int) * (N + 1) * (M + 1));
	memset(direction_matrix, 0, sizeof(short) * (N + 1) * (M + 1));

    t1 = clock();
	compute_matrices(query, database, max_index, similarity_matrix, direction_matrix, N, M);
	t2 = clock();

    printf(" max index is in position (%d, %d) \n", max_index[0]/(N+1) - 1, max_index[0]%(N+1) - 1);
	printf(" execution time of LSAL SW algorithm is %f sec \n", (double)(t2-t1) / CLOCKS_PER_SEC);
    
    // ---- PRINT MATRICES FOR DEBUGGING PURPOSES ----
    //print_matrices(similarity_matrix, direction_matrix, N, M, query, database);

    printf("\nMaximum Score found: %d at index %d\n", similarity_matrix[*max_index], *max_index - N*2);
	free(similarity_matrix);
	free(direction_matrix);
	free(max_index);

    // printf("cnt_ops=%d, cnt_bytes=%d\n", cnt_ops, cnt_bytes);
	return EXIT_SUCCESS;
}
