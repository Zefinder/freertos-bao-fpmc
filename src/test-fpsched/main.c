#include <FreeRTOS.h>
#include <task.h>

#include <uart.h>
#include <irq.h>
#include <plat.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <ipi.h>
#include <periodic_task.h>
#include <data.h>

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

// Change location for appdata
#define DATA_SIZE 448 kB

void prefetch_memory()
{
    // TODO Create random app data
    prefetch_data((uint64_t)appdata, DATA_SIZE);
}

void vTaskHypercall(void *pvParameters)
{
    int cpu_id = (int *)pvParameters;

    // Asking for memory access
    int ack_access = request_memory_access(cpu_id);

    // If access not granted then we suspend task
    if (!ack_access)
    {
        change_state(SUSPENDED);
        vTaskSuspend(NULL);
    }

    // Access granted
    change_state(MEMORY_PHASE);
    prefetch_memory();
    vTaskDelay(800 / portTICK_PERIOD_MS);

    // printf("Enter computation section\n");
    change_state(COMPUTATION_PHASE);
    revoke_memory_access();
}

void uart_rx_handler()
{
    printf("%s\n", "PAUSE");
    while (1)
        __asm__ volatile("wfi \n\t");
    // hypercall(5, 1, 2, 3);
    uart_clear_rxirq();
}

void ipi_pause_handler()
{
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // TODO Pause task
        change_state(SUSPENDED);
        vTaskSuspend(xTaskHandler);
    }
    else
    {
        int cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
        printf("Try to pause CPU %d when not in pausable state\n", cpu_id);
        taskDISABLE_INTERRUPTS();
        for (;;)
            ;
    }
}

void ipi_resume_handler()
{
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // TODO Resume task
        vTaskResume(xTaskHandler);
    }
    else
    {
        int cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
        printf("Try to pause CPU %d when not in pausable state\n", cpu_id);
        taskDISABLE_INTERRUPTS();
        for (;;)
            ;
    }
}

void main_app(void)
{
    uart_enable_rxirq();
    irq_set_handler(UART_IRQ_ID, uart_rx_handler);
    irq_set_prio(UART_IRQ_ID, IRQ_MAX_PRIO);
    irq_enable(UART_IRQ_ID);

    irq_set_handler(IPI_IRQ_PAUSE, ipi_pause_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_RESUME, ipi_resume_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    int cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    struct periodic_arguments periodic_arguments = {.tickPeriod = pdMS_TO_TICKS(1000), .pvParameters = (void *)cpu_id};
    xTaskPeriodicCreate(
        vTaskHypercall,
        "TestPeriodicTask",
        configMINIMAL_STACK_SIZE,
        periodic_arguments,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}