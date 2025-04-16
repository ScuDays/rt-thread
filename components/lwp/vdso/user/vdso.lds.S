/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2024-07-04     rcitach        init ver.
 */
 
#include <vdso_config.h>

/* 修改: 原aarch64格式 */
/* OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64") */
OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)

SECTIONS
{
    PROVIDE(_vdso_data = . - __VVAR_PAGES * VDSO_PAGE_SIZE);
    . = SIZEOF_HEADERS;

    .hash        : { *(.hash) }     :text
    .gnu.hash    : { *(.gnu.hash) }
    .dynsym      : { *(.dynsym) }
    .dynstr      : { *(.dynstr) }
    .gnu.version    : { *(.gnu.version) }
    .gnu.version_d  : { *(.gnu.version_d) }
    .gnu.version_r  : { *(.gnu.version_r) }

    .dynamic     : { *(.dynamic) }    :text    :dynamic
    .rela.dyn    : ALIGN(8) { *(.rela .rela*) }

    .rodata      : {
        *(.rodata*) 
        *(.got)
        *(.got.plt)
        *(.plt)
        *(.plt.*)
        *(.iplt)
        *(.igot .igot.plt)
    }    :text
    /DISCARD/    : {
        *(.data .data.* .sdata*)
        *(.bss .sbss .dynbss .dynsbss)
    }
}

PHDRS
{
    text        PT_LOAD       FLAGS(5) FILEHDR PHDRS;
    dynamic     PT_DYNAMIC    FLAGS(4);
}

VERSION
{
    LINUX_2.6.39 {
    global:
        __kernel_clock_gettime;
    local: *;
    };
}
