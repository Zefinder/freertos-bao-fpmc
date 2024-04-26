#include <FreeRTOS.h>
#include <periodic_task.h>

#include <irq.h>
#include <ipi.h>
#include <plat.h>

#include <stdio.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <benchmark.h>

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define CPU_PRIO 0
#define NUMBER_OF_TESTS 90000

// Change location for appdata
#define DATA_SIZE 200 * 1024
uint8_t appdata[DATA_SIZE] = {1};

void prefetch_memory()
{
    // TODO Create random app data
    prefetch_data((uint64_t)appdata, DATA_SIZE);
}

int counter = 0;
int print_counter = 0;
void vTask(void *pvParameters)
{
    if (counter < NUMBER_OF_TESTS)
    {
        // Asking for memory access
        int ack_access = request_memory_access(CPU_PRIO);

        // If access not granted then we suspend task
        if (!ack_access)
        {
            change_state(SUSPENDED);
            vTaskSuspend(NULL);
        }

        // Clear and prefetch
        change_state(MEMORY_PHASE);
        clear_L2_cache((uint64_t)appdata, DATA_SIZE);
        run_benchmark(prefetch_memory, NULL);

        // Revoke for computation
        change_state(COMPUTATION_PHASE);
        revoke_memory_access();

        // Set idling back
        change_state(WAITING);

        // Increment counter
        counter += 1;

        // Print for each 1000 tests
        if (++print_counter == 1000)
        {
            printf("\t# Number of realised tests: %d\n", counter);
            print_counter = 0;
        }
    }
    else
    {
        // Show results and destroy task
        print_benchmark_results();
        end_benchmark();
        printf("Tests finished!\n");
        vTaskDelete(NULL);
    }
}

void ipi_pause_handler()
{
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // TODO Pause task
        change_state(SUSPENDED);
        vTaskSuspend(xTaskHandler);
        // printf("Suspended\n");
    }
    else
    {
        // TODO Ensure that it is not possible OR won't block for nothing
        // int cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
        // printf("Try to pause CPU %d when not in pausable state\n", cpu_id);
        // taskDISABLE_INTERRUPTS();
        // for (;;)
        //     ;
    }
}

void ipi_resume_handler()
{
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // TODO Resume task
        vTaskResume(xTaskHandler);
        // printf("Resumed\n");
    }
    else
    {
        // int cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);
        // printf("Try to pause CPU %d when not in pausable state\n", cpu_id);
        // taskDISABLE_INTERRUPTS();
        // for (;;)
        //     ;
    }
}

void main_app(void)
{
    irq_set_handler(IPI_IRQ_PAUSE, ipi_pause_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_RESUME, ipi_resume_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    int frequency = 300;
    printf("Begin fpsched prefetch tests...\n");
    start_benchmark();
    init_benchmark(NULL, 0);

    xTaskPeriodicCreate(
        vTask,
        "ExecutionFPSCHED",
        configMINIMAL_STACK_SIZE,
        pdFREQ_TO_TICKS(frequency),
        NULL,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}