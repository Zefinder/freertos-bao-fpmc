#ifndef __PERIODIC_TASK_H__
#define __PERIODIC_TASK_H__

#include <FreeRTOS.h>
#include <task.h>

/*
 * Converts a frequency in hertz to a time in ticks. 
 */
#define pdFREQ_TO_TICKS(xFreq) ((TickType_t)((uint64_t)configTICK_RATE_HZ / (uint64_t)xFreq))

/*
 * Create a new periodic task and add it to the list of tasks that are ready to run.
 *
 * Repeats the given task every tickPeriod ticks. If the task runs "late", means that
 * it executes for a longer period than expected (non schedulable), the periodic task
 * will wait the next period to start and won't try to fit in the remaining slot time
 * (see the drawing just a little after). How to overcome this? Increase the period,
 * increase the task priority, reduce memory usage (if using memory scheduling)...
 *  ┌───────┐   ┌─────
 *  │       │   │       ... Normal execution
 * ─┘       └───┘
 * ￪            ￪
 *
 *  ┌─────| |─────┐        ┌────
 *  │             │        │      ... Late execution (preemption)
 * ─┘             └        │
 * ￪           ￪           ￪
 *
 * There must be no while(1) or for(;;) inside the task to repeat. Since it is an
 * already periodic task, it will repeat until deletion.
 *
 * Note that you can use pdMS_TO_TICKS to transform a time into ticks and
 * pdFREQ_TO_TICKS to transform a frequency into ticks
 *
 * This is simply creating a task using xTaskCreate but makes it periodic.
 */
BaseType_t xTaskPeriodicCreate(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const configSTACK_DEPTH_TYPE uxStackDepth,
                               const TickType_t tickPeriod,
                               void *const pvParameters,
                               UBaseType_t uxPriority,
                               TaskHandle_t *const pxCreatedTask);

#endif