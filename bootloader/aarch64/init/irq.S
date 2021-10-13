#include <common/asm.h>
#include "irq.h"

.macro	exception_entry	label
	.align	7
	b	\label
.endm

.macro handle_entry type
	exception_enter
	mov	x0, #\type
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	show_entry_message
	exception_exit
.endm

.macro  exception_enter
	sub	sp, sp, #IRQ_FRAME_SIZE
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
	str	x30, [sp, #16 * 15]
.endm

.macro	exception_exit
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
	ldr	x30, [sp, #16 * 15]
	add	sp, sp, #IRQ_FRAME_SIZE
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
EXPORT(boot_vector)
	exception_entry	sync_el1t			// Synchronous EL1t
	exception_entry	irq_el1t			// IRQ EL1t
	exception_entry	fiq_el1t			// FIQ EL1t
	exception_entry	error_el1t			// Error EL1t

	exception_entry	sync_el1h			// Synchronous EL1h
	exception_entry	irq_el1h			// IRQ EL1h
	exception_entry	fiq_el1h			// FIQ EL1h
	exception_entry	error_el1h			// Error EL1h

sync_el1t:
	handle_entry  SYNC_EL1t

irq_el1t:
	handle_entry  IRQ_EL1t

fiq_el1t:
	handle_entry  FIQ_EL1t

error_el1t:
	handle_entry  ERROR_EL1t

sync_el1h:
	handle_entry  SYNC_EL1h

irq_el1h:
	handle_entry  IRQ_EL1h

fiq_el1h:
	handle_entry  FIQ_EL1h

error_el1h:
	handle_entry  ERROR_EL1h
