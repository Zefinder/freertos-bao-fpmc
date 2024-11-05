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
#ifndef MEMORY_REQUEST_WAIT
    #error MEMORY_REQUEST_WAIT must be defined for this benchmark, (make all ... MEMORY_REQUEST_WAIT=y)
#endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 90000
#define DATA_SIZE_0 100 kB
#define DATA_SIZE_INTERFERENCE 40 kB
#define MAX_PRIO 6

// Generic timer frequency
uint64_t timer_frequency;

int counter = 0;
int print_counter = 0;
uint32_t prio = 0;
volatile uint64_t summm = 0;

void vMasterTask(void *pvParameters)
{
    // start_benchmark();

    // while (prio < MAX_PRIO + 1)
    // {
    //     // Init benchmark
    //     char test_name[20];
    //     sprintf(test_name, "_%dkB_prio%d_interference%dkB", BtkB(DATA_SIZE_0), prio, BtkB(DATA_SIZE_INTERFERENCE));
    //     init_benchmark(test_name, MEASURE_NANO);

    //     while (counter++ < NUMBER_OF_TESTS)
    //     {
    //         run_benchmark(prefetch, (void *)(DATA_SIZE_0));

    //         // Print for each 1000 tests
    //         if (++print_counter == 1000)
    //         {
    //             printf("\t# Number of realised tests: %d\n", counter);
    //             print_counter = 0;
    //         }

    //         // Wait for 3 x last time
    //         vTaskPREMDelay(3 * get_last_measured_time());
    //     }

    //     // Reset counters
    //     counter = 0;
    //     print_counter = 0;
    //     prio += 2;

    //     // Print results
    //     print_benchmark_results();
    //     printf("\n");
    // }

    // end_benchmark();
    // vTaskDelete(NULL);
    // printf("Coucou!\n");
    for (int i = 0; i < DATA_SIZE_INTERFERENCE; i++)
    {
        summm += appdata[i];
    }
    summm = 0;
}

// void vTaskInterference(void *pvParameters)
// {
//     // Always do it
//     while (1)
//     {
//         // prefetch(pvParameters);

//         // Computation phase will be the sum of all elements in the array
//         uint64_t sum = 0;
//         for (int i = 0; i < DATA_SIZE_INTERFERENCE; i++)
//         {
//             sum += appdata[i];
//         }
//     }
// }

void main_app(void)
{
    timer_frequency = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    if (cpu_id == 0)
    {
        // printf("Begin PREM tests...\n");
        // xTaskCreate(
        //     vMasterTask,
        //     "MasterTask",
        //     configMINIMAL_STACK_SIZE,
        //     NULL,
        //     tskIDLE_PRIORITY + 1,
        //     &(xTaskHandler));

        struct premtask_parameters parameters1 = {.tickPeriod=10, .data_size=MAX_DATA_SIZE, .data=appdata, .wcet=10000000, .pvParameters=NULL};

        xTaskPREMCreate(
            vMasterTask,
            "Master task1",
            configMINIMAL_STACK_SIZE,
            parameters1,
            0,
            NULL
        );

        xTaskPREMCreate(
            vMasterTask,
            "Master task1",
            configMINIMAL_STACK_SIZE,
            parameters1,
            1,
            NULL
        );
    }
    // else
    // {
    //     // Just interferences, the goal is to interfere, give the parameters to fetch... (cpu_id and prefetch offset)
    //     prio = 2 * cpu_id - 1;
    //     xTaskCreate(
    //         vTaskInterference,
    //         "Interference",
    //         configMINIMAL_STACK_SIZE,
    //         (void *)(DATA_SIZE_INTERFERENCE),
    //         tskIDLE_PRIORITY + 1,
    //         NULL);
    // }

    vTaskStartScheduler();
}