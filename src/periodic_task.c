#include <periodic_task.h>
#include <FreeRTOS.h>
#include <task.h>

struct prv_periodic_arguments
{
    TaskFunction_t pxTaskCode;
    TickType_t tickPeriod;
    BaseType_t priority;
    void *pvParameters;
};

uint8_t periodic_task_number = 0;
TickType_t starting_tick = 0;
TickType_t *last_period_start; // Array containing all last periods (for measuring purpose)

/* The periodic task */
void vPeriodicTask(void *pvParameters)
{
    // Initialising tick count for every periodic task so they have their start time
    if (starting_tick == 0)
    {
        starting_tick = xTaskGetTickCount();
    }

    // Getting struct from task parameters and extract task function, period and parameters
    struct prv_periodic_arguments *periodic_arguments = (struct prv_periodic_arguments *)pvParameters;

    TaskFunction_t pxTaskCode = periodic_arguments->pxTaskCode;
    BaseType_t priority = periodic_arguments->priority;
    void *pvTaskParameters = periodic_arguments->pvParameters;

    const TickType_t tickPeriod = periodic_arguments->tickPeriod;

    // Don't forget to free the struct
    vPortFree(pvParameters);

    // Initialise the last_period_start variable with the starting ticks.
    last_period_start[priority] = starting_tick;

    while (1)
    {
        // Execute task
        pxTaskCode(pvTaskParameters);

        // If period is 0, then no waiting
        if (tickPeriod != 0)
        {
            // Get current tick count
            TickType_t currentTick = xTaskGetTickCount();
            TickType_t next_period_start = last_period_start[priority] + tickPeriod;

            // If a new cycle didn't start yet, then delay
            if (currentTick < next_period_start)
            {
                // Wait for the next cycle.
                vTaskDelay(next_period_start - currentTick);
            }

            // When the current cycle is greater than the period, it gets tricky...
            // It means that the task exceeded its dead line...
            // We search for its next period...
            else if (currentTick > next_period_start)
            {
                // We loop until the current tick is smaller than the next period
                while (currentTick > next_period_start)
                {
                    next_period_start += tickPeriod;
                }
            }

            // Update last period start cycle
            last_period_start[priority] = next_period_start;
        }
    }
}

BaseType_t xTaskPeriodicCreate(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const configSTACK_DEPTH_TYPE uxStackDepth,
                               const struct periodic_arguments periodic_arguments,
                               UBaseType_t uxPriority,
                               TaskHandle_t *const pxCreatedTask)
{
    // Increasing number of periodic tasks
    periodic_task_number += 1;

    // Create and fill struct
    struct prv_periodic_arguments *periodic_arguments_ptr = (struct prv_periodic_arguments *)pvPortMalloc(sizeof(struct prv_periodic_arguments));

    periodic_arguments_ptr->pxTaskCode = pxTaskCode;
    periodic_arguments_ptr->tickPeriod = periodic_arguments.tickPeriod;
    periodic_arguments_ptr->priority = uxPriority;
    periodic_arguments_ptr->pvParameters = periodic_arguments.pvParameters;

    BaseType_t creationAck = xTaskCreate(vPeriodicTask,
                                         pcName,
                                         uxStackDepth,
                                         (void *)periodic_arguments_ptr,
                                         uxPriority,
                                         pxCreatedTask);

    return creationAck;
}

TickType_t get_last_period_start(uint8_t priority)
{
    return last_period_start[priority];
}

void vInitPeriodic(void)
{
    // Create the array of last period times
    last_period_start = (TickType_t *)pvPortMalloc(sizeof(TickType_t) * periodic_task_number);
}