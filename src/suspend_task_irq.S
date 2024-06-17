.global suspend_task_irq
.type suspend_task_irq, %function
.section .text

/*
 * TODO 
 * void suspend_task_irq(union memory_request_answer*, uint8_t)
 * 
 * Suspends the task and checks that the [memory_access] is given.
 * This is redirected from an IRQ, so this means that the IRQ has
 * overriden the ELR_EL1 and SPSR_EL1 registers... Concretely, the
 * CPU thinks he came back from the IRQ routine and this is the 
 * normal execution flow (when it is not!):
 * - X0-X3 have been restored
 * - Using ret will go back to the PREM task
 * - Flags register is not the same and can cause weird things
 * BUT we are not in an interrupt anymore. That means that we will
 * loop here until an IPI_RESUME is thrown. 
 * 
 * This needs to save X0-X3, wait and at the end restore flags and
 * jump to the prefetch routine (since blocked here means IPI_PAUSE)
 * 
 * x0: union memory_request_answer* memory_access
 * x1: struct saved_registers
 */
.func 
suspend_task_irq:

.endfunc