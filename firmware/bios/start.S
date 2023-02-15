/*
 * start.S
 *
 * Startup code
 *
 * Copyright (C) 2021 Sylvain Munaut
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define BOOT_DEBUG

#ifndef UART_DIV
# define UART_DIV 13
#endif

	.section .text.start
	.global _start
_start:

#ifdef BOOT_DEBUG
	// Set UART divisor
	li a0, 0x81000000
	li a1, UART_DIV
	sw a1, 4(a0)

	// Output '1'
	li a1, 49
	sw a1, 0(a0)
#endif

	// Init .data section from flash
	la a0, _sidata
	la a1, _sdata
	la a2, _edata
	bge a1, a2, end_init_data
loop_init_data:
	lw a3, 0(a0)
	sw a3, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	blt a1, a2, loop_init_data
end_init_data:

	// Clear .bss section
	la a0, _sbss
	la a1, _ebss
	bge a0, a1, end_init_bss
loop_init_bss:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, loop_init_bss
end_init_bss:

	// Set stack pointer
	la sp, __stacktop

#ifdef BOOT_DEBUG
	// Output '3'
	li a0, 0x81000000
	li a1, 51
	sw a1, 0(a0)
#endif

	// call main
	call main

.global	_exit
_exit:
	j _exit
