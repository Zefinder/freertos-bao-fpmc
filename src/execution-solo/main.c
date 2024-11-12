#include <FreeRTOS.h>
#include <periodic_task.h>

#include <irq.h>
#include <ipi.h>
#include <plat.h>

#include <stdio.h>

#include <prefetch.h>
#include <benchmark.h>
#include <data.h>

#define NUMBER_OF_TESTS 100000
#define DATA_SIZE MAX_DATA_SIZE
#define MIN_DATA_SIZE 1 kB
#define DATA_SIZE_INCREMENT 1 kB

int counter = 0;
int print_counter = 0;
volatile uint64_t data_size = 0;

void prefetch(void *arg)
{
    // Just prefetch
    prefetch_data((uint64_t)appdata, (uint64_t)data_size);
}

void vMasterTask(void *pvParameters)
{
    start_benchmark();
    printf("\t# Number of realised tests: %d\n", counter);

    for (data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
    {
        // Init benchmark
        int data_size_ko = BtkB(data_size);
        char test_name[20];
        sprintf(test_name, "_%dkB", data_size_ko);
        init_benchmark(test_name, MEASURE_NANO);

        while (counter++ < NUMBER_OF_TESTS)
        {
            // Clear cache
            clear_L2_cache((uint64_t)appdata, (uint64_t)data_size);

            // Run prefetch benchmark
            run_benchmark(prefetch, NULL);

            // Print for each 1000 tests
            if (++print_counter == 1000)
            {
                printf("\t# Number of realised tests: %d\n", counter);
                print_counter = 0;
            }
        }

        // Reset counters
        counter = 0;
        print_counter = 0;
        print_benchmark_results();
        printf("\n");
    }

    end_benchmark();
    vTaskDelete(NULL);
}

void main_app(void)
{
    printf("Begin solo tests...\n");
    xTaskCreate(
        vMasterTask,
        "MasterTask",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);

    vTaskStartScheduler();
}