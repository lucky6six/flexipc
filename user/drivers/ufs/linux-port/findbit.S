/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  linux/arch/arm/lib/findbit.S
 *
 *  Copyright (C) 1995-2000 Russell King
 *
 * 16th March 2001 - John Ripley <jripley@sonicblue.com>
 *   Fixed so that "size" is an exclusive not an inclusive quantity.
 *   All users of these functions expect exclusive sizes, and may
 *   also call with zero size.
 * Reworked by rmk.
 */
// #include "assembler.h"

#define ASM_NL       ;
#define ARM(x...) x
#define THUMB(...)
#define ALIGN     .align 4,0x90


#define ENTRY(name) \
	.globl name ASM_NL \
		ALIGN ASM_NL \
		name:

#define END(name) \
	.size name, .-name

#define ENDPROC(name) \
	.type name, @function ASM_NL \
	END(name)


.text

/*
 * Purpose  : Find a 'zero' bit
 * Prototype: int find_first_zero_bit(void *addr, unsigned int maxbit);
 */
ENTRY(_find_first_zero_bit_le)
		teq	r1, #0
		beq	3f
		mov	r2, #0
1:
 ARM(		ldrb	r3, [r0, r2, lsr #3]	)
 THUMB(		lsr	r3, r2, #3		)
 THUMB(		ldrb	r3, [r0, r3]		)
		eors	r3, r3, #0xff		@ invert bits
		bne	.L_found		@ any now set - found zero bit
		add	r2, r2, #8		@ next bit pointer
2:		cmp	r2, r1			@ any more?
		blo	1b
3:		mov	r0, r1			@ no free bits
		ret	lr
ENDPROC(_find_first_zero_bit_le)

/*
 * Purpose  : Find next 'zero' bit
 * Prototype: int find_next_zero_bit(void *addr, unsigned int maxbit, int offset)
 */
ENTRY(_find_next_zero_bit_le)
		teq	r1, #0
		beq	3b
		ands	ip, r2, #7
		beq	1b			@ If new byte, goto old routine
 ARM(		ldrb	r3, [r0, r2, lsr #3]	)
 THUMB(		lsr	r3, r2, #3		)
 THUMB(		ldrb	r3, [r0, r3]		)
		eor	r3, r3, #0xff		@ now looking for a 1 bit
		movs	r3, r3, lsr ip		@ shift off unused bits
		bne	.L_found
		orr	r2, r2, #7		@ if zero, then no bits here
		add	r2, r2, #1		@ align bit pointer
		b	2b			@ loop for next bit
ENDPROC(_find_next_zero_bit_le)

/*
 * Purpose  : Find a 'one' bit
 * Prototype: int find_first_bit(const unsigned long *addr, unsigned int maxbit);
 */
ENTRY(_find_first_bit_le)
		teq	r1, #0	
		beq	3f
		mov	r2, #0
1:
 ARM(		ldrb	r3, [r0, r2, lsr #3]	)
 THUMB(		lsr	r3, r2, #3		)
 THUMB(		ldrb	r3, [r0, r3]		)
		movs	r3, r3
		bne	.L_found		@ any now set - found zero bit
		add	r2, r2, #8		@ next bit pointer
2:		cmp	r2, r1			@ any more?
		blo	1b
3:		mov	r0, r1			@ no free bits
		ret	lr
ENDPROC(_find_first_bit_le)

/*
 * Purpose  : Find next 'one' bit
 * Prototype: int find_next_zero_bit(void *addr, unsigned int maxbit, int offset)
 */
ENTRY(_find_next_bit_le)
		teq	r1, #0
		beq	3b
		ands	ip, r2, #7
		beq	1b			@ If new byte, goto old routine
 ARM(		ldrb	r3, [r0, r2, lsr #3]	)
 THUMB(		lsr	r3, r2, #3		)
 THUMB(		ldrb	r3, [r0, r3]		)
		movs	r3, r3, lsr ip		@ shift off unused bits
		bne	.L_found
		orr	r2, r2, #7		@ if zero, then no bits here
		add	r2, r2, #1		@ align bit pointer
		b	2b			@ loop for next bit
ENDPROC(_find_next_bit_le)


/*
 * One or more bits in the LSB of r3 are assumed to be set.
 */
.L_found:
		rsb	r0, r3, #0
		and	r3, r3, r0
		clz	r3, r3
		rsb	r3, r3, #31
		add	r0, r2, r3
		cmp	r1, r0			@ Clamp to maxbit
		movlo	r0, r1
		ret	lr

