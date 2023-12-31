#include <common/asm.h>
#define __ASM__
#include <seg.h>
#undef __ASM__
#include <memlayout.h>

/* Linux 0xffffffff80000000 */
#define KCODE      0xFFFFFFFFC0000000
#define PADDR(x)   ((x) - KCODE)
#define KSTACKSIZE (4*0x1000)

/* linkder.lds provides: edata and end */

/* MultiBoot header */
.align 4
.text
EXPORT(multiboot_header)
		#define magic 0x1badb002
		#define flags (1 << 16 | 1 << 0)
		.long magic
		.long flags
		.long (- magic - flags)        # checksum
		.long PADDR(multiboot_header)  # head address
		.long PADDR(multiboot_header)  # load address
		.long PADDR(edata)             # load end address
		.long PADDR(end)               # bss end address
		.long PADDR(start)             # entry address


/* Entry point: running in 32-bit mode */
.code32
EXPORT(start)
	movl %eax, %edi /* multiboot_magic */
	movl %ebx, %esi /* multiboot_info_ptr */

	/* set init stack */
	movl $PADDR(stack+KSTACKSIZE), %esp

	/* zero bss */
	movl %edi, %edx
	/*
	 * rep stosb
	 * For ecx repetitions, stores the contents of eax into where edi points to,
	 * incrementing or decrementing edi (depending on the direction flag) by 4 bytes each time.
	 * Normally, this is used for a memset-type operation.  */
	movl $PADDR(edata), %edi
	movl $PADDR(end), %ecx
	subl $PADDR(edata), %ecx
	movl $0, %eax
	cld
	rep stosb
	movl %edx, %edi

	call loadgdt

	/* Enter new 32-bit code segment (already in 32-bit mode) */
	ljmp $KCSEG32, $PADDR(_start32)

.code32
_start32:
	/* enabling 64-bit and paging */
	call init32e

	movl $PADDR(_start64), %eax
	/* Reload CS with long bit to enable long mode */
	ljmp $KCSEG64, $PADDR(_tramp64)

.code64
_start64:
	/* load VA stack */
	movabsq $(stack+KSTACKSIZE), %rsp
	movq $0, %rbp
	call main
	/* Un-reachable */
	jmp .


#define PROTECTED_CSEG 	(1 << 3)
#define PROTECTED_DSEG 	(2 << 3)
#define CR0_PE      	0x1
#define MP_PADDR(x)		(MP_BOOT_ADDR + x - _mp_start)

/* SMP boot entry */
.code16
EXPORT(_mp_start)
	cli
	wbinvd
	mov $0, %ax
	mov %ax, %ds
	cld

	lgdt MP_PADDR(mp_boot_gdtdesc)
	movl %cr0, %eax
	orl $CR0_PE, %eax
	movl %eax, %cr0
	ljmp $PROTECTED_CSEG, $MP_PADDR(_mp_protected)

.align 4
mp_boot_gdt:
	.quad 0 # null seg
	SEGDESC(0, 0xfffff,
			SEG_R | SEG_CODE | SEG_S | SEG_DPL(0) | SEG_P | SEG_D | SEG_G)
	SEGDESC(0, 0xfffff,
			SEG_W | SEG_S | SEG_DPL(0) | SEG_P | SEG_D | SEG_G)

.align 16
mp_boot_gdtdesc:
	.word	0x17 # sizeof(mp_boot_gdt) - 1
	.long	MP_PADDR(mp_boot_gdt)  # address gdt


.code32
_mp_protected:
	movw $PROTECTED_DSEG, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	movl $PADDR(cur_cpu_stack), %esp

	call loadgdt
	ljmp $KCSEG32, $PADDR(_mp_star32)

.code32
_mp_star32:
	call init32e
	movl $PADDR(_mp_star64), %eax
	ljmp $KCSEG64, $PADDR(_tramp64)

.code64
_mp_star64:
	movq cur_cpu_stack, %rsp
	movq $0, %rbp
	movl cur_cpu_id, %edi
	call secondary_start
	/* Un-reachable */
	jmp .

.code64
_tramp64:
	movq $KCODE, %r11
	/* rax stores the paddr of _start64 */
	addq %r11, %rax
	jmp *%rax

/* initial stack */
.comm stack, KSTACKSIZE

/* Page table attribute bits */
#define PRESENT  (1 << 0)
#define WRITABLE (1 << 1)
#define HUGE_1G  (1 << 7)
#define GLOBAL   (1 << 8)
#define NX	 (1ULL << 63)

/* CHCORE page table: one PGD page */
.align 4096
.global CHCORE_PGD
CHCORE_PGD:
	.quad PADDR(PUD_0)    + PRESENT + WRITABLE
	.space 4096 - 3*8
	.quad PADDR(PUD_DM)   + PRESENT + WRITABLE
	.quad PADDR(PUD_CODE) + PRESENT + WRITABLE

/* CHCORE page table: three PUD pages */
.align 4096
PUD_0:
	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
	.space 4096 - 8

.align 4096
/* PUD_Direct_Mapping */
.global CHCORE_PUD_Direct_Mapping
CHCORE_PUD_Direct_Mapping:
PUD_DM:
	/* Mapping 0~1G */
	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL + NX
	.space 4096 - 8

.align 4096
PUD_CODE:
	.space 4096 - 8
	/* Simply mapping 0~1G with rwx */
	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL

.align 4096
.code32
loadgdt:
	subl $8, %esp
	// TODO: bootgdt
	movl $PADDR(bootgdt), 4(%esp)
	movw $(8*GDT_ENTRIES-1), 2(%esp)
	lgdt 2(%esp)
	addl $8, %esp

	/* data segment selector */
	movl $KDSEG, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	/* null segment selector */
	movl $0, %eax
	movw %ax, %fs
	movw %ax, %gs

	ret


.code32
/* initialize IA-32e mode */
init32e:
	/* set cr4.PAE = cr4.PSE = cr4.OSFXSR = cr4.OSXMMEXCPT = 1 */
	movl %cr4, %eax
	// TODO: 0x630
	orl $0x630, %eax
	movl %eax, %cr4

	/* load cr3 with physical base address of level 4 page table */
	movl $PADDR(CHCORE_PGD), %eax
	movl %eax, %cr3

	/*
	 * enable IA-32e mode by setting IA32_EFER.LME = 1
	 * turn on IA32_EFER.SCE (syscall enable) and IA32_EFER.NXE (no-execute enable)
	 */
	movl $0xc0000080, %ecx
	rdmsr
	orl $((1 << 8) | (1 << 0) | (1 << 11)), %eax
	wrmsr

	/* enable paging by setting cr0.PG = 1 */
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	ret

EXPORT(_mp_start_end)
