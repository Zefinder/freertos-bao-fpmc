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
#include <benchmark.h>

// Request the default IPI and memory request wait
#ifdef DEFAULT_IPI_HANDLERS
#error DEFAULT_IPI must not be defined with value y for this test (make all ... DEFAULT_IPI=y is no!)
#endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 90000
#define DATA_SIZE MAX_DATA_SIZE
#define MIN_DATA_SIZE 1 kB
#define DATA_SIZE_INCREMENT 1 kB

// Generic timer frequency
uint64_t timer_frequency;

int counter = 0;
int print_counter = 0;
uint64_t data_size = 0;
volatile uint8_t suspend_prefetch = 0;

void clear_cache(void* arg)
{
    // clear_L2_cache_CISW((uint64_t)0, (uint64_t)15, (uint64_t)0, (uint64_t)(((0x100000 / 16) / 4) - 1));
    clear_L2_cache((uint64_t)appdata, 1024 kB);
}

void vMasterTask(void *pvParameters)
{
    start_benchmark();
    printf("\t# Number of realised tests: %d\n", counter);
    clear_L2_cache((uint64_t)appdata, (uint64_t)DATA_SIZE);

    for (data_size = MIN_DATA_SIZE; data_size <= DATA_SIZE; data_size += DATA_SIZE_INCREMENT)
    {
        // Init benchmark
        int data_size_ko = BtkB(data_size);
        char test_name[20];
        sprintf(test_name, "_%dkB", data_size_ko);
        init_benchmark(test_name, MEASURE_NANO);

        while (counter++ < NUMBER_OF_TESTS)
        {
            // Prefetch data and measure cache clear
            prefetch_data_prem((uint64_t)appdata, (uint64_t)data_size, &suspend_prefetch);
            run_benchmark(clear_cache, NULL);

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

struct premtask_parameters premtask_parameters;
void main_app(void)
{
    timer_frequency = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    printf("Begin cache clear tests...\n");
    xTaskCreate(
        vMasterTask,
        "MasterTask",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}