#ifndef __PREM_TASK_H__
#define __PREM_TASK_H__

#include <FreeRTOS.h>
#include <task.h>

/*
 * Structure that contains important data for the PREM task: the task's period (in
 * tick), the size of the data to prefetch (in bytes), the data array and the
 * argument(s) of the task. You do not need to malloc is as it will be malloc'ed
 * and freed in xTaskPREMCreate.
 */
struct premtask_parameters
{
    TickType_t tickPeriod;
    uint64_t data_size;
    void *data;
    void *pvParameters;
};

/*
 * Create a new PREM task and add it to the list of tasks that are ready to run.
 *
 * A PREM task is a periodic task (cf. periodic_task.h) that has a certain pattern:
 * Waiting -> Memory phase -> Computation phase -> Waiting
 *
 * This implementation of PREM task takes into account the Bao hypervisor (TODO) asking for
 * memory access before entering memory phase and releasing access after computation
 * phase. If the memory access is not granted or if a higher priority processor takes
 * the memory token, then the task goes in suspended mode. However that mode does not
 * always allow preemption! If a task of a lower priority began its memory phase, no
 * one can preempt it on the same processor and it will remain the executing task
 * until the end of its computation phase, even if a task of higher priority began
 * while fetching memory or computing. However, if a task of lower priority is
 * suspended and didn't begin its memory phase, then a task of higher priority can
 * take its place. (TODO)
 * 
 * Unless you want to break a lot of things, avoid using the vTaskSuspendAll and
 * xTaskResumeAll functions. If you really want to use them, please use vTaskSuspendAll 
 * first since during computation phase (the user task) the scheduler is suspended
 * and resuming it will probably prempt the task and won't do what you wanted to do...
 *
 * There must be no while(1) or for(;;) inside the task to repeat. Since it is an
 * already periodic task, it will repeat until deletion.
 *
 * Note that you can use pdMS_TO_TICKS to transform a time into ticks and
 * pdFREQ_TO_TICKS to transform a frequency into ticks.
 *
 * This is simply creating a task using xTaskPeriodicCreate but makes it PREM. Don't
 * forget to run the vInitPREM() before creating a PREM task
 */
BaseType_t xTaskPREMCreate(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const configSTACK_DEPTH_TYPE uxStackDepth,
                           const struct premtask_parameters premtask_parameters,
                           UBaseType_t uxPriority,
                           TaskHandle_t *const pxCreatedTask);

/*
 * Init PREM with this function. This is mandatory to do it once before starting PREM
 * tasks. This function will enable IPI and implement handlers for you! Running this
 * function more than once is useless... You can of course run PREM task without it, 
 * but do not exepect IPI and if your task gets suspended, it's for the rest of its
 * life
 */
void vInitPREM();

#endif
