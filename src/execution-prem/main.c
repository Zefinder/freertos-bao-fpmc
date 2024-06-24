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
#define DATA_SIZE_0 100 kB
#define DATA_SIZE_INTERFERENCE 10 kB
#define MAX_PRIO 0

// Generic timer frequency
uint64_t timer_frequency;

int counter = 0;
int print_counter = 0;
uint32_t prio = 0;
volatile uint8_t suspend_prefetch = 0;

void ipi_pause_handler(unsigned int id)
{
    enum states current_state = get_current_state();
    if (current_state == MEMORY_PHASE)
    {
        // Pause task (we already are in prefetch)
        change_state(SUSPENDED);
        suspend_prefetch = 1;
    }
}

void ipi_resume_handler(unsigned int id)
{
    enum states current_state = get_current_state();
    if (current_state == SUSPENDED)
    {
        // Resume task
        suspend_prefetch = 0;
        change_state(MEMORY_PHASE);
    }
}

void PREMPart(void *arg)
{
    // Memory phase
    union memory_request_answer answer = {.raw = request_memory_access(prio)};

    // If no ack, then suspend prefetch
    if (answer.ack == 0)
    {
        suspend_prefetch = 1;
    }

    // We can begin to prefetch, even if suspended it's not a big deal
    clear_L2_cache((uint64_t)appdata, (uint64_t)DATA_SIZE_0);
    prefetch_data_prem((uint64_t)appdata, (uint64_t)DATA_SIZE_0, &suspend_prefetch);

    // Release memory
    revoke_memory_access();
}

void vMasterTask(void *pvParameters)
{
    start_benchmark();

    while (prio < MAX_PRIO + 1)
    {
        // Init benchmark
        char test_name[20];
        sprintf(test_name, "_%dkB_prio%d", DATA_SIZE_0, prio);
        init_benchmark(test_name, MEASURE_NANO);

        while (counter++ < NUMBER_OF_TESTS)
        {
            run_benchmark(PREMPart, NULL);

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
        prio += 2;
    }

    print_benchmark_results();
    printf("\n");
    end_benchmark();
    vTaskDelete(NULL);
}

void vTaskInterference(void *pvParameters)
{
    struct premtask_parameters *premtask_parameters = (struct premtask_parameters *)pvParameters;
    uint32_t prio = *premtask_parameters->priority;
    uint8_t *appdata = premtask_parameters->data;
    uint64_t data_size = premtask_parameters->data_size;

    // Always do it
    while (1)
    {
        // Memory phase
        union memory_request_answer answer = {.raw = request_memory_access(prio)};

        // If no ack, then suspend prefetch
        if (answer.ack == 0)
        {
            suspend_prefetch = 1;
        }

        // We can begin to prefetch, even if suspended it's not a big deal
        clear_L2_cache((uint64_t)appdata, (uint64_t)data_size);
        prefetch_data_prem((uint64_t)appdata, (uint64_t)data_size, &suspend_prefetch);

        // Release memory
        revoke_memory_access();

        // Computation phase will be the sum of all elements in the array
        uint64_t sum = 0;
        for (int i = 0; i < data_size; i++)
        {
            sum += appdata[i];
        }
    }
}

struct premtask_parameters premtask_parameters;
void main_app(void)
{
    timer_frequency = generic_timer_get_freq();
    uint64_t cpu_id = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    // IPI for everyone
    // Enable IPI pause
    irq_set_handler(IPI_IRQ_PAUSE, ipi_pause_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    // Enable IPI resume
    irq_set_handler(IPI_IRQ_RESUME, ipi_resume_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    if (cpu_id == 0)
    {
        printf("Begin PREM tests...\n");
        xTaskCreate(
            vMasterTask,
            "MasterTask",
            configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY + 1,
            &(xTaskHandler));
    }
    else
    {
        // Just interferences, the goal is to interfere, give the parameters to fetch... (cpu_id and prefetch offset)
        // From 120kB to 130kB for CPU 1
        // From 140kB to 150kB for CPU 2
        // From 160kB to 170kB for CPU 3
        prio = 2 * cpu_id - 1;
        premtask_parameters.tickPeriod = 0;
        premtask_parameters.data_size = DATA_SIZE_INTERFERENCE;
        premtask_parameters.data = appdata + DATA_SIZE_0 + 2 * cpu_id * DATA_SIZE_INTERFERENCE;
        premtask_parameters.priority = &prio;
        premtask_parameters.pvParameters = (int *)(DATA_SIZE_0 + 2 * cpu_id * DATA_SIZE_INTERFERENCE);
        xTaskPREMCreate(
            vTaskInterference,
            "Interference",
            configMINIMAL_STACK_SIZE,
            premtask_parameters,
            tskIDLE_PRIORITY + 1,
            NULL);
    }

    vTaskStartScheduler();
}