/* TODO: borrowed from Linux */

/*
 * _head is the beginning (first instruction) of the bootloader.
 * Without EFI, _start function will be directly invoked.
 * With EFI, the EFI-related function will be invoked in advance by the EFI firmware.
 */

#include "image.h"

_head:
	add	x13, x18, #0x16 			/* Magic number (check by grub) */
	b	_start
	.quad   TEXT_OFFSET				/* Image load offset little-endian */
	.quad	(img_end - _head)		/* Effective size of kernel image little-endian */
	.quad	0						/* reserved */
	.quad   0			 			/* reserved */
	.quad	0						/* reserved */
	.quad	0						/* reserved */
	.byte	0x41					/* Magic number, "ARM\x64" */
	.byte	0x52
	.byte	0x4d
	.byte	0x64
	.long   pe_header - _head       /* Offset to the PE header. */

	/*
	 * Add the PE/COFF header to the file.  The address of this header
	 * is at offset 0x3c in the file, and is part of Linux "Image"
	 * header.  The arm64 Linux Image format is designed to support
	 * being both an 'Image' format binary and a PE/COFF binary.
	 * The PE/COFF format is defined by Microsoft, and is available
	 * from: http://msdn.microsoft.com/en-us/gg463119.aspx
	 * Version 8.3 adds support for arm64 and UEFI usage.
	 */
	.align  3

pe_header:
	.ascii  "PE"
	.short  0

coff_header:
	.short  0xaa64                          /* AArch64 */
	.short  2                               /* nr_sections */
	.long   0                               /* TimeDateStamp */
	.long   0                               /* PointerToSymbolTable */
	.long   1                               /* NumberOfSymbols */
	.short  section_table - optional_header /* SizeOfOptionalHeader */
	.short  0x206                           /* Characteristics. */

optional_header:
	.short  0x20b                           /* PE32+ format */
	.byte   0x02                            /* MajorLinkerVersion */
	.byte   0x14                            /* MinorLinkerVersion */
	.long   img_end - efi_header_end        /* SizeOfCode */
	.long   0                               /* SizeOfInitializedData */
	.long   0                               /* SizeOfUninitializedData */
	.long   efi_stub_entry - _head          /* AddressOfEntryPoint */
	.long   efi_header_end - _head          /* BaseOfCode */

extra_header_fields:
	.quad   0                               /* ImageBase */
	.long   0x1000                          /* SectionAlignment (4 KByte) */
	.long   0x200                           /* FileAlignment */
	.short  0                               /* MajorOperatingSystemVersion */
	.short  0                               /* MinorOperatingSystemVersion */
	.short  0                               /* MajorImageVersion */
	.short  0                               /* MinorImageVersion */
	.short  0                               /* MajorSubsystemVersion */
	.short  0                               /* MinorSubsystemVersion */
	.long   0                               /* Win32VersionValue */
	.long   img_end - _head                 /* SizeOfImage */

	/* Everything before the kernel image is considered part of the header */
	.long   efi_header_end - _head          /* SizeOfHeaders */
	.long   0                               /* CheckSum */
	.short  0xa                             /* Subsystem (EFI application) */
	.short  0                               /* DllCharacteristics */
	.quad   0                               /* SizeOfStackReserve */
	.quad   0                               /* SizeOfStackCommit */
	.quad   0                               /* SizeOfHeapReserve */
	.quad   0                               /* SizeOfHeapCommit */
	.long   0                               /* LoaderFlags */
	.long   0x6                             /* NumberOfRvaAndSizes */
	.quad   0                               /* ExportTable */
	.quad   0                               /* ImportTable */
	.quad   0                               /* ResourceTable */
	.quad   0                               /* ExceptionTable */
	.quad   0                               /* CertificationTable */
	.quad   0                               /* BaseRelocationTable */

/* Section table */
section_table:
	/*
	 * The EFI application loader requires a relocation section
	 * because EFI applications must be relocatable.  This is a
	 * dummy section as far as we are concerned.
	 */
	.ascii  ".reloc"
	.byte   0
	.byte   0                               /* end of 0 padding of section name */
	.long   0
	.long   0
	.long   0                               /* SizeOfRawData */
	.long   0                               /* PointerToRawData */
	.long   0                               /* PointerToRelocations */
	.long   0                               /* PointerToLineNumbers */
	.short  0                               /* NumberOfRelocations */
	.short  0                               /* NumberOfLineNumbers */
	.long   0x42100040                      /* Characteristics (section flags) */


	.ascii  ".text"
	.byte   0
	.byte   0
	.byte   0                               /* end of 0 padding of section name */
	.long   img_end - efi_header_end        /* VirtualSize */
	.long   efi_header_end - _head          /* VirtualAddress */
	.long   _edata - efi_header_end     	/* SizeOfRawData */
	.long   efi_header_end - _head          /* PointerToRawData */

	.long   0                /* PointerToRelocations (0 for executables) */
	.long   0                /* PointerToLineNumbers (0 for executables) */
	.short  0                /* NumberOfRelocations  (0 for executables) */
	.short  0                /* NumberOfLineNumbers  (0 for executables) */
	.long   0xe0500020       /* Characteristics (section flags) */
	.align  12
efi_header_end:
