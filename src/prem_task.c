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
    uint32_t priority;
    void *pvParameters;
};

volatile union memory_request_answer memory_access = {.raw = 0};
volatile int hypercalled = 0;

uint64_t sysfreq = 0;

void suspend_task()
{
    while (!memory_access.raw)
        ;
}

#ifdef MEMORY_REQUEST_WAIT
/* Waits for ttw systick ticks */
void wait_for(uint64_t ttw)
{
    // Because the scheduler is disabled, we can't use vTaskDelay
    // TODO Look for setting a timer
    uint64_t current_time = generic_timer_read_counter();
    uint64_t end_time = current_time + ttw;
    while (current_time < end_time)
    {
        current_time = generic_timer_read_counter();
    }
}
#endif

#ifdef DEFAULT_IPI_HANDLERS
void ipi_pause_handler(unsigned int id)
{
    // printf("BBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
    // printf("Please register the IPI PAUSE interrupt ;(\n");
    hypercall(HC_DISPLAY_STRING, 6, 0, 0);
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // Pause task
        change_state(SUSPENDED);
        memory_access.raw = 0;
        suspend_task();
    }
    else
    {
        printf("CPU 1 is not in MEMORY_PHASE...\n");
    }
    hypercall(HC_DISPLAY_STRING, 6, 0, 0);
}

void ipi_resume_handler(unsigned int id)
{
    enum states current_state = get_current_state();
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    hypercall(HC_DISPLAY_STRING, 7, 0, 0);
    if (current_state == SUSPENDED)
    {
        // Resume task
        memory_access.raw = 1;
        change_state(MEMORY_PHASE);
    }
    else
    {
        printf("CPU 1 is not in SUSPENDED state...\n");
    }
    hypercall(HC_DISPLAY_STRING, 7, 0, 0);
}
#endif

void vTaskPREMDelay(TickType_t waitingTicks)
{
    uint64_t current_time = generic_timer_read_counter();
    uint64_t end_time = current_time + pdTICKS_TO_SYSTICK(sysfreq, waitingTicks);
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

    // Increment hypercall counter and if it is 1, we request memory (else no)
    if (++hypercalled == 1)
    {
        memory_access.raw = request_memory_access(prv_premtask_parameters->priority);
    }

    // When memory access wait is defined, memory ack 1 is the only yes value
    if (memory_access.raw != 1)
    {
#ifdef MEMORY_REQUEST_WAIT
        // If time to wait > 0, then hypervisor wants us to wait for a bit...
        // Just wait, no need to enable the scheduler since all hypercall will result in wait
        // If you waited to short, then rewait
        while (memory_access.ttw != 0)
        {
            // Wait for this duration and then make an hypercall
            wait_for(memory_access.ttw);
            memory_access.raw = request_memory_access(prv_premtask_parameters->priority);

            // Even if the answer is 1, we re-enable the scheduler since the task was waiting!
            // Maybe a higher priority task is ready, we need to verify that!
            // Even if no task is waiting, it's only losing a few cycles so it's ok
        }
#endif

        // Suspended state (resume scheduler to allow preemption)
        change_state(SUSPENDED);
        xTaskResumeAll();
        suspend_task();

        // Stop scheduler again
        vTaskSuspendAll();
    }

    // Fetch
    change_state(MEMORY_PHASE);
    prefetch_data((uint64_t)prv_premtask_parameters->data, prv_premtask_parameters->data_size);
    vTaskPREMDelay(pdMS_TO_TICKS(100));

    // Revoke access and compute
    change_state(COMPUTATION_PHASE);
    revoke_memory_access();
    prv_premtask_parameters->pxTaskCode(prv_premtask_parameters->pvParameters);

    // Decrease hypercall counter and if it is more than 1, we request again (for preempted task)
    if (--hypercalled)
    {
        memory_access.raw = request_memory_access(prv_premtask_parameters->priority);

#ifdef MEMORY_REQUEST_WAIT
        // Same thing here, if the hypervisor wants to wait, then we wait... instead of giving the hand.
        // This allows to not put these checks in handlers and allow new tasks to arrive
        while (memory_access.ack == 0 && memory_access.ttw != 0)
        {
            // Wait for this duration and then make an hypercall
            wait_for(memory_access.ttw);
            memory_access.raw = request_memory_access(prv_premtask_parameters->priority);

            // This task exits and made a memory request for the next one and is sure that it didn't ask too fast
        }
#endif
    }
    else
    {
        memory_access.raw = 0;
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
    // This structure is NEVER freed since task is never deleted and this is always used!
    struct prv_premtask_parameters *premtask_parameters_ptr = (struct prv_premtask_parameters *)pvPortMalloc(sizeof(struct prv_premtask_parameters));

    premtask_parameters_ptr->pxTaskCode = pxTaskCode;
    premtask_parameters_ptr->data_size = premtask_parameters.data_size;
    premtask_parameters_ptr->data = premtask_parameters.data;
    premtask_parameters_ptr->priority = hypercall(HC_GET_CPU_ID, 0, 0, 0);
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

void vInitPREM()
{
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

    // Get system freq
    sysfreq = generic_timer_get_freq();
}
