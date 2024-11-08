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
#include "inc/TACle.h"

// To use with only one CPU!

// #ifndef MEASURE_RESPONSE_TIME
// #error MEASURE_RESPONSE_TIME must be defined to run this test
// #endif
// #ifndef DEFAULT_IPI_HANDLERS
//     #error DEFAULT_IPI must be defined with value y for this test (make all ... DEFAULT_IPI=y)
// #endif
// #ifndef MEMORY_REQUEST_WAIT
//     #error MEMORY_REQUEST_WAIT must be defined for this benchmark, (make all ... MEMORY_REQUEST_WAIT=y)
// #endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 10

uint64_t ntest_mpeg = 0;
uint64_t ntest_countnegative = 0;
uint64_t ntest_weightavg = 0;

void mpeg2_task(void *pvParameters)
{
    // Execute function
    mpeg2_main();

    // When executed everything, detete itself
    if (++ntest_mpeg == NUMBER_OF_TESTS)
    {
        vTaskDelete(NULL);
    }
}

// void countnegative_task(void *pvParameters)
// {
//     // Execute function
//     countnegative_main();

//     // When executed everything, detete itself
//     if (++ntest_countnegative == NUMBER_OF_TESTS)
//     {
//         vTaskDelete(NULL);
//     }
// }

void weightavg_task(void *pvParameters)
{
    printf("%d\n", ntest_weightavg);

    // Will perform sum += (array1[i] - array[2]) / 2
    // avg = sum / 102400
    // 200kB of appdata loaded in the cache!
    uint32_t offset_1 = 0;
    uint32_t offset_2 = 102400;

    uint64_t array_sum = 0;
    uint64_t array_avg = 0;
    for (int index = 0; index < 102400; index++)
    {
        array_sum += (appdata[index + offset_1] - appdata[index + offset_2]) / 2;
    }

    array_avg = array_sum / 102400;

    // This line is here to ensure that array_avg is computed
    array_sum += array_avg;

    // When executed everything, detete itself
    if (++ntest_weightavg >= NUMBER_OF_TESTS)
    {
        vTaskDelete(NULL);
    }
}
 
// void logic(void *arg)
// {
//     uint8_t *data = get_mpeg2_oldorgframe();

//     // Request memory
//     request_memory_access(0, 0);

//     // Prefetch data
//     prefetch_data((uint64_t)data, 90112);

//     // Revoke memory
//     revoke_memory_access();

//     // Compute
//     mpeg2_main();

//     // Clear cache
//     clear_L2_cache((uint64_t)data, 90112);
// }

// void task(void *pvParameters)
// {
//     start_benchmark();
//     init_benchmark("_mpeg", 1);
//     for (int ntest = 0; ntest < NUMBER_OF_TESTS; ntest++) {
//         run_benchmark(logic, NULL);
//     }
//     print_benchmark_results();
//     end_benchmark();
// }

void main_app(void)
{

    /* Two tasks from TACleBench:
     * - MPEG2
     * - Count negative
     * One custom task:
     * - Weighted average of 2 100kB arrays
     */

    // Init countnegative matrix
    // countnegative_init();

    printf("%d\n", configUSE_PREEMPTION);

    uint8_t *mpeg_data = get_mpeg2_oldorgframe();
    // uint8_t *countnegative_data = get_countnegative_array();

    struct premtask_parameters mpeg_struct = {.tickPeriod = 100, .data_size = 90112, .data = mpeg_data, .wcet = 0, .pvParameters = NULL};
    // struct premtask_parameters countnegative_struct = {.tickPeriod = 10, .data_size = 147456, .data = countnegative_data, .wcet = 0, .pvParameters = NULL};
    struct premtask_parameters weigthavg_struct = {.tickPeriod = 3000, .data_size = 204800, .data = appdata, .wcet = 0, .pvParameters = NULL};

    // xTaskCreate(task, "aa", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskPREMCreate(
        mpeg2_task,
        "MPEG2",
        configMINIMAL_STACK_SIZE,
        mpeg_struct,
        1,
        NULL);

    // xTaskPREMCreate(
    //     countnegative_task,
    //     "Count negative",
    //     configMINIMAL_STACK_SIZE,
    //     countnegative_struct,
    //     2,
    //     NULL);

    xTaskPREMCreate(
        weightavg_task,
        "Weight average",
        configMINIMAL_STACK_SIZE,
        weigthavg_struct,
        3,
        NULL);

    vInitPREM();
    vTaskStartScheduler();
}
