#ifndef __PREM_TASK_H__
#define __PREM_TASK_H__

#include <FreeRTOS.h>
#include <task.h>

/*
 * Structure that contains important data for the PREM task:
 * - TickType_t tickPeriod: the task's period (in FreeRTOS ticks). A tick
 * period of 0 means that there is no waiting time and that the task ask directly
 * for a new memory period
 * - uint64_t data_size: the size of the data to prefetch (in bytes)
 * - void *data: the data array to prefetch. It will internally be considered as
 * a uint8_t array of size [data_size]
 * - uint64_t wcet: Worst case execution time. Time must be in NANOSECONDS, conversion
 * to systicks will be done internally.
 * - void *pvParameters: the argument(s) of the task.
 *
 * Note that you do not need to malloc the struct is as it will be malloc'ed
 * and freed in xTaskPREMCreate.
 */
struct premtask_parameters
{
    TickType_t tickPeriod;
    uint64_t data_size;
    void *data;
    uint64_t wcet;
    void *pvParameters;
};

/* Answer of hypervisor after a memory request */
union memory_request_answer
{
    struct
    {
        uint64_t ack : 1;  // 0 = no, 1 = yes
        uint64_t ttw : 63; // Time to wait in case of no (0 = no waiting, just not prio)
    };
    uint64_t raw;
};

/*
 * Delays the PREM task of [waitingTicks] ticks. (Systicks ticks)
 *
 * This implementation of vTaskDelay is here for a reason. As specified in the
 * documentation of xTaskPREMCreate, the scheduler is off when running the
 * calculation phase thus using vTaskDelay will do nothing. Preemption is
 * enabled to allow switching when a lower priority task is waiting for a
 * memory access and a higher priority task is ready. If the scheduler was
 * still up when using vTaskDelay, even if the priority of the task was change
 * to the max during its execution, there would be a context switch... This is
 * not what we want!
 *
 * This uses the generic timer to block the task and not provoke a context switch.
 */
void vTaskPREMDelay(TickType_t waitingTicks);

/*
 * Create a new PREM task and add it to the list of tasks that are ready to run.
 *
 * A PREM task is a periodic task (cf. periodic_task.h) that has a certain pattern:
 * Waiting -> Memory phase -> Computation phase -> Waiting
 *
 * This implementation of PREM task takes into account the Bao hypervisor asking for
 * memory access before entering memory phase and releasing access after computation
 * phase. If the memory access is not granted or if a higher priority processor takes
 * the memory token, then the task goes in suspended mode. However that mode does not
 * always allow preemption! If a task of a lower priority began its memory phase, no
 * one can preempt it on the same processor and it will remain the executing task
 * until the end of its computation phase, even if a task of higher priority began
 * while fetching memory or computing. However, if a task of lower priority is
 * suspended and didn't begin its memory phase, then a task of higher priority can
 * take its place.
 *
 * Unless you want to break a lot of things, avoid using the vTaskSuspendAll and
 * xTaskResumeAll functions. If you really want to use them, please use vTaskSuspendAll
 * first since during computation phase (the user task) the scheduler is suspended
 * and resuming it will probably prempt the task and won't do what you wanted to do...
 *
 * As the scheduler is suspended, using vTaskDelay will do... nothing! Because preemption
 * is enabled, vTaskDelay would cause a reschedule if the scheduler was enabled, even
 * if the task had the highest priority. One idea was to set the running task to the
 * highest priority when running so you never get preempted, but if delaying causes
 * preemption then this is useless. If you want to put delay in PREM tasks, there is
 * a vTaskPREMDelay that uses the generic timer.
 *
 * There must be no while(1) or for(;;) inside the task to repeat. Since it is an
 * already periodic task, it will repeat until deletion.
 *
 * Note that you can use pdMS_TO_TICKS to transform a time into ticks and
 * pdFREQ_TO_TICKS to transform a frequency into ticks.
 *
 * This is simply creating a task using xTaskPeriodicCreate but makes it PREM. Don't
 * forget to run the vInitPREM before creating a PREM task
 */
BaseType_t xTaskPREMCreate(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const configSTACK_DEPTH_TYPE uxStackDepth,
                           const struct premtask_parameters premtask_parameters,
                           UBaseType_t uxPriority,
                           TaskHandle_t *const pxCreatedTask);

/*
 * Deletes the PREM task. If time is measured, it will send one hypercall with 4 values:
 * - The core id
 * - The task id
 * - The max response time
 * - The sum of all response times (for average)
 * 
 * This will call the vTaskPeriodicDelete function.
 */
void vTaskPREMDelete(void);    

void askDisplayResults(void);

/*
 * Init PREM with this function. This is mandatory to do it if you used the DEFAULT_IPI
 * option. In that case you will need to run it once before starting PREM tasks. This
 * function will enable IPI and implement handlers for you! Running this function more
 * than once is useless... You can of course run PREM task without it, but do not exepect
 * IPI and if your task gets suspended, it's for the rest of its life.
 */
void vInitPREM();

#endif
