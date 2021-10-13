#include <common/asm.h>

#define __ASM__
#include <arch/machine/registers.h>
#undef __ASM__

entry:
BEGIN_FUNC(efi_stub_entry)
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    adrp    x8, img_start
    add     x8, x8, #:lo12:img_start
    add     x2, sp, 16
    str     x8, [x2]
    bl	    efi_entry_c

    mov     x21, x0

    /* Disable MMU in el2 (current el) */
    mrs     x0, sctlr_el2
    /* Disable MMU */
	bic     x0, x0, #SCTLR_EL1_M
    /* Disable alignment checking */
	bic     x0, x0, #SCTLR_EL1_A
	bic     x0, x0, #SCTLR_EL1_SA0
	bic     x0, x0, #SCTLR_EL1_SA
	orr     x0, x0, #SCTLR_EL1_nAA
    /* Disable Data Cache */
	bic     x0, x0, #SCTLR_EL1_C
    /* Disable Instruction Cache */
	bic     x0, x0, #SCTLR_EL1_I
	msr     sctlr_el2, x0
	isb

    mov     x0, x21
    /* Move the kernel to the address specified in linker */
    /* TODO BUG Remain: If the space is reserved by the ARM? */
    bl      move_kernel_image

    /* Wake up the cores and ask them to execute from the kernel start */
    bl      plat_wake_up_cores

    /* Jump to the header of the kernel: _head */
    br      x0
END_FUNC(efi_stub_entry)

