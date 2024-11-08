#include <periodic_task.h>
#include <FreeRTOS.h>
#include <task.h>
#include <generic_timer.h>

struct prv_periodic_arguments
{
    TaskFunction_t pxTaskCode;
    uint64_t systick_period;
    uint8_t task_id;
    void *pvParameters;
};

uint8_t periodic_task_number = 0;
TickType_t starting_tick = 0;
TickType_t *last_period_start; // Array containing all last periods
uint64_t sysfreq = 0;

/* The periodic task */
void vPeriodicTask(void *pvParameters)
{
    // Initialising tick count for every periodic task so they have their start time
    if (starting_tick == 0)
    {
        starting_tick = generic_timer_read_counter();
    }

    // Getting struct from task parameters and extract task function, period and parameters
    struct prv_periodic_arguments *periodic_arguments = (struct prv_periodic_arguments *)pvParameters;

    TaskFunction_t pxTaskCode = periodic_arguments->pxTaskCode;
    uint8_t task_id = periodic_arguments->task_id;
    void *pvTaskParameters = periodic_arguments->pvParameters;

    const uint64_t tickPeriod = periodic_arguments->systick_period;

    // Don't forget to free the struct
    vPortFree(pvParameters);

    // Initialise the last_period_start variable with the starting ticks.
    last_period_start[task_id] = starting_tick;

    while (1)
    {
        printf("task_id: %d\n", periodic_arguments->task_id);

        // Execute task
        pxTaskCode(pvTaskParameters);

        // If period is 0, then no waiting
        if (tickPeriod != 0)
        {
            // Get current tick count
            uint64_t currentTick = generic_timer_read_counter();
            uint64_t next_period_start = last_period_start[task_id] + tickPeriod;

            // If a new cycle didn't start yet, then delay
            if (currentTick < next_period_start)
            {
                printf("%lld\n", pdSYSTICK_TO_TICKS(sysfreq, next_period_start - currentTick));

                // Put the task into WAIT state, so compute the number of FreeRTOS cycles to wait
                vTaskDelay(pdSYSTICK_TO_TICKS(sysfreq, next_period_start - currentTick));

                // The pdSYSTICK_TO_TICKS gives an underestimation of the number of cycles to wait...
                // So wait for the remaining number of systicks 
                while (currentTick < next_period_start)
                {
                    currentTick = generic_timer_read_counter();
                }
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
            last_period_start[task_id] = next_period_start;
        }
        else 
        {
            last_period_start[task_id] = generic_timer_read_counter();
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
    if (sysfreq == 0) {
        sysfreq = generic_timer_get_freq();
    }

    // Create and fill struct
    struct prv_periodic_arguments *periodic_arguments_ptr = (struct prv_periodic_arguments *)pvPortMalloc(sizeof(struct prv_periodic_arguments));

    periodic_arguments_ptr->pxTaskCode = pxTaskCode;
    printf("Period in ticks: %d\n", periodic_arguments.tickPeriod);
    printf("Period in µs: %d\n", pdTICKS_TO_US(periodic_arguments.tickPeriod));
    printf("Period in systicks: %lld\n\n", pdTICKS_TO_SYSTICK(sysfreq, periodic_arguments.tickPeriod));
    periodic_arguments_ptr->systick_period = pdTICKS_TO_SYSTICK(sysfreq, periodic_arguments.tickPeriod);
    periodic_arguments_ptr->task_id = periodic_task_number++;
    periodic_arguments_ptr->pvParameters = periodic_arguments.pvParameters;

    BaseType_t creationAck = xTaskCreate(vPeriodicTask,
                                         pcName,
                                         uxStackDepth,
                                         (void *)periodic_arguments_ptr,
                                         uxPriority,
                                         pxCreatedTask);

    return creationAck;
}

TickType_t get_last_period_start(uint8_t task_id)
{
    return last_period_start[task_id];
}

void vInitPeriodic(void)
{
    printf("Task number: %d\n", periodic_task_number);

    // Create the array of last period times
    last_period_start = (uint64_t *)pvPortMalloc(sizeof(uint64_t) * periodic_task_number);
}