#ifndef __GENERIC_TIMER_H__
#define __GENERIC_TIMER_H__

#include <FreeRTOS.h>

/* Converts the time in systicks to a time in nanoseconds. Be careful, this
 * result will depend on the frequency of your clock and won't be nano-second 
 * precise. For example, the Cortex A72 in the Raspberry Pi 4 has a base
 * frequency of 62,5 MHz, so ticking at 16 nanoseconds... 
 */
#define pdSYSTICK_TO_NS(sysfreq, tickCounter) ((((uint64_t) tickCounter * (uint64_t) 1000000000) / (uint64_t) sysfreq))

/* Converts the time in systicks to a time in microseconds. Your results can
 * be troncated a lot if the tickCounter number you put is too small, if it
 * is the case use pdSYSTICK_TO_NS */
#define pdSYSTICK_TO_US(sysfreq, tickCounter) (((uint64_t) tickCounter * (uint64_t) 1000000) / (uint64_t) sysfreq)

/* Returns the frequency of the generic timer */
uint64_t generic_timer_get_freq(void);

/* Returns the count of the generic timer */
uint64_t generic_timer_read_counter(void);

#endif