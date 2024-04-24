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

#define CPU_MASTER 1
#define NUMBER_OF_TESTS 90000

// Change location for appdata
#define DATA_SIZE 200 * 1024
uint8_t appdata[DATA_SIZE] = {1};

// CPU number to determine which CPU has the priority
int cpu_number = -1;

void prefetch_memory()
{
    // TODO Create random app data
    prefetch_data((uint64_t)appdata, DATA_SIZE);
}

int counter = 0;
int print_counter = 0;
void vTask(void *pvParameters)
{
    if (counter < NUMBER_OF_TESTS * (cpu_number + 1))
    {
        // Clear and prefetch
        clear_L2_cache((uint64_t)appdata, DATA_SIZE);

        if (cpu_number == CPU_MASTER)
        {
            run_benchmark(prefetch_memory);
        }
        else
        {
            prefetch_memory();
        }

        // Increment counter
        counter += 1;

        // Print for each 1000 tests
        if (++print_counter == 1000 && cpu_number == CPU_MASTER)
        {
            printf("\t# Number of realised tests: %d\n", counter);
            print_counter = 0;
        }
    }
    else
    {
        // Show results and destroy task
        if (cpu_number == CPU_MASTER)
        {
            print_benchmark_results();
            printf("Tests finished!\n");
        }
        vTaskDelete(NULL);
    }
}

void main_app(void)
{
    cpu_number = hypercall(HC_GET_CPU_ID, 0, 0, 0);
    int frequency = 150 * (cpu_number + 1);

    if (cpu_number == CPU_MASTER)
    {
        printf("Begin 2 tasks tests (mesured on %d Hz processor)...\n", frequency);
        init_benchmark(NULL);
    }

    xTaskPeriodicCreate(
        vTask,
        "Execution2Tasks",
        configMINIMAL_STACK_SIZE,
        pdFREQ_TO_TICKS(frequency),
        NULL,
        tskIDLE_PRIORITY + 1,
        &(xTaskHandler));

    vTaskStartScheduler();
}