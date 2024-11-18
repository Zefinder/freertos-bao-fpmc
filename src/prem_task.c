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

extern uint64_t sysfreq;
uint64_t cpu_priority = 0;

uint8_t task_id = 0;

#ifdef MEASURE_RESPONSE_TIME
uint8_t measure_response_time = 1;
#else
uint8_t measure_response_time = 0;
#endif

void suspend_task()
{
    // Wait for interrupt so we don't look at the memory for nothing
    while (!memory_access.raw)
        __asm__ volatile("wfi");
        
        // Woken up by either tick interrupt or IPI resume, ask for rescheduling
        vTaskDelay(0);

    suspend_prefetch = 0;
}

#ifdef DEFAULT_IPI_HANDLERS
/* Value that indicates if need to suspend prefetch (0 is no) */
void ipi_pause_handler(unsigned int id)
{
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // Pause task (we already are in prefetch)
        change_state(SUSPENDED);
        suspend_prefetch = 1;
    }
}

void ipi_resume_handler(unsigned int id)
{
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // Resume task
        suspend_prefetch = 0;
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
                update_memory_access(cpu_priority);
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
        xTaskResumeAll();
        suspend_task();

        // Once got out, stop scheduler!
        vTaskSuspendAll();
    }

    // Begin memory phase
    change_state(MEMORY_PHASE);
    prefetch_data_prem((uint64_t)prv_premtask_parameters->data, prv_premtask_parameters->data_size, &suspend_prefetch);

    // Revoke access and compute
    change_state(COMPUTATION_PHASE);
    end_low_prio = 0;
    revoke_memory_access();
    prv_premtask_parameters->pxTaskCode(prv_premtask_parameters->pvParameters);

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
    if (measure_response_time)
    {
        uint64_t end_time = generic_timer_read_counter();
        uint64_t release_time = get_last_period_start(prv_premtask_parameters->task_id);
        uint64_t response_time = pdSYSTICK_TO_NS(sysfreq, end_time - release_time);
        hypercall(HC_DISPLAY_RESULTS, prv_premtask_parameters->task_id, response_time, 0);
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
    if (sysfreq == 0) {
        sysfreq = generic_timer_get_freq();
    }

    // Create and fill struct
    // This structure is NEVER freed since task is never deleted and this is always used!
    struct prv_premtask_parameters *premtask_parameters_ptr = (struct prv_premtask_parameters *)pvPortMalloc(sizeof(struct prv_premtask_parameters));

    premtask_parameters_ptr->pxTaskCode = pxTaskCode;
    premtask_parameters_ptr->data_size = premtask_parameters.data_size;
    premtask_parameters_ptr->data = premtask_parameters.data;
    premtask_parameters_ptr->task_id = task_id++;
    premtask_parameters_ptr->wcet = premtask_parameters.wcet;
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

// TODO vPREMTaskDelete

void vInitPREM()
{
    // Init periodic tasks
    vInitPeriodic();

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