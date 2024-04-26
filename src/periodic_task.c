#include <periodic_task.h>
#include <FreeRTOS.h>
#include <task.h>

struct prv_periodic_arguments
{
    TaskFunction_t pxTaskCode;
    TickType_t tickPeriod;
    void *pvParameters;
};

void vPeriodicTask(void *pvParameters)
{
    // Getting struct from task parameters and extract task function, period and parameters
    struct prv_periodic_arguments *periodic_arguments = (struct prv_periodic_arguments *)pvParameters;

    TaskFunction_t pxTaskCode = periodic_arguments->pxTaskCode;
    void *pvTaskParameters = periodic_arguments->pvParameters;

    TickType_t xLastWakeTime;
    const TickType_t tickPeriod = periodic_arguments->tickPeriod;

    // Don't forget to free the struct
    vPortFree(pvParameters);

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
            xTaskDelayUntil(&xLastWakeTime, tickPeriod);
        }
        else if (currentTick - xLastWakeTime > tickPeriod)
        {
            // We compute next sync period and wait until there to restart
            while (currentTick - xLastWakeTime > tickPeriod)
            {
                TickType_t diff = currentTick - xLastWakeTime;
                xLastWakeTime += tickPeriod;
            }
            xTaskDelayUntil(&xLastWakeTime, tickPeriod);
        }

        // Execute task here
        pxTaskCode(pvTaskParameters);
    }
}

BaseType_t xTaskPeriodicCreate(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const configSTACK_DEPTH_TYPE uxStackDepth,
                               const struct periodic_arguments periodic_arguments,
                               UBaseType_t uxPriority,
                               TaskHandle_t *const pxCreatedTask)
{
    // Create and fill struct
    struct prv_periodic_arguments *periodic_arguments_ptr = (struct prv_periodic_arguments *)pvPortMalloc(sizeof(struct prv_periodic_arguments));

    periodic_arguments_ptr->pxTaskCode = pxTaskCode;
    periodic_arguments_ptr->tickPeriod = periodic_arguments.tickPeriod;
    periodic_arguments_ptr->pvParameters = periodic_arguments.pvParameters;

    BaseType_t creationAck = xTaskCreate(vPeriodicTask,
                                         pcName,
                                         uxStackDepth,
                                         (void *)periodic_arguments_ptr,
                                         uxPriority,
                                         pxCreatedTask);

    return creationAck;
}