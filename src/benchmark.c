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

// Benchmark name
char* bench_name;

int test_counter = 0;

uint64_t base_frequency;

int get_minimum_time()
{
    return pdSYSTICK_TO_US(base_frequency, min);
}

int get_maximum_time()
{
    return pdSYSTICK_TO_US(base_frequency, max);
}

int get_average_time()
{
    average = sum / test_counter;
    return pdSYSTICK_TO_US(base_frequency, average);
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
    printf("%ld,", pdSYSTICK_TO_US(base_frequency, elapsedTime));
}

void init_benchmark(char* benchmark_name)
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

    // Reset min, max and average
    min = -1;
    max = 0;
    sum = 0;
    average = 0;

    // Reset test counter
    test_counter = 0;

    // Get base frequency 
    base_frequency = generic_timer_get_freq();

    printf("elapsed_time_array%s = [", bench_name);
}

void print_benchmark_results()
{
    printf("]\n"); // End elapsed time array
    printf("min%s = %ld # us\n", bench_name, get_minimum_time());
    printf("max%s = %ld # us\n", bench_name, get_maximum_time());
    printf("int_average%s = %ld # us\n", bench_name, get_average_time());
}

void start_benchmark()
{
    printf("# Benchmark start\n");
}

void end_benchmark()
{
    printf("# Benchmark end\n");
}