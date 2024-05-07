#include <prem_task.h>
#include <FreeRTOS.h>
#include <task.h>
#include <periodic_task.h>
#include <state_machine.h>
#include <prefetch.h>
#include <hypervisor.h>
#include <ipi.h>
#include <irq.h>

// TODO IPI handlers + PREM init

struct prv_premtask_parameters
{
    TaskFunction_t pxTaskCode;
    uint64_t data_size;
    void *data;
    uint32_t priority;
    void *pvParameters;
};

volatile int memory_access = 0;
volatile int hypercalled = 0;

void suspend_task()
{
    while (!memory_access)
        ;
}

#ifdef DEFAULT_IPI_HANDLERS
void ipi_pause_handler()
{
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // Pause task
        change_state(SUSPENDED);
        memory_access = 0;
        suspend_task();
    }
}

void ipi_resume_handler()
{
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // Pause task
        memory_access = 1;
        change_state(MEMORY_PHASE);
    }
}
#endif

/* The PREM task */
void vPREMTask(void *pvParameters)
{
    // pvParameters are the private PREM struct
    struct prv_premtask_parameters *prv_premtask_parameters = (struct prv_premtask_parameters *)pvParameters;

    // Stop scheduler to be sure to not be preempted
    vTaskSuspendAll();

    // Increment hypercall counter and if it is 1, we request memory (else no)
    if (++hypercalled == 1)
    {
        memory_access = request_memory_access(prv_premtask_parameters->priority);
    }

    if (!memory_access)
    {
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

    // Revoke access and compute
    change_state(COMPUTATION_PHASE);
    revoke_memory_access();
    prv_premtask_parameters->pxTaskCode(prv_premtask_parameters->pvParameters);

    // Decrease hypercall counter and if it is more than 1, we request again (for preempted task)
    if (--hypercalled)
    {
        memory_access = request_memory_access(prv_premtask_parameters->priority);
    }
    else
    {
        memory_access = 0;
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
}
