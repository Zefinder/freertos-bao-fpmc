#include <prefetch_inc.h>

.global clear_L2_cache
.type clear_L2_cache, %function
.section .text

/*
 * void clear_L2_cache(address, size)
 * 
 * Flushes all caches that are in the range [address ; address + size]
 * 
 * x0: uint64_t address
 * x1: uint64_t size 
 */
.func 
clear_L2_cache:
    // Compute end address
    add x1, x0, x1  // x1 = address + size

    // Get cache line length of the executing core
    mrs x3, ctr_el0                                 // Getting CTR_EL0 value
    ubfx x3, x3, #D_MIN_LINE_OFF, #D_MIN_LINE_LEN   // Extracting DminLine from CTR_EL0 (log2 of min word number of cache line)
    mov x2, #WORD_SIZE                              // x2 = word size for getting MIN_LINE_SIZE
    lsl x2, x2, x3                                  // x2 is now MIN_LINE_SIZE
    sub x3, x2, #1                                  // x3 is now MIN_LINE_SIZE mask
    bic x0, x0, x3                                  // x0 &= ~x3 (mask start address)

flush_loop:
    dc civac, x0    // Flush lines that contained address x0
    add x0, x0, x2  // x0 += MIN_LINE_SIZE
    cmp x0, x1      // Comparing flushed address to end address
    blt flush_loop  // If x0 < x1 continue

end_flush:
    dsb sy  // Data sync barrier
    ret
.endfunc

.global prefetch_data
.type prefetch_data, %function
.section .text

/*
 * void prefetch_data(address, size)
 * 
 * Prefetches [size] bytes from address [address]
 * 
 * x0: uint64_t address
 * x1: uint64_t size 
 */
.func 
prefetch_data:
    lsr x1, x1, #LOG2_L2_CACHE_LINE_SIZE    // x1 is now the remaining number of lines to prefetch

prefetch_loop:
    prfm PLDL2KEEP, [x0]            // Prefetch L2 cache from address [x0 & ~(L2_CACHE_LINE_SIZE - 1) ; x0 | (L2_CACHE_LINE_SIZE - 1)]
    add x0, x0, #L2_CACHE_LINE_SIZE // x0 += L2_CACHE_LINE_SIZE
    subs x1, x1, #1                 // Decrease remaining lines
    bne prefetch_loop               // If x1 > 0 continue

end_prefetch:
    ret
.endfunc