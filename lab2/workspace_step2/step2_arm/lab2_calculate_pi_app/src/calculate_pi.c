#include <stdio.h>
#include "xil_printf.h"
#include "timer_util.h"

int main() {
    double nterms = 1000000000.0;
    double x = 0.0;
    
    CORE_TICKS total_time;
    secs_ret secs_passed;

    xil_printf("\n\r--- Pi Calculation Benchmark (Leibniz Series) ---\n\r");
    printf("Calculating Pi for %f terms...\n\r", nterms);

    start_time();

    for (int n = 0; n < nterms; n++) {
        double z = 1.0 / (2.0 * n + 1.0);
        
        if (n % 2 == 1) {
            z = -z;
        }
        x = x + z;
    }

    double pi = 4.0 * x;

    stop_time();
    total_time = get_time();
    secs_passed = time_in_secs(total_time);

    printf("Total Ticks: %llu\n\r", (unsigned long long)total_time);
    printf("Time elapsed: %f seconds\n\r", (double)secs_passed);

    printf("The value of pi is: %.30f\n\r", pi);


    return 0;
}
