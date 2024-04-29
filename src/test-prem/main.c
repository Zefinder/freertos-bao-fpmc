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

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

// Change location for appdata
#define DATA_SIZE 448 * 1024
uint8_t appdata[DATA_SIZE] = {[0 ... DATA_SIZE - 1] = 1};

void vTask(void *pvParameters)
{
    // Just computes the sum and prints the result
    int sum = 0;
    for (int i = 0; i < DATA_SIZE; i++)
    {
        sum += appdata[i];
    }

    printf("Sum of appdata for CPU%d: %d\n", hypercall(HC_GET_CPU_ID, 0, 0, 0), sum);
}

void main_app(void)
{
    struct premtask_parameters premtask_parameters = {.tickPeriod = pdFREQ_TO_TICKS(100), .data = appdata, .data_size = DATA_SIZE, .pvParameters = NULL};

    // Init and create PREM task
    vInitPREM();
    xTaskPREMCreate(
        vTask,
        "TestPREMTask",
        configMINIMAL_STACK_SIZE,
        premtask_parameters,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}