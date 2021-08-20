/*
 * start.S
 *
 * Startup code
 *
 * Copyright (C) 2021 Sylvain Munaut
 * SPDX-License-Identifier: MIT
 */

#define BOOT_DEBUG

	.section .text.start
	.global _start
_start:

#ifdef BOOT_DEBUG
	// Set UART divisor
	li a0, 0x81000000
	li a1, 13
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


	.global  _trap_entry
	.align  4
_trap_entry:
	addi	sp,sp, -16*4
	sw		x1,   0*4(sp)
	sw		x5,   1*4(sp)
	sw		x6,   2*4(sp)
	sw		x7,   3*4(sp)
	sw		x10,  4*4(sp)
	sw		x11,  5*4(sp)
	sw		x12,  6*4(sp)
	sw		x13,  7*4(sp)
	sw		x14,  8*4(sp)
	sw		x15,  9*4(sp)
	sw		x16, 10*4(sp)
	sw		x17, 11*4(sp)
	sw		x28, 12*4(sp)
	sw		x29, 13*4(sp)
	sw		x30, 14*4(sp)
	sw		x31, 15*4(sp)

	call	trap

	lw		x1,   0*4(sp)
	lw		x5,   1*4(sp)
	lw		x6,   2*4(sp)
	lw		x7,   3*4(sp)
	lw		x10,  4*4(sp)
	lw		x11,  5*4(sp)
	lw		x12,  6*4(sp)
	lw		x13,  7*4(sp)
	lw		x14,  8*4(sp)
	lw		x15,  9*4(sp)
	lw		x16, 10*4(sp)
	lw		x17, 11*4(sp)
	lw		x28, 12*4(sp)
	lw		x29, 13*4(sp)
	lw		x30, 14*4(sp)
	lw		x31, 15*4(sp)
	addi	sp,sp, 16*4

	mret

