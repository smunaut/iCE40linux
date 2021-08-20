/*
 * main.c
 *
 * Main Machine Mode bios, extracted from Linux-on-LiteX-VexRiscv
 * project and adapted for use here.
 *
 * Copyright (c) 2019-2021, Linux-on-LiteX-VexRiscv Developers
 * Copyright (C) 2021 Sylvain Munaut
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "irq.h"
#include "riscv.h"

#include "config.h"


#define MAIN_RAM_BASE		0x40000000
#define LINUX_IMAGE_BASE	(MAIN_RAM_BASE + 0x00000000)
#define LINUX_DTB_BASE		(MAIN_RAM_BASE + 0x01000000)

#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })


#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })


extern const uint32_t __stacktop;

void vexriscv_machine_mode_trap(void);


/* ------------------------------------------------------------------------ */
/* Peripheral access functions                                              */
/* ------------------------------------------------------------------------ */

/* UART */

struct wb_uart {
	uint32_t data;
	uint32_t clkdiv;
} __attribute__((packed,aligned(4)));

static volatile struct wb_uart * const uart_regs = (void*)(UART_BASE);


static void
hw_putchar(char c)
{
	uart_regs->data = c;
}

static int32_t
hw_getchar(void)
{
	int32_t c;
	c = uart_regs->data;
	return c & 0x80000000 ? -1 : (c & 0xff);
}


/* Platform / Timer */

struct wb_platform {
	uint32_t spi_csr;
	uint32_t spi_dat;
	uint32_t timer;
} __attribute__((packed,aligned(4)));

static volatile struct wb_platform * const plat_regs = (void*)(PLAT_BASE);


static uint32_t timer_lsb = 0;
static uint32_t timer_msb = 0;

static uint32_t
hw_read_cpu_timer(bool msb)
{
	uint32_t timer_lsb_new = plat_regs->timer;

	if (timer_lsb_new < timer_lsb)
		timer_msb++;

	timer_lsb = timer_lsb_new;

	return msb ? timer_msb : timer_lsb;
}

static void
hw_write_cpu_timer_cmp(uint32_t low, uint32_t high)
{
	plat_regs->timer = low & 0xfffffffe;
}

static void
hw_stop(void)
{
	while(1);
}


/* ------------------------------------------------------------------------ */
/* VexRiscv Registers / Words access functions                              */
/* ------------------------------------------------------------------------ */

static int vexriscv_read_register(uint32_t id){
	unsigned int sp = (unsigned int) (&__stacktop);
	return ((int*) sp)[id-32];
}
static void vexriscv_write_register(uint32_t id, int value){
	uint32_t sp = (uint32_t) (&__stacktop);
	((uint32_t*) sp)[id-32] = value;
}



#define vexriscv_trap_barrier_start \
		"  	li       %[tmp],  0x00020000\n" \
		"	csrs     mstatus,  %[tmp]\n" \
		"  	la       %[tmp],  1f\n" \
		"	csrw     mtvec,  %[tmp]\n" \
		"	li       %[fail], 1\n" \

#define vexriscv_trap_barrier_end \
		"	li       %[fail], 0\n" \
		"1:\n" \
		"  	li       %[tmp],  0x00020000\n" \
		"	csrc     mstatus,  %[tmp]\n" \

static int32_t vexriscv_read_word(uint32_t address, int32_t *data){
	int32_t result, tmp;
	int32_t fail;
	__asm__ __volatile__ (
		vexriscv_trap_barrier_start
		"	lw       %[result], 0(%[address])\n"
		vexriscv_trap_barrier_end
		: [result]"=&r" (result), [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address)
		: "memory"
	);

	*data = result;
	return fail;
}

static int32_t vexriscv_read_word_unaligned(uint32_t address, int32_t *data){
	int32_t result, tmp;
	int32_t fail;
	__asm__ __volatile__ (
			vexriscv_trap_barrier_start
		"	lbu      %[result], 0(%[address])\n"
		"	lbu      %[tmp],    1(%[address])\n"
		"	slli     %[tmp],  %[tmp], 8\n"
		"	or       %[result], %[result], %[tmp]\n"
		"	lbu      %[tmp],    2(%[address])\n"
		"	slli     %[tmp],  %[tmp], 16\n"
		"	or       %[result], %[result], %[tmp]\n"
		"	lbu      %[tmp],    3(%[address])\n"
		"	slli     %[tmp],  %[tmp], 24\n"
		"	or       %[result], %[result], %[tmp]\n"
		vexriscv_trap_barrier_end
		: [result]"=&r" (result), [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address)
		: "memory"
	);

	*data = result;
	return fail;
}

static int32_t vexriscv_read_half_unaligned(uint32_t address, int32_t *data){
	int32_t result, tmp;
	int32_t fail;
	__asm__ __volatile__ (
		vexriscv_trap_barrier_start
		"	lb       %[result], 1(%[address])\n"
		"	slli     %[result],  %[result], 8\n"
		"	lbu      %[tmp],    0(%[address])\n"
		"	or       %[result], %[result], %[tmp]\n"
		vexriscv_trap_barrier_end
		: [result]"=&r" (result), [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address)
		: "memory"
	);

	*data = result;
	return fail;
}

static int32_t vexriscv_write_word(uint32_t address, int32_t data){
	int32_t tmp;
	int32_t fail;
	__asm__ __volatile__ (
		vexriscv_trap_barrier_start
		"	sw       %[data], 0(%[address])\n"
		vexriscv_trap_barrier_end
		: [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address), [data]"r" (data)
		: "memory"
	);

	return fail;
}


static int32_t vexriscv_write_word_unaligned(uint32_t address, int32_t data){
	int32_t tmp;
	int32_t fail;
	__asm__ __volatile__ (
		vexriscv_trap_barrier_start
		"	sb       %[data], 0(%[address])\n"
		"	srl      %[data], %[data], 8\n"
		"	sb       %[data], 1(%[address])\n"
		"	srl      %[data], %[data], 8\n"
		"	sb       %[data], 2(%[address])\n"
		"	srl      %[data], %[data], 8\n"
		"	sb       %[data], 3(%[address])\n"
		vexriscv_trap_barrier_end
		: [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address), [data]"r" (data)
		: "memory"
	);

	return fail;
}



static int32_t vexriscv_write_short_unaligned(uint32_t address, int32_t data){
	int32_t tmp;
	int32_t fail;
	__asm__ __volatile__ (
		vexriscv_trap_barrier_start
		"	sb       %[data], 0(%[address])\n"
		"	srl      %[data], %[data], 8\n"
		"	sb       %[data], 1(%[address])\n"
		vexriscv_trap_barrier_end
		: [fail]"=&r" (fail), [tmp]"=&r" (tmp)
		: [address]"r" (address), [data]"r" (data)
		: "memory"
	);

	return fail;
}


/* ------------------------------------------------------------------------ */
/* VexRiscv Machine Mode Emulator                                           */
/* ------------------------------------------------------------------------ */

static void vexriscv_machine_mode_trap_entry(void) {
	__asm__ __volatile__ (
	"csrrw sp, mscratch, sp\n"
	"sw x1,   1*4(sp)\n"
	"sw x3,   3*4(sp)\n"
	"sw x4,   4*4(sp)\n"
	"sw x5,   5*4(sp)\n"
	"sw x6,   6*4(sp)\n"
	"sw x7,   7*4(sp)\n"
	"sw x8,   8*4(sp)\n"
	"sw x9,   9*4(sp)\n"
	"sw x10,   10*4(sp)\n"
	"sw x11,   11*4(sp)\n"
	"sw x12,   12*4(sp)\n"
	"sw x13,   13*4(sp)\n"
	"sw x14,   14*4(sp)\n"
	"sw x15,   15*4(sp)\n"
	"sw x16,   16*4(sp)\n"
	"sw x17,   17*4(sp)\n"
	"sw x18,   18*4(sp)\n"
	"sw x19,   19*4(sp)\n"
	"sw x20,   20*4(sp)\n"
	"sw x21,   21*4(sp)\n"
	"sw x22,   22*4(sp)\n"
	"sw x23,   23*4(sp)\n"
	"sw x24,   24*4(sp)\n"
	"sw x25,   25*4(sp)\n"
	"sw x26,   26*4(sp)\n"
	"sw x27,   27*4(sp)\n"
	"sw x28,   28*4(sp)\n"
	"sw x29,   29*4(sp)\n"
	"sw x30,   30*4(sp)\n"
	"sw x31,   31*4(sp)\n"
	"call vexriscv_machine_mode_trap\n"
	"lw x1,   1*4(sp)\n"
	"lw x3,   3*4(sp)\n"
	"lw x4,   4*4(sp)\n"
	"lw x5,   5*4(sp)\n"
	"lw x6,   6*4(sp)\n"
	"lw x7,   7*4(sp)\n"
	"lw x8,   8*4(sp)\n"
	"lw x9,   9*4(sp)\n"
	"lw x10,   10*4(sp)\n"
	"lw x11,   11*4(sp)\n"
	"lw x12,   12*4(sp)\n"
	"lw x13,   13*4(sp)\n"
	"lw x14,   14*4(sp)\n"
	"lw x15,   15*4(sp)\n"
	"lw x16,   16*4(sp)\n"
	"lw x17,   17*4(sp)\n"
	"lw x18,   18*4(sp)\n"
	"lw x19,   19*4(sp)\n"
	"lw x20,   20*4(sp)\n"
	"lw x21,   21*4(sp)\n"
	"lw x22,   22*4(sp)\n"
	"lw x23,   23*4(sp)\n"
	"lw x24,   24*4(sp)\n"
	"lw x25,   25*4(sp)\n"
	"lw x26,   26*4(sp)\n"
	"lw x27,   27*4(sp)\n"
	"lw x28,   28*4(sp)\n"
	"lw x29,   29*4(sp)\n"
	"lw x30,   30*4(sp)\n"
	"lw x31,   31*4(sp)\n"
	"csrrw sp, mscratch, sp\n"
	"mret\n"
	);
}

static void vexriscv_machine_mode_pmp_init(void)
{
	/* Allow access to all the memory, ignore the illegal
	   instruction trap if PMPs are not supported */
	uintptr_t pmpc = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
	asm volatile (
		"la t0, 1f\n\t"
		"csrw mtvec, t0\n\t"
		"csrw pmpaddr0, %1\n\t"
		"csrw pmpcfg0, %0\n\t"
		".align 2\n\t"
		"1:"
		: : "r" (pmpc), "r" (-1UL) : "t0"
	);
}

static void vexriscv_machine_mode_init(void) {
	vexriscv_machine_mode_pmp_init();
	uint32_t sp = (uint32_t) (&__stacktop);
	csr_write(mtvec,    vexriscv_machine_mode_trap_entry);
	csr_write(mscratch, sp -32*4);
	csr_write(mstatus,  0x0800 | MSTATUS_MPIE);
	csr_write(mie,      0);
	csr_write(mepc,     LINUX_IMAGE_BASE);
	/* Stop in case of miss-aligned accesses */
	csr_write(medeleg, MEDELEG_INSTRUCTION_PAGE_FAULT | MEDELEG_LOAD_PAGE_FAULT | MEDELEG_STORE_PAGE_FAULT | MEDELEG_USER_ENVIRONNEMENT_CALL);
	csr_write(mideleg, MIDELEG_SUPERVISOR_TIMER | MIDELEG_SUPERVISOR_EXTERNAL | MIDELEG_SUPERVISOR_SOFTWARE);
	/* Avoid simulation missmatch */
	csr_write(sbadaddr, 0);
}

static void vexriscv_machine_mode_trap_to_supervisor_trap(uint32_t sepc, uint32_t mstatus){
	csr_write(mtvec,    vexriscv_machine_mode_trap_entry);
	csr_write(sbadaddr, csr_read(mbadaddr));
	csr_write(scause,   csr_read(mcause));
	csr_write(sepc,     sepc);
	csr_write(mepc,	    csr_read(stvec));
	csr_write(mstatus,
			  (mstatus & ~(MSTATUS_SPP | MSTATUS_MPP | MSTATUS_SIE | MSTATUS_SPIE))
			| ((mstatus >> 3) & MSTATUS_SPP)
			| (0x0800 | MSTATUS_MPIE)
			| ((mstatus & MSTATUS_SIE) << 4)
	);
}


static uint32_t vexriscv_read_instruction(uint32_t pc){
	uint32_t i;
	if (pc & 2) {
		vexriscv_read_word(pc - 2, (int32_t*)&i);
		i >>= 16;
		if ((i & 3) == 3) {
			uint32_t u32Buf;
			vexriscv_read_word(pc+2, (int32_t*)&u32Buf);
			i |= u32Buf << 16;
		}
	} else {
		vexriscv_read_word(pc, (int32_t*)&i);
	}
	return i;
}

static uint32_t vexriscv_sbi_ext(uint32_t fid, uint32_t extid){
	switch (fid) {
		case SBI_EXT_SPEC_VERSION:
			return 0x00000001;
		case SBI_EXT_IMPL_ID:
			return 0;
		case SBI_EXT_IMPL_VERSION:
			return 0;
		case SBI_EXT_PROBE_EXTENSION:
			break;
		case SBI_EXT_GET_MVENDORID:
			return 0;
		case SBI_EXT_GET_MARCHID:
			return 0;
		case SBI_EXT_GET_MIMPID:
			return 0;
		default:
			return 0;
	}

	/* sbi_probe_extension() */
	switch (extid) {
		case SBI_CONSOLE_PUTCHAR:
		case SBI_CONSOLE_GETCHAR:
		case SBI_SET_TIMER:
		case SBI_EXT_BASE:
			return 1;
		default:
			return 0;
	}
}

__attribute__((used)) void vexriscv_machine_mode_trap(void) {
	int32_t cause = csr_read(mcause);

	/* Interrupt */
	if(cause < 0){
		switch(cause & 0xff){
			case CAUSE_MACHINE_TIMER: {
				csr_set(sip, MIP_STIP);
				csr_clear(mie, MIE_MTIE);
			} break;
			default: hw_stop(); break;
		}
	/* Exception */
	} else {
		switch(cause){
		    case CAUSE_UNALIGNED_LOAD:{
			    uint32_t mepc = csr_read(mepc);
			    uint32_t mstatus = csr_read(mstatus);
			    uint32_t instruction = vexriscv_read_instruction(mepc);
			    uint32_t address = csr_read(mbadaddr);
			    uint32_t func3 =(instruction >> 12) & 0x7;
			    uint32_t rd = (instruction >> 7) & 0x1F;
			    int32_t readValue;
			    int32_t fail = 1;

			    switch(func3){
			    case 1: fail = vexriscv_read_half_unaligned(address, &readValue); break;  //LH
			    case 2: fail = vexriscv_read_word_unaligned(address, &readValue); break; //LW
			    case 5: fail = vexriscv_read_half_unaligned(address, &readValue) & 0xFFFF; break; //LHU
			    }

			    if(fail){
				    vexriscv_machine_mode_trap_to_supervisor_trap(mepc, mstatus);
				    return;
			    }

			    vexriscv_write_register(rd, readValue);
			    csr_write(mepc, mepc + 4);
			    csr_write(mtvec, vexriscv_machine_mode_trap_entry); //Restore mtvec
		    }break;
		    case CAUSE_UNALIGNED_STORE:{
			    uint32_t mepc = csr_read(mepc);
			    uint32_t mstatus = csr_read(mstatus);
			    uint32_t instruction = vexriscv_read_instruction(mepc);
			    uint32_t address = csr_read(mbadaddr);
			    uint32_t func3 =(instruction >> 12) & 0x7;
			    int32_t writeValue = vexriscv_read_register((instruction >> 20) & 0x1F);
			    int32_t fail = 1;

			    switch(func3){
			    case 1: fail = vexriscv_write_short_unaligned(address, writeValue); break; //SH
			    case 2: fail = vexriscv_write_word_unaligned(address, writeValue); break; //SW
			    }

			    if(fail){
				    vexriscv_machine_mode_trap_to_supervisor_trap(mepc, mstatus);
				    return;
			    }

			    csr_write(mepc, mepc + 4);
			    csr_write(mtvec, vexriscv_machine_mode_trap_entry); //Restore mtvec
		    }break;
			/* Illegal instruction */
			case CAUSE_ILLEGAL_INSTRUCTION:{
				uint32_t mepc = csr_read(mepc);
				uint32_t mstatus = csr_read(mstatus);
				uint32_t instr = csr_read(mbadaddr);

				uint32_t opcode = instr & 0x7f;
				uint32_t funct3 = (instr >> 12) & 0x7;
				switch(opcode){
					/* Atomic */
					case 0x2f:
						switch(funct3){
							case 0x2:{
								uint32_t sel = instr >> 27;
								uint32_t addr = vexriscv_read_register((instr >> 15) & 0x1f);
								int32_t src = vexriscv_read_register((instr >> 20) & 0x1f);
								uint32_t rd = (instr >> 7) & 0x1f;
								int32_t read_value;
								int32_t write_value = 0
								;
								if(vexriscv_read_word(addr, &read_value)) {
									vexriscv_machine_mode_trap_to_supervisor_trap(mepc, mstatus);
									return;
								}

								switch(sel){
									case 0x0:  write_value = src + read_value; break;
									case 0x1:  write_value = src; break;
									case 0x2:  break; /*  LR, SC done in hardware (cheap) and require */
									case 0x3:  break; /*  to keep track of context switches */
									case 0x4:  write_value = src ^ read_value; break;
									case 0xC:  write_value = src & read_value; break;
									case 0x8:  write_value = src | read_value; break;
									case 0x10: write_value = min(src, read_value); break;
									case 0x14: write_value = max(src, read_value); break;
									case 0x18: write_value = min((unsigned int)src, (unsigned int)read_value); break;
									case 0x1C: write_value = max((unsigned int)src, (unsigned int)read_value); break;
									default: hw_stop(); return; break;
								}
								if(vexriscv_write_word(addr, write_value)){
									vexriscv_machine_mode_trap_to_supervisor_trap(mepc, mstatus);
									return;
								}
								vexriscv_write_register(rd, read_value);
								csr_write(mepc, mepc + 4);
								csr_write(mtvec, vexriscv_machine_mode_trap_entry); /* Restore MTVEC */
							} break;
							default: hw_stop(); break;
						} break;
					/* CSR */
					case 0x73:{
						uint32_t input = (instr & 0x4000) ? ((instr >> 15) & 0x1f) : vexriscv_read_register((instr >> 15) & 0x1f);
						__attribute__((unused)) uint32_t clear, set;
						uint32_t write;
						switch (funct3 & 0x3) {
							case 0: hw_stop(); break;
							case 1: clear = ~0; set = input; write = 1; break;
							case 2: clear = 0; set = input; write = ((instr >> 15) & 0x1f) != 0; break;
							case 3: clear = input; set = 0; write = ((instr >> 15) & 0x1f) != 0; break;
						}
						uint32_t csrAddress = instr >> 20;
						uint32_t old;
						switch(csrAddress){
							case RDCYCLE :
							case RDINSTRET:
							case RDTIME  : old = hw_read_cpu_timer(false); break;
							case RDCYCLEH :
							case RDINSTRETH:
							case RDTIMEH : old = hw_read_cpu_timer(true); break;
							default: hw_stop(); break;
						}
						if(write) {
							switch(csrAddress){
								default: hw_stop(); break;
							}
						}

						vexriscv_write_register((instr >> 7) & 0x1f, old);
						csr_write(mepc, mepc + 4);

					} break;
					default: hw_stop(); break;
				}
			} break;
			/* System call */
			case CAUSE_SCALL:{
				uint32_t which = vexriscv_read_register(17);
				uint32_t a0 = vexriscv_read_register(10);
				uint32_t a1 = vexriscv_read_register(11);
				__attribute__((unused)) uint32_t a2 = vexriscv_read_register(12);
				switch(which){
					case SBI_CONSOLE_PUTCHAR: {
						hw_putchar(a0);
						csr_write(mepc, csr_read(mepc) + 4);
					} break;
					case SBI_CONSOLE_GETCHAR: {
						vexriscv_write_register(10, hw_getchar());
						csr_write(mepc, csr_read(mepc) + 4);
					} break;
					case SBI_SET_TIMER: {
						hw_write_cpu_timer_cmp(a0, a1);
						csr_set(mie, MIE_MTIE);
						csr_clear(sip, MIP_STIP);
						csr_write(mepc, csr_read(mepc) + 4);
					} break;
					case SBI_EXT_BASE: {
						vexriscv_write_register(10, vexriscv_sbi_ext(a0, a1));
						csr_write(mepc, csr_read(mepc) + 4);
					} break;
					default: {
						vexriscv_write_register(10, -2); /* SBI_ERR_NOT_SUPPORTED */
						csr_write(mepc, csr_read(mepc) + 4);
					} break;
				}
			} break;
			default: hw_stop(); break;
		}
	}
}

static void vexriscv_machine_mode_boot(void) {
	__asm__ __volatile__ (
		" li a0, 0\n"
		" li a1, %0\n"
		" mret"
		 : : "i" (LINUX_DTB_BASE)
	);
}


/* ------------------------------------------------------------------------ */
/* Main                                                                     */
/* ------------------------------------------------------------------------ */

int main(void)
{
	irq_setmask(0);
	irq_setie(1);
	vexriscv_machine_mode_init();
	vexriscv_machine_mode_boot();
	return 0;
}
