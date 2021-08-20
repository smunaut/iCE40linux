/*
 * irq.h
 *
 * Copyright (c) 2019-2021, Linux-on-LiteX-VexRiscv Developers
 * Copyright (C) 2021 Sylvain Munaut
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "riscv.h"


#define CSR_IRQ_MASK 0xBC0
#define CSR_IRQ_PENDING 0xFC0

static inline unsigned int irq_getie(void)
{
	return (csr_read(mstatus) & MSTATUS_MIE) != 0;
}

static inline void irq_setie(unsigned int ie)
{
	if(ie) csr_set(mstatus,MSTATUS_MIE); else csr_clear(mstatus,MSTATUS_MIE);
}

static inline unsigned int irq_getmask(void)
{
	unsigned int mask;
	asm volatile ("csrr %0, %1" : "=r"(mask) : "i"(CSR_IRQ_MASK));
	return mask;
}

static inline void irq_setmask(unsigned int mask)
{
	asm volatile ("csrw %0, %1" :: "i"(CSR_IRQ_MASK), "r"(mask));
}

static inline unsigned int irq_pending(void)
{
	unsigned int pending;
	asm volatile ("csrr %0, %1" : "=r"(pending) : "i"(CSR_IRQ_PENDING));
	return pending;
}
