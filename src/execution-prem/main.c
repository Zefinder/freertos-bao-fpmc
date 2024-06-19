#include <FreeRTOS.h>
#include <task.h>

#include <uart.h>
#include <irq.h>
#include <plat.h>

#include <stdio.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <ipi.h>
#include <periodic_task.h>
#include <prem_task.h>
#include <data.h>
#include <generic_timer.h>
#include <benchmark.h>

// Request the default IPI and memory request wait
#ifndef DEFAULT_IPI_HANDLERS
#error DEFAULT_IPI must be defined with value y for this test (make all ... DEFAULT_IPI=y)
#endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 90000
#define DATA_SIZE_0 100 kB
#define DATA_SIZE_INTERFERENCE 10 kB

// Generic timer frequency
uint64_t timer_frequency;

uint64_t end_time = 0;
int counter = 0;
int print_counter = 0;
uint32_t prio = 0;
uint64_t fetch_sum = 0;
uint64_t min_time = -1;
uint64_t max_time = 0;
void vMasterTask(void *pvParameters)
{
    printf("AAAAAAAAAAAAAAH\n");

    // Get time at start of computation phase
    uint64_t current_time = generic_timer_read_counter();

    // If first time, just reask
    if (end_time == 0)
    {
        end_time = current_time;
        return;
    }

    // Else that means that we have a PREM fetch time
    uint64_t fetch_time = current_time - end_time;

    // Increase sum
    fetch_sum += fetch_time;

    // Test min and max
    if (fetch_time > max_time)
    {
        max_time = fetch_time;
    }

    if (fetch_time < min_time)
    {
        min_time = fetch_time;
    }

    // If first measurement, then display name
    if (counter == 0)
    {
        printf("elapsed_time_array_%dkB_prio%d_ns = [", DATA_SIZE_0, 2 * prio);
    }

    // Show result and print add print counter
    printf("%ld,", pdSYSTICK_TO_NS(timer_frequency, fetch_time));
    if (++print_counter == 1000)
    {
        printf("\t# Number of realised tests: %d\n", counter);
        print_counter = 0;
    }

    // When tests finished, show results, reset counters and increment prio
    if (counter == NUMBER_OF_TESTS)
    {
        // Compute average
        uint64_t average_time = fetch_sum / NUMBER_OF_TESTS;

        // Print results
        printf("]\n"); // End of array
        printf("min_%dkB_prio%d_ns = %ld # ns\n", DATA_SIZE_0, prio, pdSYSTICK_TO_NS(timer_frequency, min_time));
        printf("max_%dkB_prio%d_ns = %ld # ns\n", DATA_SIZE_0, prio, pdSYSTICK_TO_NS(timer_frequency, max_time));
        printf("int_average_%dkB_prio%d_ns = %ld # ns\n", DATA_SIZE_0, prio, pdSYSTICK_TO_NS(timer_frequency, average_time));
        printf("\n");

        // Reset everything
        end_time = 0;
        counter = 0;
        print_counter = 0;
        fetch_sum = 0;
        min_time = -1;
        max_time = 0;

        // Increment prio
        prio++;
    }

    // If prio > 3 then tests finished.
    if (prio > 3)
    {
        end_benchmark();
        printf("\n");
        vTaskDelete(NULL);
    }

    // Set new old time and begin fetching again
    end_time = generic_timer_read_counter();
}

void vTaskInterference(void *pvParameters)
{
    // Computation phase will be the sum of all elements in the array
    uint64_t offset = (uint64_t)pvParameters;
    uint64_t sum = 0;
    for (int i = offset; i < offset + DATA_SIZE_INTERFERENCE; i++)
    {
        sum += appdata[i];
    }
}

struct premtask_parameters premtask_parameters;
void main_app(void)
{
    timer_frequency = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    // Init and create PREM task
    vInitPREM();

    if (cpu_id == 0)
    {
        // Init benchmark
        printf("Begin PREM tests...\n");
        start_benchmark();

        // Fill in the struct all "interesting"
        premtask_parameters.tickPeriod = 0;
        premtask_parameters.data_size = DATA_SIZE_0;
        premtask_parameters.data = appdata;
        premtask_parameters.priority = &prio;
        premtask_parameters.pvParameters = NULL;
        xTaskPREMCreate(
            vMasterTask,
            "MasterTask",
            configMINIMAL_STACK_SIZE,
            premtask_parameters,
            tskIDLE_PRIORITY + 1,
            &(xTaskHandler));
    }
    else
    {
        // Just interferences, the goal is to interfere, give the parameters to fetch... (cpu_id and prefetch offset)
        // From 120kB to 130kB for CPU 1
        // From 140kB to 150kB for CPU 2
        // From 160kB to 170kB for CPU 3
        prio = 2 * cpu_id - 1;
        premtask_parameters.tickPeriod = 0;
        premtask_parameters.data_size = DATA_SIZE_INTERFERENCE;
        premtask_parameters.data = appdata + DATA_SIZE_0 + 2 * cpu_id * DATA_SIZE_INTERFERENCE;
        premtask_parameters.priority = &prio;
        premtask_parameters.pvParameters = (int *)(DATA_SIZE_0 + 2 * cpu_id * DATA_SIZE_INTERFERENCE);
        xTaskPREMCreate(
            vTaskInterference,
            "Interference",
            configMINIMAL_STACK_SIZE,
            premtask_parameters,
            tskIDLE_PRIORITY + 1,
            NULL);
    }

    vTaskStartScheduler();
}