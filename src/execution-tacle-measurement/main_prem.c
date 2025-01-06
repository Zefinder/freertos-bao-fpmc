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

// To use with 1 processor for WCET measure, 4 processors for taskset measurement

// First one does not count
#define NUMBER_OF_TESTS 30001

struct task_parameters
{
    benchmark_t task_function;
    uint64_t counter;
};

extern uint64_t cpu_priority;
uint64_t ntest_mpeg = 0;
uint64_t ntest_countnegative = 0;
uint64_t ntest_bubblesort = 0;
uint64_t ntest_weightavg = 0;
uint64_t cpu_number = 0;

void weightavg_task(void *pvParameters)
{
    // Will perform sum += (array1[i] - array[2]) / 2
    // 460kB of appdata loaded in the cache!
    uint32_t offset_1 = 0;
    uint32_t offset_2 = 230 * 1024;

    volatile uint64_t array_sum = 0;
    volatile uint64_t array_avg = 0;
    for (int index = 0; index < offset_2; index++)
    {
        array_sum += (appdata[index + offset_1] - appdata[index + offset_2]) / 2;
    }

    array_avg = array_sum / offset_2;
}

void master_task(void *pvParameters)
{
    struct task_parameters *task_parameters = (struct task_parameters *)pvParameters;
    task_parameters->task_function(NULL);

    if (++task_parameters->counter == NUMBER_OF_TESTS)
    {
        // When this is the last test, then print results but continue to run (if this stops, it won't be a good example)
        askDisplayResults();
    }
}

struct task_parameters mpeg, countnegative, bubblesort, weightavg;

void main_app(void)
{
    // start_benchmark();
    // Each execution last 60ms * number of tests
    /*
        Tasks mpeg2_task and countnegative_task from TACleBench
    */

    // If core 0 then init normal tasks
    cpu_number = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    // Init countnegative matrix and binarysearch array
    countnegative_init();
    bubblesort_init();

    uint8_t *mpeg_data = get_mpeg2_oldorgframe();
    uint8_t *countnegative_data = get_countnegative_array();
    uint8_t *binarysearch_data = get_bubblesort_array();

    // Create for all processors the same tasks, all will measure
    mpeg.task_function = mpeg2_main;
    mpeg.counter = ntest_mpeg;
    struct premtask_parameters mpeg_struct = {.tickPeriod = pdMS_TO_TICKS(45), .data_size = 88 kB * 4, .data = mpeg_data, .wcet = 4755907, .pvParameters = &mpeg};

    countnegative.task_function = countnegative_main;
    countnegative.counter = ntest_countnegative;
    struct premtask_parameters countnegative_struct = {.tickPeriod = pdMS_TO_TICKS(55), .data_size = CN_MAXSIZE * CN_MAXSIZE, .data = countnegative_data, .wcet = 6083944, .pvParameters = &countnegative};

    bubblesort.task_function = bubblesort_main;
    bubblesort.counter = ntest_bubblesort;
    struct premtask_parameters bubblesort_struct = {.tickPeriod = pdMS_TO_TICKS(60), .data_size = BS_MAXSIZE * 8, .data = binarysearch_data, .wcet = 26362129, .pvParameters = &bubblesort};

    weightavg.task_function = weightavg_task;
    weightavg.counter = ntest_weightavg;
    struct premtask_parameters weightavg_struct = {.tickPeriod = pdMS_TO_TICKS(50), .data_size = 460 kB, .data = appdata, .wcet = 5861944, .pvParameters = &weightavg};

    xTaskPREMCreate(
        master_task,
        "MPEG2",
        1 MB >> sizeof(StackType_t),
        mpeg_struct,
        4,
        NULL);

    xTaskPREMCreate(
        master_task,
        "Weight average",
        1 MB >> sizeof(StackType_t),
        weightavg_struct,
        3,
        NULL);

    xTaskPREMCreate(
        master_task,
        "Count negative",
        1 MB >> sizeof(StackType_t),
        countnegative_struct,
        2,
        NULL);

    xTaskPREMCreate(
        master_task,
        "Bubble sort",
        1 MB >> sizeof(StackType_t),
        bubblesort_struct,
        1,
        NULL);


    vInitPREM();
    vTaskStartScheduler();
}
