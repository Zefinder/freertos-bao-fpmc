#include <FreeRTOS.h>
#include <periodic_task.h>

#include <plat.h>

#include <stdio.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <benchmark.h>

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
        // Clear cache
        clear_L2_cache((uint64_t)appdata, DATA_SIZE);

        // Benchmark prefetching
        run_benchmark(prefetch_memory, NULL);

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

void main_app(void)
{
    int frequency = 300;
    printf("Begin legacy prefetch tests...\n");
    start_benchmark();
    init_benchmark(NULL, 0);

    struct periodic_arguments periodic_arguments = {.tickPeriod = pdFREQ_TO_TICKS(frequency), .pvParameters = NULL};

    xTaskPeriodicCreate(
        vTask,
        "ExecutionLEGACY",
        configMINIMAL_STACK_SIZE,
        periodic_arguments,
        tskIDLE_PRIORITY + 1,
        (TaskHandle_t *)NULL);

    vTaskStartScheduler();
}