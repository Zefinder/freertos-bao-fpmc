#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

/* Defines the prototype to which benchmark functions must conform */
typedef void (*benchmark_t)(void*);

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
 * 
 * If the benchmark function requires an argument, you can give it via
 * the void* second argument
 *
 * During the test, things will be printed, unless you don't want them,
 * it is recommended to not print anything after running the init
 * function... If you really want to print something and use the results
 * in Python, you can print your text beginning with # and do not forget
 * the line return!
 */
void run_benchmark(benchmark_t benchmark_code, void *benchmark_argument);

/*
 * Init the benchmark with the timer frequency, it's better to measure
 * things. You can give the benchmark a name that will be given to the
 * benchmark output variables. The name can start with _ to be better,
 * since it will be concatenated with the base variable names. You also
 * specify if you want the time in microseconds (µs) or nanoseconds 
 * (ns). If use_nano is 1, then time will be given in ns. 
 *
 * For instance, if the name is "_color", the variable names will be
 * min_color, max_color, ...
 *
 * If benchmark_name is NULL, then it will be replaced by the empty
 * string
 */
void init_benchmark(char *benchmark_name, int use_nano);

/*
 * Print all times, the min, the max and the int average, ideal for a
 * copy to put it on python and compute other things that can require
 * floating points
 */
void print_benchmark_results();

/* Start benchmarks printing the starting string */
void start_benchmark();

/* End benchmarks printing the end string */
void end_benchmark();

#endif