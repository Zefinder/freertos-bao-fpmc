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

.global clear_L2_cache_CISW
.type clear_L2_cache_CISW, %function
.section .text

/*
 * void clear_L2_cache_CISW(start_way, end_way, start_set, end_set)
 */
.func 
clear_L2_cache_CISW:
#define StartWay x0
#define EndWay x1
#define StartSet x2
#define EndSet x3
#define Level  x9
#define SetWay x10
#define Ways   x11
#define Sets   x12

	MOV   Level, #2
	MOV   Ways, EndWay
clear_cache_partition_L2_loop_reset_sets:
	MOV   Sets, EndSet
clear_cache_partition_L2_loop:
	ADD   SetWay, Level, Ways, LSL #28  // create new bitfield: level and ways
	ADD   SetWay, SetWay, Sets, LSL #6 // create new bitfield: add sets
	DC    CISW, SetWay                        // clear invalidate set way
	CMP   Sets, StartSet
	Bls   clear_cache_partition_L2_dec_way    // if Sets == 0: all the sets cleared -> next way
	SUB   Sets, Sets, #1                      // next set
	B     clear_cache_partition_L2_loop
clear_cache_partition_L2_dec_way:
	CMP   Ways, StartWay
	Bls   clear_cache_partition_L2_end_loop   // ways are 0 so we are done
	SUB   Ways, Ways, #1                      // next way
	B     clear_cache_partition_L2_loop_reset_sets
clear_cache_partition_L2_end_loop:

	/* flush pipeline */
	dsb   SY
	isb
	ret

#undef StartWay
#undef EndWay
#undef StartSet
#undef EndSet
#undef Level
#undef SetWay
#undef Ways
#undef Sets
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
    bne prefetch_loop               // If x1 > 0 loop

end_prefetch:
    ret
.endfunc

.global prefetch_data_prem
.type prefetch_data_prem, %function
.section .text

/*
 * void prefetch_data_prem(address, size, suspend_prefetch)
 * 
 * Prefetches [size] bytes from address [address]
 * Uses [suspend_prefetch] to know if it is possible to
 * continue the prefetch. If not, then waits for an int
 * (since IPI_RESUME will be the one that will give access
 * to memory) and rechecks. 
 * 
 * x0: uint64_t address
 * x1: uint64_t size 
 * x2: uint8_t* suspend_prefetch
 */
.func 
prefetch_data_prem:
    lsr x1, x1, #LOG2_L2_CACHE_LINE_SIZE    // x1 is now the remaining number of lines to prefetch
    b wait_for_int                          // First check if can prefetch

prefetch_loop_prem:
    prfm PLDL2KEEP, [x0]            // Prefetch L2 cache from address [x0 & ~(L2_CACHE_LINE_SIZE - 1) ; x0 | (L2_CACHE_LINE_SIZE - 1)]
    add x0, x0, #L2_CACHE_LINE_SIZE // x0 += L2_CACHE_LINE_SIZE
    subs x1, x1, #1                 // Decrease remaining lines
    beq end_prefetch_prem           // If x1 == 0 end

wait_for_int:
    eor x3, x3, x3          // Reset x3
	ldrb w3, [x2]			// Load suspend prefetch (only the byte!)
	cmp x3, #0				// Compare if paused
	beq prefetch_loop_prem	// If suspend_prefetch == 0 then we can continue
    wfi                     // Else wait for next interrupt
    b wait_for_int          // And recheck (maybe it was IPI_RESUME)

end_prefetch_prem:
    ret
.endfunc