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

// Request the default IPI and memory request wait
#ifndef MEASURE_RESPONSE_TIME
    #error MEASURE_RESPONSE_TIME must be defined with value y for this test (make all ... MEASURE_RESPONSE_TIME=y)
#endif
// #ifndef DEFAULT_IPI_HANDLERS
//     #error DEFAULT_IPI must be defined with value y for this test (make all ... DEFAULT_IPI=y)
// #endif
// #ifndef MEMORY_REQUEST_WAIT
//     #error MEMORY_REQUEST_WAIT must be defined for this benchmark, (make all ... MEMORY_REQUEST_WAIT=y)
// #endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 10000
#define DATA_SIZE_0 100 kB
#define DATA_SIZE_INTERFERENCE 40 kB
#define MAX_PRIO 6

uint64_t ntest= 0;

void mpeg2_task(void *pvParameters){
    mpeg2_main(NULL);

    if (++ntest == NUMBER_OF_TESTS) {
        print_benchmark_results();
        end_benchmark();
        while(1);
    }
}

void main_app(void)
{
    start_benchmark();
    init_benchmark("_mpeg2", 1);

    /* Two tasks from TACleBench:
     * - MPEG2 (? Hz)
     * - Count negative (? Hz)
     * One custom task: 
     * - Weighted average of 2 100kB arrays (? Hz)
     */

    // xTaskCreate(mpeg2_task, "MPEG2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    uint8_t *data = get_mpeg2_oldorgframe();
    struct premtask_parameters parameters1 = {.tickPeriod=0, .data_size=90112, .data=data, .wcet=0, .pvParameters=NULL};

    xTaskPREMCreate(
        mpeg2_task,
        "Master task1",
        configMINIMAL_STACK_SIZE,
        parameters1,
        0,
        NULL
    );

    // xTaskPREMCreate(
    //     vMasterTask,
    //     "Master task1",
    //     configMINIMAL_STACK_SIZE,
    //     parameters1,
    //     1,
    //     NULL
    // );

    vTaskStartScheduler();
}
