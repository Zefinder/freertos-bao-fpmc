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

#define NUMBER_OF_TESTS 90000
#define TASK_FREQUENCY 500
#define MIN_DATA_SIZE 20 * 1024
#define DATA_SIZE_INCREMENT 20 * 1024

#define MAX_NO_IMPROVE 3
#define PREFETCH_BATCH_SIZE 0x40

// Structure for task
struct task_info
{
    uint64_t data_size;
    int nop_number;
};

// Change location for appdata
#define DATA_SIZE (440 * 1024)
uint8_t appdata[DATA_SIZE] = {[0 ... DATA_SIZE - 1] = 1};
TaskHandle_t xTaskCreatorHandler;

void prefetch_memory(void *task_struct)
{
    struct task_info *task_info = (struct task_info *)task_struct;
    for (int offset = 0; offset < task_info->data_size; offset += PREFETCH_BATCH_SIZE)
    {
        prefetch_data((uint64_t)appdata + offset, (uint64_t)PREFETCH_BATCH_SIZE);
        for (int wait = 0; wait < task_info->nop_number; wait++)
        {
            __asm__ volatile("nop");
        }
    }
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
    for (uint64_t data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
    {
        int no_improve = 0;
        int nop_number = 0;
        uint64_t min_result = -1;
        while (no_improve < MAX_NO_IMPROVE)
        {
            // Add 1 to number of NOP
            nop_number += 1;
            uint32_t ulNotifiedValue;
            int data_size_ko = data_size / 1024;

            printf("# Start benchmark: frequency=%dHz,data size=%dkB, nop=%d\n", TASK_FREQUENCY, data_size_ko, nop_number);
            char test_name[20];
            sprintf(test_name, "_%dkB_%dnop", data_size_ko);
            init_benchmark(test_name, 0);

            // Create task
            TaskHandle_t xBenchmarkTaskHandle;
            counter = 0;
            print_counter = 0;
            struct task_info task_info = {.data_size = data_size, .nop_number = nop_number};
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

            // Check if improved
            uint64_t min_run = get_minimum_time();
            if (min_run < min_result)
            {
                // Reset the no improve and change minimum
                no_improve = 0;
                min_result = min_run;
            }
            else
            {
                // Else increate the no improve
                no_improve += 1;
            }
        }

        printf("# %d attemps that did not improve results, next!\n");
    }

    end_benchmark();
}

void main_app(void)
{
    printf("Begin class determination tests (%d Hz, prefetch %dkB to %dkB)...\n", TASK_FREQUENCY, MIN_DATA_SIZE / 1024, DATA_SIZE / 1024);
    xTaskCreate(vBenchCreationTask,
                "Benchmark creator",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                &(xTaskCreatorHandler));

    vTaskStartScheduler();
}