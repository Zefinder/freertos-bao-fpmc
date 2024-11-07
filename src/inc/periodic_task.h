#ifndef __PERIODIC_TASK_H__
#define __PERIODIC_TASK_H__

#include <FreeRTOS.h>
#include <task.h>

/*
 * Structure that contains important data for the periodic task: the task's period
 * (in tick) and the argument(s) of the task. You do not need to malloc it as it will
 * be malloc'ed and freed in xTaskPeriodicCreate.
 *
 * Note that if the period is 0, then no waiting and this is equivalent to just having
 * a normal task. This can be useful for non periodic tasks that can be executed at any
 * time
 */
struct periodic_arguments
{
    TickType_t tickPeriod;
    void *pvParameters;
};

/*
 * Converts a frequency in hertz to a time in ticks.
 */
#define pdFREQ_TO_TICKS(xFreq) ((TickType_t)((uint64_t)configTICK_RATE_HZ / (uint64_t)xFreq))

/*
 * Create a new periodic task and add it to the list of tasks that are ready to run.
 *
 * Repeats the given task every tickPeriod ticks. If the task runs "late", means that
 * it executes for a longer period than expected (non schedulable), the periodic task
 * will try to execute it as soon as possible...
 *
 *  ┌───────┐   ┌─────
 *  │       │   │       ... Normal execution
 * ─┘       └───┘
 * ￪            ￪
 *
 *  ┌─────| |─────┐ ┌────
 *  │             │ │      ... Late execution (preemption)
 * ─┘             └─┘
 * ￪           ￪           ￪
 *
 * There must be no while(1) or for(;;) inside the task to repeat. Since it is an
 * already periodic task, it will repeat until deletion.
 *
 * Note that you can use pdMS_TO_TICKS to transform a time into ticks and
 * pdFREQ_TO_TICKS to transform a frequency into ticks
 *
 * This is simply creating a task using xTaskCreate but makes it periodic.
 *
 * TODO Notify somewhere that task was not schedulable (Enable tracing?)
 */
BaseType_t xTaskPeriodicCreate(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const configSTACK_DEPTH_TYPE uxStackDepth,
                               const struct periodic_arguments,
                               UBaseType_t uxPriority,
                               TaskHandle_t *const pxCreatedTask);

/*
 * Gets the last period start time with the priority
 */
TickType_t get_last_period_start(uint8_t priority);

/*
 * This function must be called to initialise periodic tasks. If you run
 * PREM tasks, it will be already called. Call before running tasks. RUN
 * ONLY ONCE!
 */
void vInitPeriodic(void);

#endif