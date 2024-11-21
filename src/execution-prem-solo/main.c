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

void prefetch(void *arg)
{
    // Memory phase
    union memory_request_answer answer = {.raw = request_memory_access(0, 0)};

    // If no ack, then suspend prefetch
    suspend_prefetch = !answer.ack;

    // We can begin to prefetch, even if suspended it's not a big deal
    prefetch_data_prem((uint64_t)appdata, (uint64_t)data_size, &suspend_prefetch);

    // Release memory
    revoke_memory_access();
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
        init_benchmark(test_name, MEASURE_NANO, 1);

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
    timer_frequency = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    printf("Begin PREM tests...\n");
    xTaskCreate(
        vMasterTask,
        "MasterTask",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}