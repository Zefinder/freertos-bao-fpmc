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

#define NUMBER_OF_TESTS 100001
#define DATA_SIZE MAX_DATA_SIZE
#define MIN_DATA_SIZE 1 kB
#define DATA_SIZE_INCREMENT 1 kB

uint64_t data_size = 0;
uint64_t test_number = 0;
uint8_t init = 0;

struct premtask_parameters measure_task_struct;
uint64_t current_data_size = MIN_DATA_SIZE;

void measuring_task(void *pvParameters)
{
    if (test_number == 0)
    {
        // This one does not count is the measurements so we can
        // display the size without fearing having bad benchmarks
        printf("\n# Execution for %d kB\n", BtkB(current_data_size));
    }

    // If test number ok, then return to 0 and new size
    if (++test_number == NUMBER_OF_TESTS)
    {
        test_number = 0;

        // If already max size, then delete
        if (current_data_size == DATA_SIZE)
        {
            vTaskPREMDelete();
        }
        else
        {
            askDisplayResults();
            current_data_size += DATA_SIZE_INCREMENT;
            askChangePrefetchSize(current_data_size);
        }
    }
}

void interference_task(void *pvParameters)
{
    // Nothin' here, just vibin'
}

void main_app(void)
{
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
    if (cpu_id == 0)
    {
        measure_task_struct.tickPeriod = 0;
        measure_task_struct.data_size = current_data_size;
        measure_task_struct.data = appdata;
        measure_task_struct.wcet = 0;
        measure_task_struct.pvParameters = NULL;

        xTaskPREMCreate(
            measuring_task,
            "measure task",
            configMINIMAL_STACK_SIZE,
            measure_task_struct,
            1,
            NULL);
    }
    else
    {
        struct premtask_parameters prem_struct = {.tickPeriod = 0, .data_size = MAX_DATA_SIZE, .data = appdata, .wcet = 0, .pvParameters = NULL};
        xTaskPREMCreate(
            interference_task,
            "interference",
            configMINIMAL_STACK_SIZE,
            prem_struct,
            1,
            NULL);
    }

    vInitPREM();
    vTaskStartScheduler();
}