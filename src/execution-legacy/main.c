#include <FreeRTOS.h>
#include <periodic_task.h>

#include <plat.h>
#include <ipi.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <benchmark.h>

#define NUMBER_OF_TESTS 900

// Change location for appdata
#define DATA_SIZE 448 * 1024
uint8_t appdata[DATA_SIZE] = {1};

void prefetch_memory()
{
    // TODO Create random app data
    prefetch_data((uint64_t)appdata, DATA_SIZE);
}

int counter = 0;
void vTask(void *pvParameters)
{
    if (counter < NUMBER_OF_TESTS)
    {
        // Clear cache
        clear_L2_cache((uint64_t)appdata, DATA_SIZE);

        // Benchmark prefetching
        run_benchmark(prefetch_memory);

        // Increment counter
        counter += 1;
    }
    else
    {
        // Show results and destroy task
        print_benchmark_results();
        vTaskDelete(NULL);
    }
}

void main_app(void)
{
    int frequency = 300;
    init_benchmark(NUMBER_OF_TESTS);

    xTaskPeriodicCreate(
        vTask,
        "ExecutionFPSCHED",
        configMINIMAL_STACK_SIZE,
        pdFREQ_TO_TICKS(frequency),
        NULL,
        tskIDLE_PRIORITY + 1,
        (TaskHandle_t *)NULL);

    vTaskStartScheduler();
}