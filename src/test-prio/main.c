#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#include <uart.h>
#include <irq.h>
#include <plat.h>

#include <hypervisor.h>
#include <periodic_task.h>

// TODO Use vTaskSuspendAll() to enter "Critical sections" since it'll stop the scheduler BUT keeping the interruptions (ENTER_CRITICAL do not)  

void vTaskHypercall(void *pvParameters)
{
    int task_id = (int)pvParameters;
    printf("Task %d in action!\n", task_id);
    TickType_t now = xTaskGetTickCount();
    xTaskDelayUntil(&now, pdMS_TO_TICKS(5000));
    printf("Task %d in action!\n", task_id);
    vTaskDelete(NULL);
}

volatile int resume = 0;
void vTask2(void *pvParameters)
{
    int ack = request_memory_access(1);
    if (ack) {
        // vTaskSuspendAll();
        int task_id = (int)pvParameters;
        printf("Task %d in action!\n", task_id);
        while (!resume)
            ;
        printf("Task %d exit critical!\n", task_id);
        // xTaskResumeAll();
    } else {
        // Resume set to 1 when memory access ok
        printf("Ack not given!\n");
        while (!resume) {
            ;
        }
    }

    vTaskDelete(NULL);
}

void uart_rx_handler()
{
    printf("%s\n", "RESUME");
    resume = 1;
    uart_clear_rxirq();
}

void main_app(void)
{
    uart_enable_rxirq();
    irq_set_handler(UART_IRQ_ID, uart_rx_handler);
    irq_set_prio(UART_IRQ_ID, IRQ_MAX_PRIO);
    irq_enable(UART_IRQ_ID);

    xTaskCreate(
        vTaskHypercall,
        "TestTask1",
        configMINIMAL_STACK_SIZE,
        (void *)1,
        tskIDLE_PRIORITY + 3,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vTask2,
        "TestTask2",
        configMINIMAL_STACK_SIZE,
        (void *)2,
        tskIDLE_PRIORITY + 2,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vTaskHypercall,
        "TestTask3",
        configMINIMAL_STACK_SIZE,
        (void *)3,
        tskIDLE_PRIORITY + 1,
        (TaskHandle_t *)NULL);

    vTaskStartScheduler();
}