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

// To use with 1 processor for WCET measure

#define NUMBER_OF_TESTS 100001

struct task_parameters
{
    char *name;
    benchmark_t task_function;
    uint64_t counter;
};

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
    struct task_parameters *task_parameters = (struct task_parameters *)(pvParameters);
    if (task_parameters->counter == 0)
    {
        printf("\n# Execution of %s\n", task_parameters->name);
    }

    task_parameters->task_function(NULL);

    if (++task_parameters->counter == NUMBER_OF_TESTS)
    {
        vTaskPREMDelete();
    }
}

uint64_t mpeg_counter = 0;
uint64_t countnegative_counter = 0;
uint64_t bubblesort_counter = 0;
uint64_t weigthavg_counter = 0;

struct task_parameters mpeg_struct = {.name = "_mpeg2", .task_function = mpeg2_main};
struct task_parameters countnegative_struct = {.name = "_countnegative", .task_function = countnegative_main};
struct task_parameters bubblesort_struct = {.name = "_bubblesort", .task_function = bubblesort_main};
struct task_parameters weightavg_struct = {.name = "_weightavg", .task_function = weightavg_task};
void main_app(void)
{
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
    if (cpu_id == 0)
    {
        start_benchmark();
        /*
            Tasks mpeg2_task and countnegative_task from TACleBench
        */
        // Init countnegative matrix and binarysearch array
        countnegative_init();
        bubblesort_init();

        uint8_t *mpeg_data = get_mpeg2_oldorgframe();
        uint8_t *countnegative_data = get_countnegative_array();
        uint8_t *binarysearch_data = get_bubblesort_array();

        mpeg_struct.counter = mpeg_counter;
        countnegative_struct.counter = countnegative_counter;
        bubblesort_struct.counter = bubblesort_counter;
        weightavg_struct.counter = weigthavg_counter;

        struct premtask_parameters mpeg_prem = {.tickPeriod = 0, .data_size = 88 kB * 4, .data = mpeg_data, .wcet = 0, .pvParameters = &mpeg_struct};
        struct premtask_parameters countnegative_prem = {.tickPeriod = 0, .data_size = CN_MAXSIZE * CN_MAXSIZE, .data = countnegative_data, .wcet = 0, .pvParameters = &countnegative_struct};
        struct premtask_parameters bubblesort_prem = {.tickPeriod = 0, .data_size = BS_MAXSIZE * 8, .data = binarysearch_data, .wcet = 0, .pvParameters = &bubblesort_struct};
        struct premtask_parameters weightavg_prem = {.tickPeriod = 0, .data_size = 460 kB, .data = appdata, .wcet = 0, .pvParameters = &weightavg_struct};

        xTaskPREMCreate(
            master_task,
            "MPEG2",
            1 MB >> sizeof(StackType_t),
            mpeg_prem,
            4,
            NULL);

        xTaskPREMCreate(
            master_task,
            "countnegative",
            1 MB >> sizeof(StackType_t),
            countnegative_prem,
            3,
            NULL);

        xTaskPREMCreate(
            master_task,
            "bubblesort",
            1 MB >> sizeof(StackType_t),
            bubblesort_prem,
            2,
            NULL);

        xTaskPREMCreate(
            master_task,
            "weightavg",
            1 MB >> sizeof(StackType_t),
            weightavg_prem,
            1,
            NULL);
    }

    vInitPREM();

    vTaskStartScheduler();
}
