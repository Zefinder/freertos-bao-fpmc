#include <benchmark.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <generic_timer.h>

// Min, Max, Average
uint64_t sum = 0;
uint64_t min = -1;
uint64_t max = 0;
uint64_t average = -1;

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

void run_benchmark(benchmark_t benchmark_code)
{
    // Taking time before and after code execution
    uint64_t startTime = generic_timer_read_counter();
    benchmark_code();
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

void init_benchmark()
{
    printf("elapsed_time_array = [");
    base_frequency = generic_timer_get_freq();
}

void print_benchmark_results()
{
    printf("]\n"); // End elapsed time array
    printf("min = %ld # us\n", get_minimum_time());
    printf("max = %ld # us\n", get_maximum_time());
    printf("int_average = %ld # us\n", get_average_time());
}