#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

/* Defines the prototype to which benchmark functions must conform */
typedef void (*benchmark_t)(void);

/* Get minimum execution time in ticks */
int get_minimum_time();

/* Get maximum execution time in ticks */
int get_maximum_time();

/* Get average execution time in ticks */
int get_average_time();

/* 
 * Run the benchmark! It means running the code once while measuring the
 * execution time. Then computes the average time with all latter runs,
 * remembering maximum (worst execution time) and minimum (best execution
 * time) time. 
 * 
 * The code to test must be the one to measure. For example if you want 
 * to check the prefetch time, you will flush the cache before or after
 * the benchmark but not during (unless you want to measure the time of
 * prefetching and then cleaning)
 */
void run_benchmark(benchmark_t benchmark_code);

/*
 * Init the benchmark with the total number of tests you want to do. 
 * The number you indicate is the upper bound of the number of tests, if
 * you do less it's ok and if you do more, there will be a warning but 
 * it will replace the first values (to continue with no crash).
 */
void init_benchmark(int number_of_tests);

/* Prints the results */
void print_benchmark_results();

#endif