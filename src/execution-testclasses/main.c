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
#define TASK_FREQUENCY 500
#define MIN_DATA_SIZE 20 kB
#define DATA_SIZE_INCREMENT 20 kB
#define DATA_SIZE (440 kB)

TaskHandle_t xTaskCreatorHandler;

void prefetch_memory(void *data_size)
{
    prefetch_data((uint64_t)appdata, (uint64_t)data_size);
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
    int use_nano = 0;

    for (uint64_t data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
    {
        uint32_t ulNotifiedValue;
        int data_size_ko = BtkB(data_size);

        printf("# Start benchmark: frequency=%dHz,data size=%dkB\n", TASK_FREQUENCY, data_size_ko);
        char test_name[10];
        sprintf(test_name, "_%dkB", data_size_ko);
        init_benchmark(test_name, use_nano, 1);

        TaskHandle_t xBenchmarkTaskHandle;
        counter = 0;
        print_counter = 0;
        struct periodic_arguments periodic_arguments = {.tickPeriod = ticks, .pvParameters = (void *)data_size};
        xTaskPeriodicCreate(
            vBenchmarkTask,
            "ExecutionClass",
            configMINIMAL_STACK_SIZE,
            periodic_arguments,
            tskIDLE_PRIORITY + 1,
            &(xBenchmarkTaskHandle));

        ulTaskNotifyTakeIndexed(0,              /* Use the 0th notification */
                                pdTRUE,         /* Clear the notification value before exiting. */
                                portMAX_DELAY); /* Block indefinitely. */

        // Kill the task
        vTaskDelete(xBenchmarkTaskHandle);

        // If nano precision used, remove it for next time
        if (use_nano == 1)
        {
            use_nano = 0;
        }
        // Else, if the min time is 0, then use the nano precision and redo the test
        if (get_minimum_time() == 0 && use_nano == 0)
        {
            printf("# Use nano precision because measured 0us\n");
            use_nano = 1;
            data_size -= DATA_SIZE_INCREMENT;
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