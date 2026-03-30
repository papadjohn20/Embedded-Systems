#include <stdio.h>
#include "xil_printf.h"
#include "timer_util.h"

// Rows and Columns
#define SIZE 1025
#define R1 SIZE
#define C1 SIZE
#define R2 SIZE
#define C2 SIZE

int main()
{
    int i, j, k;
    static int m1[R1][C1];
    static int m2[R2][C2];
    static int result[R1][C2];
    CORE_TICKS total_time;
    secs_ret secs_passed;

    // Initialize the matrices
    for (i = 0; i < R1; i++)
        for (j = 0; j < C1; j++)
            m1[i][j] = i + j;

    for (i = 0; i < R2; i++)
        for (j = 0; j < C2; j++)
            m2[i][j] = i * j;

    xil_printf("\n\r...Starting Matrix Multiplication (%dx%d)...\n\r", SIZE, SIZE);

    // start timer
    start_time();

    // Calculate the result
    for (i = 0; i < R1; i++) {
        for (j = 0; j < C2; j++) {
            result[i][j] = 0;
            for 	(k = 0; k < R2; k++) {
                result[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }

    // stop timer
    stop_time();
    total_time = get_time();
    secs_passed = time_in_secs(total_time);

    printf("Total Ticks: %llu\n\r", (unsigned long long)total_time);
    printf("Time elapsed: %f seconds\n\r", (double)secs_passed);

    return 0;
}
