#include <benchmark.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <generic_timer.h>

// Min, Max, Average
uint64_t min = -1;
uint64_t max = 0;
uint64_t average = -1;

uint64_t *execution_time;
int test_upper_bound = 0;
int test_counter = 0;
int test_size = 0;

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
    uint64_t sum = 0;
    for (int index = 0; index < test_size; index++)
    {
        sum += execution_time[index];
    }

    average = sum / test_size;
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

    // Put in array for average (index to start if overflow)
    execution_time[test_counter++] = elapsedTime;
    if (test_counter == test_upper_bound)
    {
        test_counter = 0;
    }
    if (test_size != test_upper_bound)
    {
        test_size += 1;
    }
}

void init_benchmark(int number_of_tests)
{
    test_upper_bound = number_of_tests;
    execution_time = (uint64_t *)pvPortMalloc(sizeof(uint64_t) * number_of_tests);
    base_frequency = generic_timer_get_freq();
}

void print_benchmark_results()
{
    printf("MIN: %ld us\n", get_minimum_time());
    printf("MAX: %ld us\n", get_maximum_time());
    printf("AVERAGE: %ld us\n", get_average_time());
}