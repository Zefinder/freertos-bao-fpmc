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

#define CPU_MASTER 0
#define NUMBER_OF_TESTS 90000
#define MIN_FREQUENCY 100
#define MAX_FREQUENCY 300
#define MIN_DATA_SIZE 40 * 1024
#define DATA_SIZE_INCREMENT 40 * 1024

// Change location for appdata
#define DATA_SIZE (440 * 1024)
uint8_t appdata[DATA_SIZE] = {1};

TaskHandle_t xTaskCreatorHandler;

// Int to define end of infinite prefetch task
int end_tests = 0;

void prefetch_memory(void *data_size)
{
    // TODO Create random app data
    prefetch_data((uint64_t)appdata, (uint64_t)data_size);
}

void vMaxFreqTask(void *pvParameters)
{
    // Clear and prefetch
    clear_L2_cache((uint64_t)appdata, DATA_SIZE);
    prefetch_memory((void *)DATA_SIZE);

    // Destroy task when all tests ended
    // TODO Use IPI interruption
    if (end_tests)
    {
        vTaskDelete(NULL);
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

    TickType_t min_ticks = pdFREQ_TO_TICKS(MIN_FREQUENCY);
    TickType_t max_ticks = pdFREQ_TO_TICKS(MAX_FREQUENCY);
    int use_nano = 0;

    // min_freq_ticks > max_freq_ticks!
    for (TickType_t ticks = min_ticks; ticks >= max_ticks; ticks--)
    {
        for (uint64_t data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
        {
            uint32_t ulNotifiedValue;
            int frequency = configTICK_RATE_HZ / ticks;
            int data_size_ko = data_size / 1024;

            printf("# Start benchmark: frequency=%dHz,data size=%dkB\n", frequency, data_size_ko);
            char test_name[20];
            sprintf(test_name, "_%dHz_%dkB", frequency, data_size_ko);
            init_benchmark(test_name, use_nano);

            TaskHandle_t xBenchmarkTaskHandle;
            counter = 0;
            print_counter = 0;
            struct periodic_arguments periodic_arguments = {.tickPeriod = ticks, .pvParameters = (void *)data_size};
            xTaskPeriodicCreate(
                vBenchmarkTask,
                "Execution2Tasks",
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
    }

    end_benchmark();
}

void main_app(void)
{
    int cpu_number = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    if (cpu_number == CPU_MASTER)
    {
        printf("Begin 2 tasks tests (from %dHz to %dHz, prefetch %dkB to %dkB)...\n", MIN_FREQUENCY, MAX_FREQUENCY, MIN_DATA_SIZE / 1024, DATA_SIZE / 1024);
        xTaskCreate(vBenchCreationTask,
                    "Benchmark creator",
                    configMINIMAL_STACK_SIZE,
                    NULL,
                    tskIDLE_PRIORITY + 1,
                    &(xTaskCreatorHandler));
    }
    else
    {
        struct periodic_arguments periodic_arguments = {.tickPeriod = pdFREQ_TO_TICKS(MAX_FREQUENCY), .pvParameters = NULL};
        xTaskPeriodicCreate(
            vMaxFreqTask,
            "Execution2Tasks",
            configMINIMAL_STACK_SIZE,
            periodic_arguments,
            tskIDLE_PRIORITY + 1,
            (TaskHandle_t *)NULL);
    }

    vTaskStartScheduler();
}