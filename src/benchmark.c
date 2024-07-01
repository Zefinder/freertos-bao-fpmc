#include <benchmark.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <generic_timer.h>

// Min, Max, Average
uint64_t sum = 0;
uint64_t min = -1;
uint64_t max = 0;
uint64_t average = 0;

// Last measured time
uint64_t last_measured_time = 0;

// Benchmark config
char *bench_name;
int nanotime;

int test_counter = 0;

uint64_t base_frequency;

uint64_t get_minimum_time()
{
    int min_time;
    if (nanotime)
    {
        min_time = pdSYSTICK_TO_NS(base_frequency, min);
    }
    else
    {
        min_time = pdSYSTICK_TO_US(base_frequency, min);
    }
    return min_time;
}

uint64_t get_maximum_time()
{
    int max_time;
    if (nanotime)
    {
        max_time = pdSYSTICK_TO_NS(base_frequency, max);
    }
    else
    {
        max_time = pdSYSTICK_TO_US(base_frequency, max);
    }
    return max_time;
}

uint64_t get_average_time()
{
    average = sum / test_counter;
    int average_time;
    if (nanotime)
    {
        average_time = pdSYSTICK_TO_NS(base_frequency, average);
    }
    else
    {
        average_time = pdSYSTICK_TO_US(base_frequency, average);
    }
    return average_time;
}

void run_benchmark(benchmark_t benchmark_code, void *benchmark_argument)
{
    // Taking time before and after code execution
    uint64_t startTime = generic_timer_read_counter();
    benchmark_code(benchmark_argument);
    uint64_t endTime = generic_timer_read_counter();

    // Compute execution time
    uint64_t elapsedTime = endTime - startTime;

    // Test for minimum
    if (min == -1 || elapsedTime < min)
    {
        min = elapsedTime;
    }

    // Test for maximum
    if (elapsedTime > max)
    {
        max = elapsedTime;
    }

    // Add elapsed time with other times
    sum += elapsedTime;
    test_counter += 1;

    // Print result for result array
    uint64_t measured_time;
    if (nanotime)
    {
        measured_time = pdSYSTICK_TO_NS(base_frequency, elapsedTime);
        last_measured_time = measured_time;
    }
    else
    {
        measured_time = pdSYSTICK_TO_US(base_frequency, elapsedTime);
        last_measured_time = pdSYSTICK_TO_NS(base_frequency, elapsedTime);
    }

    printf("%ld,", measured_time);
}

void init_benchmark(char *benchmark_name, int use_nano)
{
    // Set benchmark name
    if (benchmark_name == NULL)
    {
        bench_name = "";
    }
    else
    {
        bench_name = benchmark_name;
    }

    // Set nanotime
    nanotime = use_nano;

    // Reset min, max and average
    min = -1;
    max = 0;
    sum = 0;
    average = 0;

    // Reset last measured time
    last_measured_time = 0;

    // Reset test counter
    test_counter = 0;

    // Get base frequency
    base_frequency = generic_timer_get_freq();

    if (nanotime)
    {
        printf("elapsed_time_array%s_ns = [", bench_name);
    }
    else
    {
        printf("elapsed_time_array%s = [", bench_name);
    }
}

uint64_t get_last_measured_time()
{
    return last_measured_time;
}

void print_benchmark_results()
{
    printf("]\n"); // End elapsed time array

    if (nanotime)
    {
        printf("min%s_ns = %ld # ns\n", bench_name, get_minimum_time());
        printf("max%s_ns = %ld # ns\n", bench_name, get_maximum_time());
        printf("int_average%s_ns = %ld # ns\n", bench_name, get_average_time());
    }
    else
    {
        printf("min%s = %ld # us\n", bench_name, get_minimum_time());
        printf("max%s = %ld # us\n", bench_name, get_maximum_time());
        printf("int_average%s = %ld # us\n", bench_name, get_average_time());
    }
}

void start_benchmark()
{
    printf("# Benchmark start\n");
}

void end_benchmark()
{
    printf("# Benchmark end\n");
}