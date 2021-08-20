/*
 * main.c
 *
 * Copyright (C) 2021 Sylvain Munaut
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "console.h"
#include "led.h"
#include "mini-printf.h"
#include "riscv.h"
#include "config.h"


/* ------------------------------------------------------------------------ */
/* Timer test                                                               */
/* ------------------------------------------------------------------------ */


struct wb_platform {
	uint32_t spi_csr;
	uint32_t spi_dat;
	uint32_t timer;
} __attribute__((packed,aligned(4)));

static volatile struct wb_platform * const plat_regs = (void*)(PLAT_BASE);


extern void _trap_entry(void);


static void
timer_set_next(void)
{
	uint32_t time = plat_regs->timer;
	printf("time   : %08x\n", time);
	plat_regs->timer = (time + 15000000) & 0xfffffffe;
}

void
trap(void)
{
        uint32_t mcause = csr_read(mcause);
		printf("mcause : %08x\n", mcause);
		timer_set_next();
		printf("\n");
}


/* ------------------------------------------------------------------------ */
/* Memory test                                                              */
/* ------------------------------------------------------------------------ */

static void
memtest1(void)
{
	uint8_t *psram_start = (uint8_t*) 0x40100000;
	uint8_t *psram_end   = (uint8_t*) 0x42000000;

	while (1) {
		uint8_t *psram;
		uint32_t c;

		c = 0x1234fdb9;
		for (psram = psram_start; psram != psram_end; psram++)
		{
			uint8_t v = (c ^ (c >> 16)) & 0xff;
			*psram = v;
			c += 0x1234fdb9;
		}

		puts("Write done\n");

		c = 0x1234fdb9;
		for (psram = psram_start; psram != psram_end; psram++)
		{
			uint8_t v = (c ^ (c >> 16)) & 0xff;
			if (*psram != v)
				printf("Err @ %08x %02x %02x\n", (uint32_t)psram);
			c += 0x1234fdb9;
		}

		puts("Check done\n");
	}
}


static void
memtest2(void)
{
	uint32_t*psram       = (uint32_t*) 0x40100000;

	int m = 123;
	uint32_t s;

	while (1) {
		// Write in random order
		s = 1;
		for (unsigned int o=0; o<(1024*1024); o++)
		{
			uint32_t v = s * m;
			psram[s] = v;
			s = (s >> 1) ^ (-(s & 1) & 0x80029);
		}

		puts("Write done\n");

		// Read in random order
		for (unsigned int o=0; o<(1024*1024); o++)
		{
			uint32_t v = s * m;
			uint32_t w = psram[s];

			s = (s >> 1) ^ (-(s & 1) & 0x80004);

			if (w != v)
				printf("Err @ %08x E:%08x R:%08x\n", s, v, w);
		}

		puts("Check done\n");

		// Update
		m += 7;
	}
}


/* ------------------------------------------------------------------------ */
/* Main                                                                     */
/* ------------------------------------------------------------------------ */

void main()
{
	int cmd = 0;

	/* Init console IO */
	console_init();
	puts("Booting Test App ...\n");

	/* LED */
	led_init();
	led_color(48, 96, 5);
	led_blink(true, 200, 1000);
	led_breathe(true, 100, 200);
	led_state(true);

	/* Main loop */
	while (1)
	{
		/* Prompt ? */
		if (cmd >= 0)
			printf("Command> ");

		/* Poll for command */
		cmd = getchar_nowait();

		if (cmd >= 0) {
			if (cmd > 32 && cmd < 127)
				putchar(cmd);
			putchar('\r');
			putchar('\n');

			switch (cmd)
			{
				/* Memory Test 1 */
			case '1':
				memtest1();
				break;

				/* Memory Test 2 */
			case '2':
				memtest2();
				break;

			case 't':
				/* Timer start */
				timer_set_next();
				csr_write(mtvec, _trap_entry);
				csr_set(mie, MIE_MTIE);
				csr_write(mstatus, MSTATUS_MIE);

			default:
				break;
			}
		}
	}
}
