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
#include <data.h>

#define NUMBER_OF_TESTS 90000
#define TASK_FREQUENCY 1000
#define DATA_SIZE 448 kB
#define MIN_DATA_SIZE 1 kB
#define DATA_SIZE_INCREMENT 1 kB

#define MAX_NOP 10
#define INTERFERING_CORES 3

// Structure for task
struct task_info
{
    uint64_t data_size;
};

TaskHandle_t xTaskCreatorHandler;

void prefetch_memory(void *task_struct)
{
    struct task_info *task_info = (struct task_info *)task_struct;
    prefetch_data((uint64_t)appdata, (uint64_t)task_info->data_size);
}

int counter = 0;
int print_counter = 0;

void vBenchmarkTask(void *pvParameters)
{
    if (counter < NUMBER_OF_TESTS)
    {
        uint64_t data_size = (uint64_t)pvParameters;

        // Clear and prefetch
        clear_L2_cache((uint64_t)appdata, data_size);
        run_benchmark(prefetch_memory, (void *)data_size);

        // Increment counter
        counter += 1;

        // Print for each 1000 tests
        if (++print_counter == 1000)
        {
            printf("\t# Number of realised tests: %d\n", counter);
            print_counter = 0;
        }
    }
    else if (counter == NUMBER_OF_TESTS)
    {
        // Show results and notify main task for deletion
        print_benchmark_results();
        printf("\n");
        xTaskNotifyGiveIndexed(xTaskCreatorHandler, 0);

        vTaskDelay(portMAX_DELAY);
    }
}

void vBenchCreationTask(void *pvParameters)
{
    start_benchmark();

    TickType_t ticks = pdFREQ_TO_TICKS(TASK_FREQUENCY);

    // For each size you test with multiple NOPs
    for (int nop_number = 0; nop_number < MAX_NOP; nop_number++)
    {
        for (uint64_t data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
        {
            uint32_t ulNotifiedValue;
            int data_size_ko = data_size / 1024;

            printf("# Start benchmark: frequency=%dHz,data size=%dkB, nop=%d\n", TASK_FREQUENCY, data_size_ko, nop_number);
            char test_name[20];
            sprintf(test_name, "_%dkB_%dnop", data_size_ko, nop_number);
            init_benchmark(test_name, 0);

            // Create task
            TaskHandle_t xBenchmarkTaskHandle;
            counter = 0;
            print_counter = 0;
            struct task_info task_info = {.data_size = data_size};
            struct periodic_arguments periodic_arguments = {.tickPeriod = ticks, .pvParameters = (void *)&task_info};
            xTaskPeriodicCreate(
                vBenchmarkTask,
                "ExecutionClass",
                configMINIMAL_STACK_SIZE,
                periodic_arguments,
                tskIDLE_PRIORITY + 1,
                &(xBenchmarkTaskHandle));

            // Wait for task to finish
            ulTaskNotifyTakeIndexed(0,              /* Use the 0th notification */
                                    pdTRUE,         /* Clear the notification value before exiting. */
                                    portMAX_DELAY); /* Block indefinitely. */

            // Kill the task
            vTaskDelete(xBenchmarkTaskHandle);
        }

        // Send an hypercall to notify CPUs to change NOP number
        for (int interference = 1; interference <= INTERFERING_CORES; interference++)
        {
            hypercall(HC_NOTIFY_CPU, interference, 0, 0);
        }
    }

    end_benchmark();
}

void main_app(void)
{
    printf("Begin class determination tests (%d Hz, prefetch %dkB to %dkB)...\n", TASK_FREQUENCY, BtkB(MIN_DATA_SIZE), BtkB(DATA_SIZE));
    xTaskCreate(vBenchCreationTask,
                "Benchmark creator",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                &(xTaskCreatorHandler));

    vTaskStartScheduler();
}