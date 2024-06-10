#ifndef __GENERIC_TIMER_H__
#define __GENERIC_TIMER_H__

#include <FreeRTOS.h>
#include <projdefs.h>

/* 
 * Converts the time in systicks to a time in nanoseconds. Be careful, this
 * result will depend on the frequency of your clock and won't be nano-second 
 * precise. For example, the Cortex A72 in the Raspberry Pi 4 has a base
 * frequency of 62,5 MHz, so ticking at 16 nanoseconds... 
 */
#define pdSYSTICK_TO_NS(sysfreq, tickCounter) ((((uint64_t) tickCounter * (uint64_t) 1000000000) / (uint64_t) sysfreq))

/* 
 * Converts the time in systicks to a time in microseconds. Your results can
 * be troncated a lot if the tickCounter number you put is too small, if it
 * is the case use pdSYSTICK_TO_NS
 */
#define pdSYSTICK_TO_US(sysfreq, tickCounter) (((uint64_t) tickCounter * (uint64_t) 1000000) / (uint64_t) sysfreq)

/* 
 * Converts the time in systicks to a time in FreeRTOS ticks. The result will
 * surely be truncated, but it is the price to pay to be able to delay a task
 * for an amount of time determined by the generic timer. You can of course 
 * do +1 to be sure that you don't underwait 
 */
#define pdSYSTICK_TO_TICK(sysfreq, tickCounter) (pdMS_TO_TICKS(((uint64_t) tickCounter * (uint64_t) 1000) / (uint64_t) sysfreq))

/* Returns the frequency of the generic timer */
uint64_t generic_timer_get_freq(void);

/* Returns the count of the generic timer */
uint64_t generic_timer_read_counter(void);

#endif