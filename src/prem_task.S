	.arch armv8-a
	.file	"prem_task.c"
	.text
	.global	memory_access
	.bss
	.align	3
	.type	memory_access, %object
	.size	memory_access, 8
memory_access:
	.zero	8
	.global	hypercalled
	.align	2
	.type	hypercalled, %object
	.size	hypercalled, 4
hypercalled:
	.zero	4
	.global	sysfreq
	.align	3
	.type	sysfreq, %object
	.size	sysfreq, 8
sysfreq:
	.zero	8
	.text
	.align	2
	.global	suspend_task
	.type	suspend_task, %function
suspend_task:
	nop
.L2:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	cmp	x0, 0
	beq	.L2
	nop
	nop
	ret
	.size	suspend_task, .-suspend_task
	.align	2
	.global	wait_for
	.type	wait_for, %function
wait_for:
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	x0, [sp, 24]
	bl	generic_timer_read_counter
	str	x0, [sp, 40]
	ldr	x1, [sp, 40]
	ldr	x0, [sp, 24]
	add	x0, x1, x0
	str	x0, [sp, 32]
	b	.L4
.L5:
	bl	generic_timer_read_counter
	str	x0, [sp, 40]
.L4:
	ldr	x1, [sp, 40]
	ldr	x0, [sp, 32]
	cmp	x1, x0
	bcc	.L5
	nop
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	wait_for, .-wait_for
	.global	saved_registers
	.bss
	.align	3
	.type	saved_registers, %object
	.size	saved_registers, 48
saved_registers:
	.zero	48
	.section	.rodata
	.align	3
.LC0:
	.string	"Content of interrupted x0: %ld\n"
	.align	3
.LC1:
	.string	"Content of interrupted x1: %ld\n"
	.align	3
.LC2:
	.string	"CPU 1 is not in MEMORY_PHASE..."
	.text
	.align	2
	.global	ipi_pause_handler
	.type	ipi_pause_handler, %function
ipi_pause_handler:
	stp	x29, x30, [sp, -80]!
	mov	x29, sp
	str	w0, [sp, 28]
	bl	get_current_state
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	cmp	w0, 2
	bne	.L7
	mov	w0, 1
	bl	change_state
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	xzr, [x0]
	// Start of user assembly
// 79 "prem_task.c" 1
	ldp x3, x2, [sp]
	ldp x1, x0, [sp], #16
	
// 0 "" 2
	// End of user assembly
	str	x3, [sp, 64]
	str	x2, [sp, 56]
	str	x1, [sp, 48]
	str	x0, [sp, 40]
	ldr	x1, [sp, 64]
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	bl	printf
	ldr	x1, [sp, 56]
	adrp	x0, .LC1
	add	x0, x0, :lo12:.LC1
	bl	printf
	bl	suspend_task
	b	.L9
.L7:
	adrp	x0, .LC2
	add	x0, x0, :lo12:.LC2
	bl	puts
.L9:
	nop
	ldp	x29, x30, [sp], 80
	ret
	.size	ipi_pause_handler, .-ipi_pause_handler
	.section	.rodata
	.align	3
.LC3:
	.string	"CPU 1 is not in SUSPENDED state..."
	.text
	.align	2
	.global	ipi_resume_handler
	.type	ipi_resume_handler, %function
ipi_resume_handler:
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	w0, [sp, 28]
	bl	get_current_state
	str	w0, [sp, 44]
	mov	w3, 0
	mov	w2, 0
	mov	w1, 7
	mov	w0, 8
	bl	hypercall
	ldr	w0, [sp, 44]
	cmp	w0, 1
	bne	.L11
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	mov	x1, 1
	str	x1, [x0]
	mov	w0, 2
	bl	change_state
	b	.L12
.L11:
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	puts
.L12:
	mov	w3, 0
	mov	w2, 0
	mov	w1, 7
	mov	w0, 8
	bl	hypercall
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	ipi_resume_handler, .-ipi_resume_handler
	.align	2
	.global	vTaskPREMDelay
	.type	vTaskPREMDelay, %function
vTaskPREMDelay:
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	x0, [sp, 24]
	bl	generic_timer_read_counter
	str	x0, [sp, 40]
	ldr	x1, [sp, 24]
	mov	x0, x1
	lsl	x0, x0, 5
	sub	x0, x0, x1
	lsl	x0, x0, 2
	add	x0, x0, x1
	lsl	x0, x0, 3
	mov	x1, x0
	mov	x0, 10661
	movk	x0, 0x6715, lsl 16
	movk	x0, 0x3e1f, lsl 32
	movk	x0, 0xaec3, lsl 48
	umulh	x0, x1, x0
	lsr	x1, x0, 11
	adrp	x0, sysfreq
	add	x0, x0, :lo12:sysfreq
	ldr	x0, [x0]
	mul	x0, x1, x0
	lsr	x1, x0, 3
	mov	x0, 63439
	movk	x0, 0xe353, lsl 16
	movk	x0, 0x9ba5, lsl 32
	movk	x0, 0x20c4, lsl 48
	umulh	x0, x1, x0
	lsr	x0, x0, 4
	ldr	x1, [sp, 40]
	add	x0, x1, x0
	str	x0, [sp, 32]
	b	.L14
.L15:
	bl	generic_timer_read_counter
	str	x0, [sp, 40]
.L14:
	ldr	x1, [sp, 40]
	ldr	x0, [sp, 32]
	cmp	x1, x0
	bcc	.L15
	nop
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	vTaskPREMDelay, .-vTaskPREMDelay
	.section	.rodata
	.align	3
.LC4:
	.string	"Content of x0: %ld\n"
	.align	3
.LC5:
	.string	"Content of x1: %ld\n"
	.text
	.align	2
	.global	vPREMTask
	.type	vPREMTask, %function
vPREMTask:
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
	bl	vTaskSuspendAll
	adrp	x0, hypercalled
	add	x0, x0, :lo12:hypercalled
	ldr	w0, [x0]
	add	w0, w0, 1
	adrp	x1, hypercalled
	add	x1, x1, :lo12:hypercalled
	str	w0, [x1]
	cmp	w0, 1
	bne	.L17
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 24]
	mov	w3, 0
	mov	w2, 0
	mov	w1, w0
	mov	w0, 2
	bl	hypercall
	mov	x1, x0
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	x1, [x0]
.L17:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	cmp	x0, 1
	beq	.L18
	b	.L19
.L20:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	lsr	x0, x0, 1
	bl	wait_for
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 24]
	mov	w3, 0
	mov	w2, 0
	mov	w1, w0
	mov	w0, 2
	bl	hypercall
	mov	x1, x0
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	x1, [x0]
.L19:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	lsr	x0, x0, 1
	cmp	x0, 0
	bne	.L20
	mov	w0, 1
	bl	change_state
	bl	xTaskResumeAll
	bl	suspend_task
	bl	vTaskSuspendAll
.L18:
	mov	w0, 2
	bl	change_state
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 16]
	mov	x1, x0
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	printf
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 8]
	mov	x1, x0
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	printf
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 16]
	mov	x2, x0
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 8]
	mov	x1, x0
	mov	x0, x2
	bl	prefetch_data
	mov	x0, 300
	bl	vTaskPREMDelay
	mov	w0, 3
	bl	change_state
	mov	w3, 0
	mov	w2, 0
	mov	w1, 0
	mov	w0, 3
	bl	hypercall
	ldr	x0, [sp, 40]
	ldr	x1, [x0]
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 32]
	blr	x1
	adrp	x0, hypercalled
	add	x0, x0, :lo12:hypercalled
	ldr	w0, [x0]
	sub	w0, w0, #1
	adrp	x1, hypercalled
	add	x1, x1, :lo12:hypercalled
	str	w0, [x1]
	cmp	w0, 0
	beq	.L21
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 24]
	mov	w3, 0
	mov	w2, 0
	mov	w1, w0
	mov	w0, 2
	bl	hypercall
	mov	x1, x0
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	x1, [x0]
	b	.L22
.L24:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	lsr	x0, x0, 1
	bl	wait_for
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 24]
	mov	w3, 0
	mov	w2, 0
	mov	w1, w0
	mov	w0, 2
	bl	hypercall
	mov	x1, x0
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	x1, [x0]
.L22:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	ubfx	x0, x0, 0, 1
	and	w0, w0, 255
	and	w0, w0, 1
	cmp	w0, 0
	bne	.L23
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	ldr	x0, [x0]
	lsr	x0, x0, 1
	cmp	x0, 0
	bne	.L24
	b	.L23
.L21:
	adrp	x0, memory_access
	add	x0, x0, :lo12:memory_access
	str	xzr, [x0]
.L23:
	mov	w0, 0
	bl	change_state
	bl	xTaskResumeAll
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	vPREMTask, .-vPREMTask
	.align	2
	.global	xTaskPREMCreate
	.type	xTaskPREMCreate, %function
xTaskPREMCreate:
	stp	x29, x30, [sp, -112]!
	mov	x29, sp
	str	x19, [sp, 16]
	str	x0, [sp, 72]
	str	x1, [sp, 64]
	str	w2, [sp, 60]
	mov	x19, x3
	str	x4, [sp, 48]
	str	x5, [sp, 40]
	mov	x0, 40
	bl	pvPortMalloc
	str	x0, [sp, 104]
	ldr	x0, [sp, 104]
	ldr	x1, [sp, 72]
	str	x1, [x0]
	ldr	x1, [x19, 8]
	ldr	x0, [sp, 104]
	str	x1, [x0, 8]
	ldr	x1, [x19, 16]
	ldr	x0, [sp, 104]
	str	x1, [x0, 16]
	mov	w3, 0
	mov	w2, 0
	mov	w1, 0
	mov	w0, 4
	bl	hypercall
	mov	w1, w0
	ldr	x0, [sp, 104]
	str	w1, [x0, 24]
	ldr	x1, [x19, 24]
	ldr	x0, [sp, 104]
	str	x1, [x0, 32]
	ldr	x0, [x19]
	str	x0, [sp, 88]
	ldr	x0, [sp, 104]
	str	x0, [sp, 96]
	ldr	x6, [sp, 40]
	ldr	x5, [sp, 48]
	ldp	x3, x4, [sp, 88]
	ldr	w2, [sp, 60]
	ldr	x1, [sp, 64]
	adrp	x0, vPREMTask
	add	x0, x0, :lo12:vPREMTask
	bl	xTaskPeriodicCreate
	nop
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 112
	ret
	.size	xTaskPREMCreate, .-xTaskPREMCreate
	.align	2
	.global	vInitPREM
	.type	vInitPREM, %function
vInitPREM:
	stp	x29, x30, [sp, -16]!
	mov	x29, sp
	adrp	x0, ipi_pause_handler
	add	x1, x0, :lo12:ipi_pause_handler
	mov	w0, 6
	bl	irq_set_handler
	mov	w0, 6
	bl	irq_enable
	mov	w1, 0
	mov	w0, 6
	bl	irq_set_prio
	adrp	x0, ipi_resume_handler
	add	x1, x0, :lo12:ipi_resume_handler
	mov	w0, 7
	bl	irq_set_handler
	mov	w0, 7
	bl	irq_enable
	mov	w1, 0
	mov	w0, 7
	bl	irq_set_prio
	bl	generic_timer_get_freq
	mov	x1, x0
	adrp	x0, sysfreq
	add	x0, x0, :lo12:sysfreq
	str	x1, [x0]
	nop
	ldp	x29, x30, [sp], 16
	ret
	.size	vInitPREM, .-vInitPREM
	.ident	"GCC: (Arm GNU Toolchain 13.2.rel1 (Build arm-13.7)) 13.2.1 20231009"
