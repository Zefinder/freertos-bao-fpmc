#include <FreeRTOS.h>
#include <generic_timer.h>

uint64_t generic_timer_get_freq(void)
{
    uint64_t cntfrq;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    return cntfrq;
}

uint64_t generic_timer_read_counter(void)
{
	unsigned long cntpct;

    // Flush instruction pipeline as counter can be read before increment
    __asm__ volatile("isb");
	__asm__ volatile("mrs %0, cntpct_el0" : "=r" (cntpct));

	return cntpct;
}

