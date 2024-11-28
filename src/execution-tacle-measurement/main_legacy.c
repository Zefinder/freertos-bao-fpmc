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

#define NUMBER_OF_TESTS 100000

struct task_parameters
{
    char *name;
    benchmark_t task_function;
    uint8_t *data;
    uint64_t data_size;
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

void prefetch_task(void *arg)
{
    struct task_parameters *task_parameters = (struct task_parameters *)(arg);

    prefetch_data((uint64_t)task_parameters->data, (uint64_t)task_parameters->data_size);
    task_parameters->task_function(NULL);
    clear_L2_cache((uint64_t)task_parameters->data, (uint64_t)task_parameters->data_size);
}

void master_task(void *pvParameters)
{
    struct task_parameters *task_parameters = (struct task_parameters *)(pvParameters);

    // Init benchmark
    init_benchmark(task_parameters->name, MEASURE_NANO, 0);

    // Clear cache
    clear_L2_cache((uint64_t)task_parameters->data, (uint64_t)task_parameters->data_size);

    // uint32_t print_counter = 0;
    // Run test
    for (int i = 0; i < NUMBER_OF_TESTS; i++)
    {
        // Run benchmark
        run_benchmark(prefetch_task, task_parameters);
    }

    // End benchmarks
    print_benchmark_results();

    // Free task parameters
    vPortFree(task_parameters);
    vTaskDelete(NULL);
}

void interfering_task(void *pvParameters)
{
    // Nothing here, just nothing...
}

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

        struct task_parameters mpeg_struct = {.name = "_mpeg2", .task_function = mpeg2_main, .data = mpeg_data, .data_size = 88 kB * 4};
        struct task_parameters countnegative_struct = {.name = "_countnegative", .task_function = countnegative_main, .data = countnegative_data, .data_size = CN_MAXSIZE * CN_MAXSIZE};
        struct task_parameters bubblesort_struct = {.name = "_bubblesort", .task_function = bubblesort_main, .data = binarysearch_data, .data_size = BS_MAXSIZE * 8};
        struct task_parameters weigthavg_struct = {.name = "_weightavg", .task_function = weightavg_task, .data = appdata, .data_size = 460 kB};

        // Malloc task parameters
        struct task_parameters *pv_mpeg_struct = (struct task_parameters *)pvPortMalloc(sizeof(struct task_parameters));
        *pv_mpeg_struct = mpeg_struct;

        struct task_parameters *pv_countnegative_struct = (struct task_parameters *)pvPortMalloc(sizeof(struct task_parameters));
        *pv_countnegative_struct = countnegative_struct;

        struct task_parameters *pv_bubblesort_struct = (struct task_parameters *)pvPortMalloc(sizeof(struct task_parameters));
        *pv_bubblesort_struct = bubblesort_struct;

        struct task_parameters *pv_weigthavg_struct = (struct task_parameters *)pvPortMalloc(sizeof(struct task_parameters));
        *pv_weigthavg_struct = weigthavg_struct;

        xTaskCreate(master_task, "MPEG2", configMINIMAL_STACK_SIZE, pv_mpeg_struct, 4, NULL);
        xTaskCreate(master_task, "Count negative", configMINIMAL_STACK_SIZE, pv_countnegative_struct, 3, NULL);
        xTaskCreate(master_task, "Bubble sort", configMINIMAL_STACK_SIZE, pv_bubblesort_struct, 2, NULL);
        xTaskCreate(master_task, "Weight average", configMINIMAL_STACK_SIZE, pv_weigthavg_struct, 1, NULL);
    }
    else
    {
        struct task_parameters interference_struct = {.name = "", .task_function = interfering_task, .data = appdata, .data_size = MAX_DATA_SIZE};
        struct task_parameters *pv_interference_struct = (struct task_parameters *)pvPortMalloc(sizeof(struct task_parameters));
        *pv_interference_struct = interference_struct;

        xTaskCreate(prefetch_task, "Interference", configMINIMAL_STACK_SIZE, pv_interference_struct, 1, NULL);
    }
    
    vTaskStartScheduler();
}
