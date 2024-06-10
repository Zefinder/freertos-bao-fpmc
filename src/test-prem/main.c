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

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

// Change location for appdata
#define DATA_SIZE 448 kB

struct task_data
{
    uint32_t cpu_id;
    uint32_t task_id;
};

void vTaskHypercall(void *pvParameters)
{
    struct task_data *task_data = (struct task_data *)pvParameters;

    // Just computes the sum and prints the result
    uint64_t sum = 0;
    for (int i = 0; i < DATA_SIZE; i++)
    {
        sum += appdata[i];
    }

    printf("Sum of appdata for task %d on CPU %d: %ld\n", task_data->task_id, task_data->cpu_id, sum);
    vTaskDelay(pdMS_TO_TICKS(300));
}

void vTaskInterference()
{
    // Here nothing, just interferences, just prefetch 512kB and return
}

struct task_data task1_data, task2_data;
void main_app(void)
{
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    // Init and create PREM task
    vInitPREM();
    if (cpu_id == 1)
    {
        task1_data.cpu_id = cpu_id;
        task1_data.task_id = 1;
        task2_data.cpu_id = cpu_id;
        task2_data.task_id = 2;

        TickType_t waiting_ticks = pdMS_TO_TICKS(1000);

        struct premtask_parameters premtask1_parameters = {.tickPeriod = waiting_ticks, .data = appdata, .data_size = DATA_SIZE, .pvParameters = (void *)&task1_data};
        struct premtask_parameters premtask2_parameters = {.tickPeriod = 3 * waiting_ticks, .data = appdata, .data_size = DATA_SIZE, .pvParameters = (void *)&task2_data};
        xTaskPREMCreate(
            vTaskHypercall,
            "TestPREMTask1",
            configMINIMAL_STACK_SIZE,
            premtask1_parameters,
            tskIDLE_PRIORITY + 2,
            &(xTaskHandler));

        xTaskPREMCreate(
            vTaskHypercall,
            "TestPREMTask2",
            configMINIMAL_STACK_SIZE,
            premtask2_parameters,
            tskIDLE_PRIORITY + 1,
            &(xTaskHandler));
    }
    else
    {
        // Just interferences, the goal is to provoke destiny and try to show the bonus message (only during debug) that we ask too frequently
        TickType_t waiting_ticks = pdMS_TO_TICKS(5);
        struct premtask_parameters premtask1_parameters = {.tickPeriod = waiting_ticks, .data = appdata, .data_size = MAX_DATA_SIZE, .pvParameters = NULL};
        xTaskPREMCreate(vTaskInterference,
                        "Blocked",
                        configMINIMAL_STACK_SIZE,
                        premtask1_parameters,
                        tskIDLE_PRIORITY + 4,
                        NULL);
    }
    vTaskStartScheduler();
}