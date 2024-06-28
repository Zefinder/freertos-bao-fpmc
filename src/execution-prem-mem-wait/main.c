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
#error DEFAULT_IPI must not be defined with value y for this test (make all ... DEFAULT_IPI=n)
#endif
#ifndef MEMORY_REQUEST_WAIT
#error MEMORY_REQUEST_WAIT must be defined for this benchmark, (make all ... MEMORY_REQUEST_WAIT=y)
#endif

#if defined(PLATFORM) && PLATFORM == rpi4
    // For Raspberry 4 model B, the model shows that the equation y=330x+1257ns is a good wcet approximation for solo prefetch 
    #define TTW(timer_frequency, prefetch_size) pdNS_TO_SYSTICK(timer_frequency, (uint64_t)(330 * BtkB((uint64_t)prefetch_size) + 1257 + 25000))
#else 
    #define TTW(timer_frequency, prefetch_size) (0) 
#endif

// Task handler, change to array for multiple tasks in the future
TaskHandle_t xTaskHandler;

#define NUMBER_OF_TESTS 90000
#define DATA_SIZE_0 100 kB
#define DATA_SIZE_INTERFERENCE 40 kB
#define MAX_PRIO 6

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
    // Change state to receive IPI 
    change_state(MEMORY_PHASE);

    // Memory phase
    union memory_request_answer answer = {.raw = request_memory_access(prio)};

    // If no ack then test if need to wait
    if (answer.ack == 0)
    {
        // Time to wait not 0, so we wait before asking again
        if (answer.ttw != 0)
        {
            vTaskPREMDelay(answer.ttw);
            // From here we know for sure that we waited enough! (Add a barrier?)
            answer.raw = request_memory_access(prio);
            suspend_prefetch = !answer.ack; // if ack is 1, then no suspend!
        }
        else
        {
            // No waiting was needed so just suspend
            suspend_prefetch = 1;
        }
    }

    // We can begin to prefetch, even if suspended it's not a big deal
    clear_L2_cache((uint64_t)appdata, (uint64_t)arg);
    prefetch_data_prem((uint64_t)appdata, (uint64_t)arg, &suspend_prefetch);

    // Changing state and release memory
    change_state(WAITING);
    revoke_memory_access();
}

void vMasterTask(void *pvParameters)
{
    start_benchmark();

    while (prio < MAX_PRIO + 1)
    {
        // Init benchmark
        char test_name[20];
        sprintf(test_name, "_%dkB_prio%d_interference%dkB", BtkB(DATA_SIZE_0), prio, BtkB(DATA_SIZE_INTERFERENCE));
        init_benchmark(test_name, MEASURE_NANO);

        while (counter++ < NUMBER_OF_TESTS)
        {
            run_benchmark(PREMPart, (void *)(DATA_SIZE_0));

            // Print for each 1000 tests
            if (++print_counter == 1000)
            {
                printf("\t# Number of realised tests: %d\n", counter);
                print_counter = 0;
            }

            // Wait for 3 x worst case of prefetching DATA_SIZE_0 kB
            vTaskPREMDelay(3 * TTW(timer_frequency, DATA_SIZE_0));
        }

        // Reset counters
        counter = 0;
        print_counter = 0;
        prio += 2;

        // Print results
        print_benchmark_results();
        printf("\n");
    }

    end_benchmark();
    vTaskDelete(NULL);
}

void vTaskInterference(void *pvParameters)
{
    // Always do it
    while (1)
    {
        PREMPart(pvParameters);

        // Computation phase will be the sum of all elements in the array
        uint64_t sum = 0;
        for (int i = 0; i < DATA_SIZE_INTERFERENCE; i++)
        {
            sum += appdata[i];
        }
    }
}

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
        prio = 2 * cpu_id - 1;
        xTaskCreate(
            vTaskInterference,
            "Interference",
            configMINIMAL_STACK_SIZE,
            (void *)(DATA_SIZE_INTERFERENCE),
            tskIDLE_PRIORITY + 1,
            NULL);
    }

    vTaskStartScheduler();
}