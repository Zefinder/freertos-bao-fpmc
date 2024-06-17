.global suspend_task_irq
.type suspend_task_irq, %function
.section .text

/*
 * void suspend_task_irq()
 * 
 * Just endlessly stop the CPU until an IRQ occurs...
 * The point is to never stay here for ever and once a
 * IPI_RESUME is catched we don't return here.
 * 
 * This must NOT use registers nor the stack to have the
 * cleanest return ever!
 */
.func 
suspend_task_irq:
	wfi 
	b suspend_task_irq

.endfunc