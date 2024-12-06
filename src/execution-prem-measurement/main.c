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

#define NUMBER_OF_TESTS 100001
#define DATA_SIZE MAX_DATA_SIZE
#define MIN_DATA_SIZE 1 kB
#define DATA_SIZE_INCREMENT 1 kB

uint64_t data_size = 0;
uint64_t test_number = 0;
uint8_t init = 0;

void task(void *arg)
{
    // Nothin, just vibin
    if (++test_number == NUMBER_OF_TESTS)
    {
        vTaskPREMDelete();
    }
}

void vMasterTask(void *pvParameters)
{
    start_benchmark();
    for (data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
    {
        test_number = 0; 

        // Create a PREM task with the good size
        struct premtask_parameters prem_struct = {.tickPeriod = 0, .data_size = data_size, .data = appdata, .wcet = 0, .pvParameters = NULL};

        // Launch it and put a higher priority
        xTaskPREMCreate(
            task,
            "task",
            configMINIMAL_STACK_SIZE,
            prem_struct,
            2,
            NULL);

        // If not initialised, init PREM
        if (!init)
        {
            init = 1;
            vInitPREM();
        }

        // Wait to force a reschedule
        vTaskDelay(0);

        // When back here, the task has been deleted, go to next size
    }

    end_benchmark();
    vTaskDelete(NULL);
}

void main_app(void)
{
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
    if (cpu_id == 0)
    {
        xTaskCreate(
            vMasterTask,
            "MasterTask",
            configMINIMAL_STACK_SIZE,
            NULL,
            1,
            &(xTaskHandler));
    }
    else
    {
        struct premtask_parameters prem_struct = {.tickPeriod = 0, .data_size = MAX_DATA_SIZE, .data = appdata, .wcet = 0, .pvParameters = NULL};
        xTaskPREMCreate(
            task,
            "interference",
            configMINIMAL_STACK_SIZE,
            prem_struct,
            1,
            NULL);

        vInitPREM();
    }

    vTaskStartScheduler();
}