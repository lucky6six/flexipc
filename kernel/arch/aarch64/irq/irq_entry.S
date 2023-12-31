#include <common/asm.h>
#include <common/debug.h>
#include <arch/machine/registers.h>

#include "irq_entry.h"
#include "esr.h"

.extern syscall_table
.extern hook_syscall
.extern finish_switch

.macro	exception_entry	label
	/* Each entry should be 0x80 aligned */
	.align	7
	b	\label
.endm

.macro handle_entry el, type
	exception_enter
	mov	x0, #\type
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	handle_entry_c
	exception_exit
.endm

/* See more details about the bias in registers.h */
.macro	exception_enter
	sub	sp, sp, #ARCH_EXEC_CONT_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	mrs	x21, sp_el0
	mrs	x22, elr_el1
	mrs	x23, spsr_el1
	stp	x30, x21, [sp, #16 * 15]
	stp	x22, x23, [sp, #16 * 16]
.endm

.macro	exception_exit
	ldp	x22, x23, [sp, #16 * 16]
	ldp	x30, x21, [sp, #16 * 15] 
	msr	sp_el0, x21
	msr	elr_el1, x22
	msr	spsr_el1, x23
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]

	add	sp, sp, #ARCH_EXEC_CONT_SIZE
	eret
.endm

/*
 * Vecotr table offsets from vector table base address from ARMv8 Manual
 *	Address		|	Exception Type		| 	Description
 * ============================================================================
 *	VBAR_Eln+0x000	|	 Synchronous		|	 SPSel=0
 * 		+0x080	|	  IRQ/vIRQ		|	Current EL
 *		+0x100	|	  FIQ/vFIQ		|   with Stack Pointer
 * 		+0x180	|	SError/vSError		|    shared with EL0
 * ============================================================================
 *	VBAR_Eln+0x200	|	 Synchronous		|	 SPSel=1
 * 		+0x280	|	  IRQ/vIRQ		|	Current EL
 *		+0x300	|	  FIQ/vFIQ		|   with dedicated
 * 		+0x380	|	SError/vSError		|    Stack Pointer
 * ============================================================================
 *	VBAR_Eln+0x400	|	 Synchronous		|
 * 		+0x480	|	  IRQ/vIRQ		|	Lower EL
 *		+0x500	|	  FIQ/vFIQ		|    using AArch64
 * 		+0x580	|	SError/vSError		|
 * ============================================================================
 *	VBAR_Eln+0x600	|	 Synchronous		|
 * 		+0x680	|	  IRQ/vIRQ		|     	Lower EL
 *		+0x700	|	  FIQ/vFIQ		|    using AArch32
 * 		+0x780	|	SError/vSError		|
 * ============================================================================
 */

.align	11
EXPORT(el1_vector)
	exception_entry	sync_el1t		// Synchronous EL1t
	exception_entry	irq_el1t		// IRQ EL1t
	exception_entry	fiq_el1t		// FIQ EL1t
	exception_entry	error_el1t		// Error EL1t

	exception_entry	sync_el1h		// Synchronous EL1h
	exception_entry	irq_el1h		// IRQ EL1h
	exception_entry	fiq_el1h		// FIQ EL1h
	exception_entry	error_el1h		// Error EL1h

	exception_entry	sync_el0_64		// Synchronous 64-bit EL0
	exception_entry	irq_el0_64		// IRQ 64-bit EL0
	exception_entry	fiq_el0_64		// FIQ 64-bit EL0
	exception_entry	error_el0_64		// Error 64-bit EL0

	exception_entry	sync_el0_32		// Synchronous 32-bit EL0
	exception_entry	irq_el0_32		// IRQ 32-bit EL0
	exception_entry	fiq_el0_32		// FIQ 32-bit EL0
	exception_entry	error_el0_32		// Error 32-bit EL0

sync_el1t:
	handle_entry	1, SYNC_EL1t

irq_el1t:
	handle_entry 	1, IRQ_EL1t

fiq_el1t:
	handle_entry	1, FIQ_EL1t

error_el1t:
	handle_entry	1, ERROR_EL1t

sync_el1h:
	handle_entry	1, SYNC_EL1h

fiq_el1h:
	handle_entry	1, FIQ_EL1h

error_el1h:
	handle_entry	1, ERROR_EL1h

sync_el0_64:
	/* Since we cannot touch x0-x7, we need some extra work here */
	exception_enter
	mrs	x25, esr_el1
	lsr	x24, x25, #ESR_EL1_EC_SHIFT
	cmp	x24, #ESR_EL1_EC_SVC_64
	b.eq	el0_syscall
	/* Not supported exception */
	mov	x0, SYNC_EL0_64 
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	handle_entry_c
	exception_exit

el0_syscall:

/* hooking syscall: ease tracing or debugging */
#if HOOKING_SYSCALL == ON
	sub	sp, sp, #16 * 8
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	
	mov x0, x8
	bl hook_syscall

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	add	sp, sp, #16 * 8
#endif

	adr	x27, syscall_table		// syscall table in x27
	uxtw	x16, w8				// syscall number in x16
	ldr	x16, [x27, x16, lsl #3]		// find the syscall entry
	blr	x16

	/* Ret from syscall */
	// bl	disable_irq
	str	x0, [sp]
	exception_exit

irq_el1h:
irq_el0_64:
	exception_enter
	bl	handle_irq
	/* should nerver reach here */
	b .

fiq_el0_64:
	handle_entry	0, FIQ_EL0_64

error_el0_64:
	handle_entry	0, ERROR_EL0_64

sync_el0_32:
	handle_entry	0, SYNC_EL0_32

irq_el0_32:
	handle_entry	0, IRQ_EL0_32

fiq_el0_32:
	handle_entry	0, FIQ_EL0_32

error_el0_32:
	handle_entry	0, ERROR_EL0_32

/* void eret_to_thread(u64 sp) */
BEGIN_FUNC(__eret_to_thread)
	mov	sp, x0
	bl finish_switch
	exception_exit
END_FUNC(__eret_to_thread)
