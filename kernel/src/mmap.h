#ifndef __MMAP_H__
#define __MMAP_H__

/* This header describes PacketNgin memory map. (docs/mmap.md) */

#include <stdint.h>

/*
 * Phyiscal memory map
 **/

/* BIOS area (0M ~ 1M) */
#define IVT_AREA_START              0x0
#define IVT_AREA_END                0x03FF
#define BDA_AREA_START              0x0400
#define BDA_AREA_END                0x04FF

/* Description table area (1M ~ 2M) */
#define DESC_TABLE_AREA_START       0x100000    /* 1 MB */
#define DESC_TABLE_AREA_END         0x200000    /* 2 MB */

/* Kernel text area (2M ~ 4M) */
#define KERNEL_TEXT_AREA_START      0x200000    /* 2 MB */
#define KERNEL_TEXT_AREA_SIZE       0x200000    /* 2 MB */
#define KERNEL_TEXT_AREA_END        0x400000    /* 4 MB */

/* Kernel data area (4M ~ 6M) */
#define KERNEL_DATA_AREA_START      0x400000    /* 4 MB */
#define KERNEL_DATA_AREA_END        0x600000    /* 6 MB */

// End of kernel - defined in linker script
#define VIRTUAL_TO_PHYSICAL(addr)	(~0xffffffff80000000L & (addr))
extern char __bss_end[];

#define KERNEL_DATA_START           0x400000    /* 4 MB */
#define KERNEL_DATA_END             VIRTUAL_TO_PHYSICAL((uint64_t)__bss_end)

#define LOCAL_MALLOC_START          KERNEL_DATA_END
#define LOCAL_MALLOC_END            VGA_BUFFER_START

#define VGA_BUFFER_START            (USER_INTR_STACK_START - VGA_BUFFER_SIZE)
#define VGA_BUFFER_SIZE             0x10000     /* 64 KB */
#define VGA_BUFFER_END              USER_INTR_STACK_START

#define USER_INTR_STACK_START       (KERNEL_INTR_STACK_START - USER_INTR_STACK_SIZE)
#define USER_INTR_STACK_SIZE        0x8000      /* 32 KB */
#define USER_INTR_STACK_END         KERNEL_INTR_STACK_START

#define KERNEL_INTR_STACK_START     (KERNEL_STACK_START - KERNEL_INTR_STACK_SIZE)
#define KERNEL_INTR_STACK_SIZE      0x8000      /* 32 KB */
#define KERNEL_INTR_STACK_END       KERNEL_STACK_START

#define KERNEL_STACK_START          (PAGE_TABLE_START - KERNEL_STACK_SIZE)
#define KERNEL_STACK_SIZE           0x10000     /* 64 KB */
#define KERNEL_STACK_END            PAGE_TABLE_START

#define PAGE_TABLE_START            (KERNEL_DATA_AREA_END - (4096 * 64))
#define PAGE_TABLE_END              KERNEL_DATA_AREA_END

/* Kernel data area (6M ~ 36M) */
// Kernel data area above (4M ~ 6M) is duplicated here for 15 cores.
// PacketNgin support maximum 16 cores.

/* Ramdisk area (36M ~ sizeof(initrd.img)) */
#define RAMDISK_START               (KERNEL_TEXT_AREA_START + KERNEL_TEXT_AREA_SIZE * 16)

#endif /* __MMAP_H__ */
