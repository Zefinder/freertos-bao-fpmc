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

uint64_t cpuid;
volatile uint8_t ipi_received;
uint64_t end_ipi_time;
volatile uint64_t answer;

void ipi_id_handler(unsigned int id)
{
    end_ipi_time = generic_timer_read_counter();
    ipi_received = 1;
}

void ipi_resume_handler(unsigned int id)
{
    answer = 1;
}

void empty_handler(unsigned int id)
{
    end_ipi_time = generic_timer_read_counter();
    ipi_received = 1;
}

void bench_hypercall(void *benchmark_argument)
{
    // Make an empty hypercall just to measure both ways
    hypercall(HC_EMPTY_CALL, 0, 0, 0);
}

void vHypercallTask(void *pvParameters)
{
    int counter = 0;
    int print_counter = 0;

    init_benchmark("_hypercall", MEASURE_NANO);
    while (counter < NUMBER_OF_TESTS)
    {
        // Benchmark prefetching
        run_benchmark(bench_hypercall, NULL);

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

void vIPITask(void *pvParameters)
{
    uint64_t base_frequency = generic_timer_get_freq();

    // Redo benchmarking here since we don't measure time of an OS function
    int counter = 0;
    int print_counter = 0;

    uint64_t max_time = 0;
    uint64_t min_time = -1;
    uint64_t sum = 0;

    // Init benchmark string
    printf("elapsed_time_array_ipi_ns = [");

    while (counter < NUMBER_OF_TESTS)
    {
        counter += 1;

        // Send the hypercall to measure IPI, the result is the start time
        uint64_t start_ipi_time = hypercall(HC_MEASURE_IPI, 0, 0, 0);

        // Wait for IPI (if sending an IPI is immediate, should be 0 time but not the case)
        while (!ipi_received)
            ;

        // Reset IPI received
        ipi_received = 0;

        // Compute the time taken by the IPI
        uint64_t ipi_time = end_ipi_time - start_ipi_time;

        /* Benchmark stuff */
        // Increase sum
        sum += ipi_time;

        // Test min and max
        if (ipi_time > max_time)
        {
            max_time = ipi_time;
        }

        if (ipi_time < min_time)
        {
            min_time = ipi_time;
        }

        // Print the result
        printf("%ld,", pdSYSTICK_TO_NS(base_frequency, ipi_time));
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
    printf("min_ipi_ns = %ld # ns\n", pdSYSTICK_TO_NS(base_frequency, min_time));
    printf("max_ipi_ns = %ld # ns\n", pdSYSTICK_TO_NS(base_frequency, max_time));
    printf("int_average_ipi_ns = %ld # ns\n", pdSYSTICK_TO_NS(base_frequency, average_time));
    printf("\n");

    vTaskDelete(NULL);
}

void vArbitrationRequestTask(void *pvParameters)
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

void vArbitrationRevokeTask(void *pvParameters)
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
        printf("elapsed_time_array_revoke_prio%d_ns = [", prio);

        while (counter < NUMBER_OF_TESTS)
        {
            counter += 1;

            // Ask memory access wait for access
            answer = request_memory_access(prio);

            while(!answer)
                ;

            // Revoke and measure it
            uint64_t arbitration_time = hypercall(HC_REVOKE_MEM_ACCESS_TIMER, prio, 0, 0);

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
        printf("int_average_revoke_prio%d_ns = %ld # ns\n", prio, pdSYSTICK_TO_NS(base_frequency, average_time));
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
    irq_set_handler(IPI_IRQ_ID, ipi_id_handler);
    irq_enable(IPI_IRQ_ID);
    irq_set_prio(IPI_IRQ_ID, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_PAUSE, empty_handler);
    irq_enable(IPI_IRQ_PAUSE);
    irq_set_prio(IPI_IRQ_PAUSE, IRQ_MAX_PRIO);

    irq_set_handler(IPI_IRQ_RESUME, ipi_resume_handler);
    irq_enable(IPI_IRQ_RESUME);
    irq_set_prio(IPI_IRQ_RESUME, IRQ_MAX_PRIO);

    // Get the CPU id
    cpuid = hypercall(HC_GET_CPU_ID, 0, 0, 0);

    printf("Begin microbench tests...\n");
    start_benchmark();

    // Create tasks
    xTaskCreate(
        vHypercallTask,
        "Microbench hypercall",
        configMINIMAL_STACK_SIZE,
        (void *)&hypercall_task,
        tskIDLE_PRIORITY + 5,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vIPITask,
        "Microbench IPI",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 4,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vArbitrationRequestTask,
        "Microbench request",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 3,
        (TaskHandle_t *)NULL);

    xTaskCreate(
        vArbitrationRevokeTask,
        "Microbench revoke",
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