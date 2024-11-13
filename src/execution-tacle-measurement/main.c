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

#define NUMBER_OF_TESTS 100001 // Skip the first one, its release time is not relevant in that case

uint64_t ntest_mpeg = 0;
uint64_t ntest_countnegative = 0;
uint64_t ntest_bubblesort = 0;
uint64_t ntest_weightavg = 0;

void mpeg2_task(void *pvParameters)
{
    // Execute function
    mpeg2_main();

    // When executed everything, delete itself
    if (++ntest_mpeg == NUMBER_OF_TESTS)
    {
        vTaskPeriodicDelete();
    }
}

void countnegative_task(void *pvParameters)
{
    // Execute function
    countnegative_main();

    // When executed everything, delete itself
    if (++ntest_countnegative == NUMBER_OF_TESTS)
    {
        vTaskPeriodicDelete();
    }
}

void bubblesort_task(void *pvParameters)
{
    // Execute function
    bubblesort_main();

    // When executed everything, delete itself
    if (++ntest_bubblesort == NUMBER_OF_TESTS)
    {
        vTaskPeriodicDelete();
    }
}

void weightavg_task(void *pvParameters)
{
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

    // When executed everything, delete itself
    if (++ntest_weightavg >= NUMBER_OF_TESTS)
    {
        vTaskPeriodicDelete();
    }
}

void main_app(void)
{

    /* Two tasks from TACleBench:
     * - MPEG2
     * - Count negative
     * One custom task:
     * - Weighted average of 2 100kB arrays
     */

    // Init countnegative matrix and binarysearch array
    countnegative_init();
    bubblesort_init();

    uint8_t *mpeg_data = get_mpeg2_oldorgframe();
    uint8_t *countnegative_data = get_countnegative_array();
    uint8_t *binarysearch_data = get_bubblesort_array();

    struct premtask_parameters mpeg_struct = {.tickPeriod = 0, .data_size = 90112, .data = mpeg_data, .wcet = 0, .pvParameters = NULL};
    struct premtask_parameters countnegative_struct = {.tickPeriod = 0, .data_size = 147456, .data = countnegative_data, .wcet = 0, .pvParameters = NULL};
    struct premtask_parameters bubblesort_struct = {.tickPeriod = 0, .data_size = 204800, .data = binarysearch_data, .wcet = 0, .pvParameters = NULL};
    struct premtask_parameters weigthavg_struct = {.tickPeriod = 0, .data_size = 204800, .data = appdata, .wcet = 0, .pvParameters = NULL};

    xTaskPREMCreate(
        mpeg2_task,
        "MPEG2",
        configMINIMAL_STACK_SIZE,
        mpeg_struct,
        1,
        NULL);

    xTaskPREMCreate(
        countnegative_task,
        "Count negative",
        configMINIMAL_STACK_SIZE,
        countnegative_struct,
        2,
        NULL);

    xTaskPREMCreate(
        bubblesort_task,
        "Bubble sort",
        configMINIMAL_STACK_SIZE,
        bubblesort_struct,
        3,
        NULL);

    xTaskPREMCreate(
        weightavg_task,
        "Weight average",
        configMINIMAL_STACK_SIZE,
        weigthavg_struct,
        4,
        NULL);

    vInitPREM();
    vTaskStartScheduler();
}
