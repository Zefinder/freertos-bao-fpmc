#include <FreeRTOS.h>
#include <task.h>
#include <periodic_task.h>

struct periodic_arguments
{
    TaskFunction_t pxTaskCode;
    TickType_t tickPeriod;
    void *pvParameters; 
};

void vPeriodicTask(void *pvParameters)
{
    // Getting struct from task parameters and extract task function, period and parameters
    struct periodic_arguments *periodic_arguments = (struct periodic_arguments *)pvParameters;

    TaskFunction_t pxTaskCode = periodic_arguments->pxTaskCode;
    void *pvTaskParameters = periodic_arguments->pvParameters;

    TickType_t xLastWakeTime;
    const TickType_t tickPeriod = periodic_arguments->tickPeriod;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Get current tick count
        TickType_t currentTick = xTaskGetTickCount();

        // If there are some ticks to wait, it means that we are not sync
        if (currentTick - xLastWakeTime < tickPeriod)
        {
            // Wait for the next cycle.
            vTaskDelayUntil(&xLastWakeTime, tickPeriod);
        }
        else if (currentTick - xLastWakeTime > tickPeriod)
        {
            // We compute next sync period and wait until there to restart
            while (currentTick - xLastWakeTime > tickPeriod)
            {
                printf("a");
                xLastWakeTime += tickPeriod;
            }
            vTaskDelayUntil(&xLastWakeTime, tickPeriod);
        }

        // Execute task here
        pxTaskCode(pvTaskParameters);
    }
}

BaseType_t xTaskPeriodicCreate(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const configSTACK_DEPTH_TYPE uxStackDepth,
                               const TickType_t tickPeriod,
                               void *const pvParameters,
                               UBaseType_t uxPriority,
                               TaskHandle_t *const pxCreatedTask)
{
    // Create struct
    struct periodic_arguments periodic_arguments = {.pxTaskCode = pxTaskCode, .tickPeriod = tickPeriod, .pvParameters = pvParameters};
    struct periodic_arguments *periodic_arguments_ptr = (struct periodic_arguments *)pvPortMalloc(sizeof(struct periodic_arguments));
    *periodic_arguments_ptr = periodic_arguments;

    xTaskCreate(
        vPeriodicTask,
        pcName,
        uxStackDepth,
        (void *)periodic_arguments_ptr,
        uxPriority,
        pxCreatedTask);
}