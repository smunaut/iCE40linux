/*
 * boot.S
 *
 * SPI boot code
 *
 * Copyright (C) 2021 Sylvain Munaut <tnt@246tNt.com>
 * SPDX-License-Identifier: MIT
 */

/* Build Options */
#define BOOT_DEBUG
//#define BOOT_SIM

/* Config */
	.equ	HRAM_BASE, 0x80000000
	.equ	UART_BASE, 0x81000000
	.equ    PLAT_BASE, 0x82000000


/* ------------------------------------------------------------------------ */
/* Main                                                                     */
/* ------------------------------------------------------------------------ */

	.section .text.start
	.global _start
_start:

	// Console Setup
#ifdef BOOT_DEBUG
		// Set UART divisor
	li		a0, UART_BASE
	li		a1, 13
	sw		a1, 4(a0)

		// Output banner
	la		a0, str_banner
	call	print_str
#endif

	// HyperRAM setup
	call	hram_init

	// SPI init
	call	spi_init

	// Load manifest @ offset 128k
#ifdef MANIFEST_FLASH_ADDR
	la		a0, manifest
	li		a1, 0x000000c0		// len : 192 bytes
	li		a2, MANIFEST_FLASH_ADDR
	call	spi_flash_read
#endif

	// Parse each manifest chunk
	la		s4, manifest

1:
		// Debug print
#ifdef BOOT_DEBUG
	la		a0, str_loading
	mv		a1, s4
	call	print_str
#endif

		// Get chunk data
	lw		a0, 0(s4)			// Load address
	lw		a1, 4(s4)			// Length
	lw		a2, 8(s4)			// Flash offset

		// Check if it's end
	beq		a1, zero, 2f

		// Load it
	call	spi_flash_read

		// Next
	addi	s4, s4, 12
	j		1b

	// Final jump
		// Debug print
#ifdef BOOT_DEBUG
	la		a0, str_booting
	mv		a1, s4
	call	print_str

	lw		a0, 0(s4)			// Got corrupted by 'print_str', reload it
#endif

2:
	jalr	ra, a0, 0
	j		2b

manifest:
	// Each entry in the manifest is 3 words:
	//  - Destination address in RAM
	//  - Length
	//  - Offset in flash
    //
	// The entry with length=0 is considered the last one
	// and triggers the boot jump

#ifdef MANIFEST_FLASH_ADDR
	.space	16*3*4
#else
# if 0
	// Load Test app
	.word	0x40000000
	.word	 16*1024
	.word	640*1024

	// Boot Test app
	.word	0x40000000
	.word	0			// Len = 0
	.word	0
# else
	// Load BIOS
	.word	0x00000400
	.word	0x00000c00	// 3k
	.word	0x00020000	// ofs = 128k

	// Load DTB
	.word	0x41000000
	.word	0x00002000	// 8k
	.word	0x00030000	// ofs = 192k

	// Load Kernel
	.word	0x40000000
	.word	0x00480000	// 4.5M
	.word	0x00040000	// ofs = 256k

	// Boot BIOS
	.word	0x00000400
	.word	0x00000000
	.word	0x00000000
# endif
#endif


/* ------------------------------------------------------------------------ */
/* Hyper RAM                                                                */
/* ------------------------------------------------------------------------ */

/* Registers and bits definitions */

	.equ	HRAM_CSR,		0
	.equ	HRAM_CMD,		4
	.equ	HRAM_WQ_DATA,	8
	.equ	HRAM_WQ_ATTR,	12

#define HRAM_CSR_RUN			(1 << 0)
#define HRAM_CSR_RESET			(1 << 1)
#define HRAM_CSR_IDLE_CFG		(1 << 2)
#define HRAM_CSR_IDLE_RUN		(1 << 3)
#define HRAM_CSR_CMD_LAT(x)		((((x)-1) & 15) <<  8)
#define HRAM_CSR_CAP_LAT(x)		((((x)-1) & 15) << 12)
#define HRAM_CSR_PHY_DELAY(x)	(((x) & 15) <<  16)
#define HRAM_CSR_PHY_PHASE(x)	(((x) &  3) <<  20)
#define HRAM_CSR_PHY_EDGE(x)	(((x) &  1) <<  22)

#define HRAM_CMD_LEN(x)			((((x)-1) & 15) << 8)
#define HRAM_CMD_LAT(x)			((((x)-1) & 15) << 4)
#define HRAM_CMD_CS(x)			(((x) &  3) << 2)
#define HRAM_CMD_REG			(1 << 1)
#define HRAM_CMD_MEM			(0 << 1)
#define HRAM_CMD_READ			(1 << 0)
#define HRAM_CMD_WRITE			(0 << 0)


/*
 * Configures CR0 in a HyperRAM chip
 *
 * Params   : a0 = CS (0...3)
 * Clobbers : t0
 * Expects  : tp = HRAM_BASE
 */
hram_set_cr0:

	li		t0, 0x30
	sw		t0, HRAM_WQ_ATTR(tp)

	li		t0, 0x60000100
	sw		t0, HRAM_WQ_DATA(tp)

	li		t0, 0x000080ec
	sw		t0, HRAM_WQ_DATA(tp)

	sw		zero, HRAM_WQ_DATA(tp)

	slli	t0, a0, 2
	ori		t0, t0, HRAM_CMD_REG | HRAM_CMD_WRITE
	sw		t0, HRAM_CMD(tp)

		/* Fall through to wait_idle */

/*
 * Wait for the HyperRAM controller to report IDLE state
 *
 * Clobbers : t0
 * Expects  : tp = HRAM_BASE
 */
hram_wait_idle:
#ifndef BOOT_SIM
	/* while (!(hram_regs->csr & HRAM_CSR_IDLE_CFG)); */
	lw		t0, HRAM_CSR(tp)
	and		t0, t0, HRAM_CSR_IDLE_CFG
	beq 	t0, zero, hram_wait_idle
#endif
	ret


/*
 * Main HyperRAM controller init (using fixed config / no training)
 *
 * Clobbers : a0, tp, t0, t1
 */
hram_init:

	// Save return
	mv		t1, ra

	// Setup all IO access
	li		tp, HRAM_BASE

	// Reset HyperRAM and controller
	li		t0, HRAM_CSR_RESET
	sw		t0, HRAM_CSR(tp)
	call	hram_wait_idle
	sw		zero, HRAM_CSR(tp)
	call	hram_wait_idle

	// Set chip config
	li		a0, 0
	call	hram_set_cr0
	addi	a0, a0, 1
	call	hram_set_cr0
	addi	a0, a0, 1
	call	hram_set_cr0
	addi	a0, a0, 1
	call	hram_set_cr0

	// Set controller config
	li		t0, (\
				HRAM_CSR_CMD_LAT(2) | \
				HRAM_CSR_CAP_LAT(4) | \
				HRAM_CSR_PHY_DELAY(7) | \
				HRAM_CSR_PHY_PHASE(1) | \
				HRAM_CSR_PHY_EDGE(1) \
			)
	sw		t0, HRAM_CSR(tp)

	ori		t0, t0, HRAM_CSR_RUN
	sw		t0, HRAM_CSR(tp)

	// Done
	mv		ra, t1
	ret


/* ------------------------------------------------------------------------ */
/* SPI                                                                      */
/* ------------------------------------------------------------------------ */

/* Registers definitions */

	.equ    SPI_CSR,  4 * 0x00
	.equ    SPI_DAT,  4 * 0x01

/*
 * SPI core init
 *
 * Clobbers : t0, tp
 */
spi_init:
	li		tp, PLAT_BASE

	li		t0, 0x80
	sw		t0, SPI_CSR(tp)

	ret

/*
 * SPI read data to RAM
 *
 * Params   : a0 = destination pointer
 *            a1 = length (bytes)
 *            a2 = flash offset
 * Clobbers : s0, s1, t0, tp
 */
spi_flash_read:
	// Skip for sim
//#ifdef BOOT_SIM
//	ret
//#endif

	// Save params
	mv		s0, a0
	mv		s1, ra

	// Setup all IO access
	li		tp, PLAT_BASE

	// Setup CS
	li		t0, 0x81
	sw		t0, SPI_CSR(tp)

	// Send command
		// READ
	li		a0, 0x03
	sw		a0, SPI_DAT(tp)
1:
	lw		a0, SPI_DAT(tp)
	blt		a0, zero, 1b

		// addr[23:16]
	srli	a0, a2, 16
	sw		a0, SPI_DAT(tp)
1:
	lw		a0, SPI_DAT(tp)
	blt		a0, zero, 1b

		// addr[15:8]
	srli	a0, a2, 8
	sw		a0, SPI_DAT(tp)
1:
	lw		a0, SPI_DAT(tp)
	blt		a0, zero, 1b

		// addr[7:0]
	mv		a0, a2
	sw		a0, SPI_DAT(tp)
1:
	lw		a0, SPI_DAT(tp)
	blt		a0, zero, 1b

	// Read loop
	sw		zero, SPI_DAT(tp)
	add		a1, a1, s0

_spi_loop:
1:
	lw		a0, SPI_DAT(tp)
	blt		a0, zero, 1b
	sw		zero, SPI_DAT(tp)	// Prepare next xfer

	sb		a0, 0(s0)
	addi	s0, s0, 1
	bne		s0, a1, _spi_loop

	// Release CS
	li		t0, 0x80
	sw		t0, SPI_CSR(tp)

	// Done
	jr		s1


/* ------------------------------------------------------------------------ */
/* Debug / Console                                                          */
/* ------------------------------------------------------------------------ */

#ifdef BOOT_DEBUG

/*
 * Outputs NUL-terminated string, replacing % with hex prints
 *
 * Params   : a0 - String ptr
 *            a1 - Pointer to numbers
 * Clobbers : a0, t0-t2, s0-s1, tp
 */
print_str:
	// Save some values
	mv		s0, a0
	mv		s1, ra

	// Setup IO
	li		tp, UART_BASE

	// Main loop
1:
		// Load char
	lb		t0, 0(s0)
	addi	s0, s0, 1

		// Test for end
	beq		t0, zero, 3f

		// Test for hex
	xori	t1, t0, '%'
	beq		t1, zero, 2f

		// Normal char
	sw		t0, 0(tp)
	j		1b

		// Print hex
2:
	lw		a0, 0(a1)
	addi	a1, a1, 4
	call	print_hex
	j		1b

	// Done
3:
	jr		s1


/*
 * Outputs 32 bits hex
 *
 * Params   : a0 - Number to print
 * Clobbers : a0, t0-t2, tp
 */
print_hex:
	li		tp, UART_BASE
	li		t0, 8
	la		t1, str_hexchar

1:
	srli	t2, a0, 28
	add		t2, t2, t1
	lb		t2, 0(t2)
	sw		t2, 0(tp)

	slli    a0, a0, 4

	addi    t0, t0, -1
	bne		zero, t0, 1b

	ret

/*
 * Outputs '\r\n'
 *
 * Clobbers : t0, tp
 */
print_nl:
	li		tp, UART_BASE

	li		t0, '\r'
	sw		t0, 0(tp)
	li		t0, '\n'
	sw		t0, 0(tp)

	ret


str_hexchar:
	.ascii  "0123456789abcdef"

str_banner:
	.asciz	"Starting Loader\r\n"

str_loading:
	.asciz	"Chunk: Addr 0x%, Len 0x%, Offset 0x%\r\n"

str_booting:
	.asciz	"Jump to 0x%\r\n"

#endif

