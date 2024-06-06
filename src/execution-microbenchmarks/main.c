#include <FreeRTOS.h>
#include <periodic_task.h>

#include <plat.h>
#include <irq.h>

#include <stdio.h>

#include <hypervisor.h>
#include <prefetch.h>
#include <state_machine.h>
#include <benchmark.h>
#include <ipi.h>
#include <data.h>
#include <generic_timer.h>

#define NUMBER_OF_TESTS 1000000
#ifdef configUSE_PREEMPTION
    #if configUSE_PREEMPTION == 1
        #error Preemption must be disabled for this test... put configUSE_PREEMPTION to 0 in FreeRTOSConfig.h
    #endif
#endif

struct task_struct
{
    char name[15];
    benchmark_t benchmark_function;
};
uint64_t cpuid;
volatile uint8_t ipi_received;

void empty_handler()
{
}

void ipi_handler()
{
    ipi_received = 1;
}

void bench_hypercall(void *benchmark_argument)
{
    // Make an empty hypercall just to measure both ways
    hypercall(HC_EMPTY_CALL, 0, 0, 0);
}

void bench_ipi(void *benchmark_argument)
{
    // Send an IPI with the core (and not the hypervisor, too long!)
    irq_send_ipi(0b0001);

    // Wait for IPI (if sending an IPI is immediate, should be 0 time but not the case)
    while (!ipi_received)
        ;

    // Reset IPI received
    ipi_received = 0;
}

void vTask(void *pvParameters)
{
    int counter = 0;
    int print_counter = 0;

    struct task_struct *task_params = (struct task_struct *)pvParameters;
    char *name = task_params->name;
    benchmark_t benchmark_function = task_params->benchmark_function;

    init_benchmark(name, MEASURE_NANO);
    while (counter < NUMBER_OF_TESTS)
    {
        // Benchmark prefetching
        run_benchmark(benchmark_function, NULL);

        // Increment counter
        counter += 1;

        // Print for each 1000 tests
        if (++print_counter == 1000)
        {
            printf("\t# Number of realised tests: %d\n", counter);
            print_counter = 0;
        }
    }

    // Show results and destroy task
    print_benchmark_results();
    printf("\n");
    vTaskDelete(NULL);
}

void vArbitrationTask(void *pvParameters)
{
    uint64_t base_frequency = generic_timer_get_freq();

    // Redo benchmarking here since we don't measure time of an OS function
    for (int prio = 0; prio <= 6; prio += 2)
    {
        int counter = 0;
        int print_counter = 0;

        uint64_t max_time = 0;
        uint64_t min_time = -1;
        uint64_t sum = 0;

        // Init benchmark string
        printf("elapsed_time_array_prio%d_ns = [", prio);

        while (counter < NUMBER_OF_TESTS)
        {
            counter += 1;

            // Ask for memory but only get the timing, not the answer!
            uint64_t arbitration_time = hypercall(HC_REQUEST_MEM_ACCESS_TIMER, prio, 0, 0);

            // Revoke memory access
            revoke_memory_access();

            /* Benchmark stuff */
            // Increase sum
            sum += arbitration_time;

            // Test min and max
            if (arbitration_time > max_time)
            {
                max_time = arbitration_time;
            }

            if (arbitration_time < min_time)
            {
                min_time = arbitration_time;
            }

            // Print the result
            printf("%ld,", pdSYSTICK_TO_NS(base_frequency, arbitration_time));
			if (++print_counter == 1000)
			{
				printf("\t# Number of realised tests: %d\n", counter);
				print_counter = 0;
			}
        }

        // Compute average
        uint64_t average_time = sum / NUMBER_OF_TESTS;

        // Print results
        printf("]\n"); // End of array
        printf("min_prio%d_ns = %ld # ns\n", prio, pdSYSTICK_TO_NS(base_frequency, min_time));
        printf("max_prio%d_ns = %ld # ns\n", prio, pdSYSTICK_TO_NS(base_frequency, max_time));
        printf("int_average_prio%d_ns = %ld # ns\n", prio, pdSYSTICK_TO_NS(base_frequency, average_time));
        printf("\n");
    }

    printf("\n");
    vTaskDelete(NULL);
}

void vTaskEndBench(void *pvParameters)
{
    end_benchmark();
    vTaskDelete(NULL);
}

void main_app(void)
{
    // Enable interrupts for IPI test
    irq_set_handler(IPI_IRQ_ID, ipi_handler);
    irq_enable(IPI_IRQ_ID);
    irq_set_prio(IPI_IRQ_ID, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_PAUSE, empty_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_RESUME, empty_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    // Get the CPU id
    cpuid = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    // Create structs for tasks
    struct task_struct hypercall_task = {.name = "_hypercall", .benchmark_function = bench_hypercall};
    struct task_struct ipi_task = {.name = "_ipi", .benchmark_function = bench_ipi};

    printf("Begin microbench tests...\n");
    start_benchmark();

    // Create tasks
    xTaskCreate(
        vTask,
        "Microbench hypercall",
        configMINIMAL_STACK_SIZE,
        (void *)&hypercall_task,
        tskIDLE_PRIORITY + 4,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vTask,
        "Microbench IPI",
        configMINIMAL_STACK_SIZE,
        (void *)&ipi_task,
        tskIDLE_PRIORITY + 3,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vArbitrationTask,
        "Microbench IPI",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vTaskEndBench,
        "End bench",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        (TaskHandle_t *)NULL);

    vTaskStartScheduler();
}