#include <prem_task.h>
#include <FreeRTOS.h>
#include <task.h>
#include <periodic_task.h>
#include <state_machine.h>
#include <prefetch.h>
#include <hypervisor.h>
#include <ipi.h>
#include <irq.h>
#include <generic_timer.h>
#include <stdio.h>

/* PREM task parameters that are really used in the task */
struct prv_premtask_parameters
{
    TaskFunction_t pxTaskCode;
    uint64_t data_size;
    void *data;
    uint8_t task_id;
    uint64_t wcet;
    void *pvParameters;
};

volatile union memory_request_answer memory_access = {.raw = 0};
volatile uint8_t hypercalled = 0;
volatile uint8_t suspend_prefetch = 0;
volatile uint64_t end_low_prio = 0;
volatile uint8_t revoked = 0; // Here to get hypercall response and ensure revoke is called

uint64_t cpu_priority = 0;

uint8_t task_id = 0;
uint8_t kill_prem_task = 0;
uint8_t ask_display_results = 0;
uint8_t ask_change_prefetch_size = 0;
uint64_t new_prefetch_size = 0;

#ifdef MEASURE_RESPONSE_TIME
uint8_t measure_response_time = 1;
#else
uint8_t measure_response_time = 0;
#endif
uint64_t *response_max;
uint64_t *response_min;
uint64_t *response_sum;
uint64_t *response_number;

void suspend_task()
{
    // Wait for interrupt so we don't look at the memory for nothing
    while (suspend_prefetch == 1)
    {
        __asm__ volatile("wfi");

        // Woken up by either tick interrupt or IPI resume, ask for rescheduling
        vTaskDelay(0);
    }
}

#ifdef DEFAULT_IPI_HANDLERS
/* Value that indicates if need to suspend prefetch (0 is no) */
void ipi_pause_handler(unsigned int id)
{
    suspend_prefetch = 1;
    // printf("PAUSE CORE %d\n", cpu_priority);
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // Pause task (we already are in prefetch)
        change_state(SUSPENDED);
    }
}

void ipi_resume_handler(unsigned int id)
{
    suspend_prefetch = 0;
    // printf("RESUME CORE %d\n", cpu_priority);
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // Change state if was in suspended state
        change_state(MEMORY_PHASE);
    }
}
#endif

/*
 * At each tick, will compute remaining time and if needs to update prio to the hypervisor.
 *
 * MUST verify that not in computation phase!
 */
void vApplicationTickHook(void)
{
    if (end_low_prio != 0)
    {
        enum states current_state = get_current_state();

        // If already in computation phase, too late stop looking
        if (current_state == COMPUTATION_PHASE)
        {
            end_low_prio = 0;
        }
        else
        {
            uint64_t current_time = generic_timer_read_counter();
            if (current_time >= end_low_prio)
            {
                end_low_prio = 0;

                // Time to wait over, just yes or no!
                union memory_request_answer update = {.raw = update_memory_access(cpu_priority)};
                suspend_prefetch = ~update.ack;
            }
        }
    }
}

void vTaskPREMDelay(TickType_t waitingTicks)
{
    uint64_t current_time = generic_timer_read_counter();
    uint64_t end_time = current_time + waitingTicks;
    while (current_time < end_time)
    {
        current_time = generic_timer_read_counter();
    }
}

void display_results(uint8_t task_id)
{
    hypercall(HC_DISPLAY_RESULTS, task_id, response_max[task_id], response_sum[task_id]);
    hypercall(HC_DISPLAY_RESULTS, task_id, response_min[task_id], 0);

    // Reset values
    response_max[task_id] = 0;
    response_min[task_id] = UINT64_MAX;
    response_sum[task_id] = 0;
    response_number[task_id] = 0;
}

void vPREMTask(void *pvParameters)
{
    // pvParameters are the private PREM struct
    struct prv_premtask_parameters *prv_premtask_parameters = (struct prv_premtask_parameters *)pvParameters;

    // Stop scheduler to be sure to not be preempted
    vTaskSuspendAll();

    // If hypercall counter is 0, then we request memory
    if (hypercalled++ == 0)
    {
        memory_access.raw = request_memory_access(cpu_priority, prv_premtask_parameters->wcet);
    }

    // Whether the answer is yes or no, if ttw is not 0 then set a number a cycles to wait before leaving low prio
    if (memory_access.ttw != 0)
    {
        // Compute cycles to wait
        end_low_prio = generic_timer_read_counter() + memory_access.ttw;
    }

    // If answer is no, then set suspended
    if (memory_access.ack == 0)
    {
        change_state(SUSPENDED);
        suspend_prefetch = 1;
        // Re-enable scheduler so higher prio tasks can take over
        // xTaskResumeAll();
        // suspend_task();

        // // Once got out, stop scheduler!
        // vTaskSuspendAll();
    }

    // Begin memory phase
    change_state(MEMORY_PHASE);
    prefetch_data_prem((uint64_t)prv_premtask_parameters->data, prv_premtask_parameters->data_size, &suspend_prefetch);

    // Revoke access and compute
    change_state(COMPUTATION_PHASE);
    end_low_prio = 0;
    revoked = revoke_memory_access();
    // If successfully revoked, then execute code, else clear cache
    if (revoked == 0)
    {
        // printf("Revoked!\n");
        prv_premtask_parameters->pxTaskCode(prv_premtask_parameters->pvParameters);
    }

    // Clear used cache
    clear_L2_cache((uint64_t)prv_premtask_parameters->data, prv_premtask_parameters->data_size);

    // Decrease hypercall counter and if it is more than 1, we request again (for preempted task)
    if (--hypercalled != 0)
    {
        memory_access.raw = request_memory_access(cpu_priority, prv_premtask_parameters->wcet);

        // If there is still delay, set cycles once again
        if (memory_access.ttw != 0)
        {
            end_low_prio = generic_timer_read_counter() + memory_access.ttw;
        }
    }
    else
    {
        memory_access.raw = 0;
    }

    // Measure response time (Write from hypervisor task priority and response time)
    // Always skip the first one
    if (measure_response_time && response_number[prv_premtask_parameters->task_id]++ > 0)
    {
        uint64_t end_time = generic_timer_read_counter();
        uint64_t release_time = get_last_period_start(prv_premtask_parameters->task_id);
        uint64_t response_time = pdSYSTICK_TO_NS(generic_timer_get_freq(), end_time - release_time);

        if (response_max[prv_premtask_parameters->task_id] < response_time)
        {
            response_max[prv_premtask_parameters->task_id] = response_time;
        }

        if (response_min[prv_premtask_parameters->task_id] > response_time)
        {
            response_min[prv_premtask_parameters->task_id] = response_time;
        }

        response_sum[prv_premtask_parameters->task_id] += response_time;
    }

    // If wants to change the prefetch size, then change it here
    // If the user is schizophrenic and also asks to kill the task,
    // it won't change to something freed
    if (ask_change_prefetch_size == 1)
    {
        ask_change_prefetch_size = 0;
        prv_premtask_parameters->data_size = new_prefetch_size;
    }

    // If the user wants to delete the task, then do it
    if (kill_prem_task == 1)
    {
        // Notify periodic task to kill this one
        kill_prem_task = 0;
        vTaskPeriodicDelete();

        display_results(prv_premtask_parameters->task_id);

        // Free task parameters
        vPortFree(prv_premtask_parameters);
    }
    else if (ask_display_results == 1)
    {
        ask_display_results = 0;
        display_results(prv_premtask_parameters->task_id);
    }

    // Wait (resume scheduler)
    change_state(WAITING);
    xTaskResumeAll();
}

BaseType_t xTaskPREMCreate(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const configSTACK_DEPTH_TYPE uxStackDepth,
                           const struct premtask_parameters premtask_parameters,
                           UBaseType_t uxPriority,
                           TaskHandle_t *const pxCreatedTask)
{
    // Create and fill struct
    // This structure is freed only when task gets deleted!
    struct prv_premtask_parameters *premtask_parameters_ptr = (struct prv_premtask_parameters *)pvPortMalloc(sizeof(struct prv_premtask_parameters));

    premtask_parameters_ptr->pxTaskCode = pxTaskCode;
    premtask_parameters_ptr->data_size = premtask_parameters.data_size;
    premtask_parameters_ptr->data = premtask_parameters.data;
    premtask_parameters_ptr->task_id = task_id++;
    premtask_parameters_ptr->wcet = pdNS_TO_SYSTICK(generic_timer_get_freq(), premtask_parameters.wcet);
    premtask_parameters_ptr->pvParameters = premtask_parameters.pvParameters;

    // Create a periodic task with custom arguments
    struct periodic_arguments periodic_arguments = {.tickPeriod = premtask_parameters.tickPeriod, .pvParameters = (void *)premtask_parameters_ptr};
    xTaskPeriodicCreate(vPREMTask,
                        pcName,
                        uxStackDepth,
                        periodic_arguments,
                        uxPriority,
                        pxCreatedTask);
}

void vTaskPREMDelete()
{
    kill_prem_task = 1;
}

void askDisplayResults()
{
    ask_display_results = 1;
}

void askChangePrefetchSize(uint64_t new_size)
{
    ask_change_prefetch_size = 1;
    new_prefetch_size = new_size;
}

void vInitPREM()
{
    // Init periodic tasks
    vInitPeriodic();

#ifdef MEASURE_RESPONSE_TIME
    // Malloc the measure pointers
    response_sum = pvPortMalloc(sizeof(uint64_t) * task_id);
    response_min = pvPortMalloc(sizeof(uint64_t) * task_id);
    response_max = pvPortMalloc(sizeof(uint64_t) * task_id);
    for (int i = 0; i < task_id; i++)
    {
        response_max[i] = 0;
        response_min[i] = UINT64_MAX;
        response_sum[i] = 0;
        response_number[i] = 0;
    }
#endif

// Only set handlers iff they are defined before
#ifdef DEFAULT_IPI_HANDLERS
    // Enable IPI pause
    irq_set_handler(IPI_IRQ_PAUSE, ipi_pause_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    // Enable IPI resume
    irq_set_handler(IPI_IRQ_RESUME, ipi_resume_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);
#endif

    // Set CPU priority
    cpu_priority = hypercall(HC_GET_CPU_ID, 0, 0, 0);
}