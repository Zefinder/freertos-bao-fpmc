#include <FreeRTOS.h>
#include <task.h>

#include <uart.h>
#include <irq.h>
#include <plat.h>

#include <stdio.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <ipi.h>
#include <periodic_task.h>
#include <prem_task.h>
#include <data.h>
#include <generic_timer.h>

// Request the default IPI and memory request wait
#ifndef DEFAULT_IPI_HANDLERS
#error DEFAULT_IPI must be defined with value y for this test (make all ... DEFAULT_IPI=y)
#endif
#ifndef MEMORY_REQUEST_WAIT
#error MEMORY_REQUEST_WAIT must be defined with value y for this test (make all ... MEMORY_REQUEST_WAIT=y)
#endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

// Change location for appdata
#define DATA_SIZE 448 kB

// Generic timer frequency 
uint64_t sysfreq;

struct task_data
{
    uint32_t cpu_id;
    uint32_t task_id;
};

void test(unsigned int id)
{
    printf("Hello hihi (int %d) (CPU %d)\n", id, hypercall(HC_GET_CPU_ID, 0, 0, 0));
}

void vTaskHypercall(void *pvParameters)
{
    struct task_data *task_data = (struct task_data *)pvParameters;

    // Just computes the sum and prints the result
    uint64_t sum = 0;
    for (int i = 0; i < DATA_SIZE; i++)
    {
        sum += appdata[i];
    }

    // printf("Sum of appdata for task %d on CPU %d: %ld\n", task_data->task_id, task_data->cpu_id, sum);
    vTaskPREMDelay(pdMS_TO_SYSTICK(sysfreq, 300));

    // irq_send_ipi(0b1);
}

void vTaskInterference()
{
    uint64_t sysfreq = generic_timer_get_freq();

    // Interfere a lot, do not let them breathe
    while (1)
    {
        // Memory phase (on highest priority CPU, so ack will always be 1 or 0 with time to wait)
        union memory_request_answer answer = {.raw = request_memory_access(0)};

        // This task is OP, please nerf
        while (answer.ack == 0 && answer.ttw != 0)
        {
            // Wait for this amount of ticks
            vTaskDelay(pdSYSTICK_TO_TICKS(sysfreq, answer.ttw));

            // Re-ask for access (but this time you are sure to be accepted right? Not sure thus while)
            answer.raw = request_memory_access(0);
        }

        // From here we are sure that we have memory access
        // We just wait for 500ms (Mi = 200ms, Ci = 3*Mi = 600ms)
        vTaskPREMDelay(pdMS_TO_TICKS(50));

        // Release memory
        revoke_memory_access();

        // Computation phase (nothing :D)
        // Back to memory phase then
        // hypercall(HC_DISPLAY_STRING, 0, 0, 0);
    }
}

struct task_data task1_data, task2_data;
void main_app(void)
{
    sysfreq = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    irq_set_handler(IPI_IRQ_ID, test);
    irq_enable(IPI_IRQ_ID);
    irq_set_prio(IPI_IRQ_ID, IRQ_MAX_PRIO);

    // irq_set_handler(IPI_IRQ_PAUSE, test);
    // irq_enable(IPI_IRQ_PAUSE);
    // irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    // // Enable IPI resume
    // irq_set_handler(IPI_IRQ_RESUME, test);
    // irq_enable(IPI_IRQ_RESUME);
    // irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    // Init and create PREM task
    vInitPREM();
    if (cpu_id == 1)
    {
        // uart_enable_rxirq();
        // irq_set_handler(UART_IRQ_ID, test);
        // irq_set_prio(UART_IRQ_ID, IRQ_MAX_PRIO);
        // irq_enable(UART_IRQ_ID);

        task1_data.cpu_id = cpu_id;
        task1_data.task_id = 1;
        task2_data.cpu_id = cpu_id;
        task2_data.task_id = 2;

        TickType_t waiting_ticks = pdMS_TO_TICKS(1000);

        struct premtask_parameters premtask1_parameters = {.tickPeriod = waiting_ticks, .data = appdata, .data_size = DATA_SIZE, .priority = &cpu_id, .pvParameters = (void *)&task1_data};
        struct premtask_parameters premtask2_parameters = {.tickPeriod = 3 * waiting_ticks, .data = appdata, .data_size = DATA_SIZE, .priority = &cpu_id, .pvParameters = (void *)&task2_data};
        xTaskPREMCreate(
            vTaskHypercall,
            "TestPREMTask1",
            configMINIMAL_STACK_SIZE,
            premtask1_parameters,
            tskIDLE_PRIORITY + 3,
            &(xTaskHandler));

        xTaskPREMCreate(
            vTaskHypercall,
            "TestPREMTask2",
            configMINIMAL_STACK_SIZE,
            premtask2_parameters,
            tskIDLE_PRIORITY + 2,
            &(xTaskHandler));
    }
    else
    {
        // Just interferences, the goal is to provoke destiny and try to show the bonus message (only during debug) that we ask too frequently
        // TickType_t waiting_ticks = pdMS_TO_TICKS(1);
        // struct premtask_parameters premtask1_parameters = {.tickPeriod = waiting_ticks, .data = appdata, .data_size = MAX_DATA_SIZE, .pvParameters = NULL};
        xTaskCreate(vTaskInterference,
                    "Blocked",
                    configMINIMAL_STACK_SIZE,
                    NULL,
                    tskIDLE_PRIORITY + 4,
                    NULL);
    }

    vTaskStartScheduler();
}