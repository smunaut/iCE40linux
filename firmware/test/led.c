/*
 * led.c
 *
 * Copyright (C) 2019-2021 Sylvain Munaut
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "led.h"


struct ledda_ip {
	uint32_t _rsvd0;
	uint32_t pwrr;		/* 0001 LEDDPWRR - Pulse Width Register Red   */
	uint32_t pwrg;		/* 0010 LEDDPWRG - Pulse Width Register Green */
	uint32_t pwrb;		/* 0011 LEDDPWRB - Pulse Width Register Blue  */
	uint32_t _rsvd1;
	uint32_t bcrr;		/* 0101 LEDDBCRR - Breathe Control Rise Register */
	uint32_t bcfr;		/* 0101 LEDDBCFR - Breathe Control Fall Register */
	uint32_t _rsvd2;
	uint32_t cr0;		/* 1000 LEDDCR0  - Control Register 0 */
	uint32_t br;		/* 1001 LEDDBR   - Pre-scale Register */
	uint32_t onr;		/* 1010 LEDONR   - ON  Time Register */
	uint32_t ofr;		/* 1011 LEDOFR   - OFF Time Register */
} __attribute__((packed,aligned(4)));

#define LEDDA_IP_CR0_LEDDEN		(1 << 7)
#define LEDDA_IP_CR0_FR250		(1 << 6)
#define LEDDA_IP_CR0_OUTPOL		(1 << 5)
#define LEDDA_IP_CR0_OUTSKEW		(1 << 4)
#define LEDDA_IP_CR0_QUICK_STOP		(1 << 3)
#define LEDDA_IP_CR0_PWM_LINEAR		(0 << 2)
#define LEDDA_IP_CR0_PWM_LFSR		(1 << 2)
#define LEDDA_IP_CR0_SCALE_MSB(x)	(((x) >> 8) & 3)

#define LEDDA_IP_BR_SCALE_LSB(x)	((x) & 0xff)

#define LEDDA_IP_ONOFF_TIME_MS(x)	(((x) >> 5) & 0xff)	/*  32ms interval up to 8s */

#define LEDDA_IP_BREATHE_ENABLE		(1 << 7)
#define LEDDA_IP_BREATHE_MODULATE	(1 << 5)
#define LEDDA_IP_BREATHE_TIME_MS(x)	(((x) >> 7) & 0x0f)	/* 128ms interval up to 2s */


struct led {
	uint32_t csr;
	uint32_t _rsvd[15];
	struct ledda_ip ip;
} __attribute__((packed,aligned(4)));

#define LED_CSR_LEDDEXE		(1 << 1)
#define LED_CSR_RGBLEDEN	(1 << 2)
#define LED_CSR_CURREN		(1 << 3)


static volatile struct led * const led_regs = (void*)(LED_BASE);

static const uint32_t led_cr0_base =
	LEDDA_IP_CR0_FR250 |
	LEDDA_IP_CR0_OUTSKEW |
	LEDDA_IP_CR0_QUICK_STOP |
	LEDDA_IP_CR0_PWM_LFSR |
	LEDDA_IP_CR0_SCALE_MSB(480);


void
led_init(void)
{
	led_regs->ip.pwrr = 0;
	led_regs->ip.pwrg = 0;
	led_regs->ip.pwrb = 0;

	led_regs->ip.bcrr = 0;
	led_regs->ip.bcfr = 0;

	led_regs->ip.onr = 0;
	led_regs->ip.ofr = 0;

	led_regs->ip.br = LEDDA_IP_BR_SCALE_LSB(480);
	led_regs->ip.cr0 = led_cr0_base;

	led_regs->csr = LED_CSR_LEDDEXE | LED_CSR_RGBLEDEN | LED_CSR_CURREN;
}

void
led_color(uint8_t r, uint8_t g, uint8_t b)
{
#if defined(BOARD_ICEBREAKER)
/*	// iCEBreaker v1.0b tnt
	led_regs->ip.pwrr = r;
	led_regs->ip.pwrg = b;
	led_regs->ip.pwrb = g;
*/
	// iCEBreaker v1.0c+
	led_regs->ip.pwrr = b;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = r;
#elif defined(BOARD_BITSY_V0)
	// iCEBreaker bitsy v0 (RGB led 'hacked on')
	led_regs->ip.pwrr = g;
	led_regs->ip.pwrg = r;
	led_regs->ip.pwrb = b;
#elif defined(BOARD_BITSY_V1)
	// iCEBreaker bitsy v1 (RGB led 'hacked on')
	led_regs->ip.pwrr = r;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = b;
#elif defined(BOARD_ICE1USB)
	// icE1usb
	led_regs->ip.pwrr = b;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = r;
#elif defined(BOARD_ICEPICK)
	// iCEpick with UHD-1110 LED
	led_regs->ip.pwrr = b;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = r;

/*	// iCEpick with alternate LED
	led_regs->ip.pwrr = g;
	led_regs->ip.pwrg = r;
	led_regs->ip.pwrb = b;
*/
#elif defined(BOARD_E1TRACER)
	// E1 tracer
	led_regs->ip.pwrr = b;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = r;
#else
	// Default / Unknown
	led_regs->ip.pwrr = r;
	led_regs->ip.pwrg = g;
	led_regs->ip.pwrb = b;
#endif
}

void
led_state(bool on)
{
	if (on)
		led_regs->ip.cr0 = led_cr0_base | LEDDA_IP_CR0_LEDDEN;
	else
		led_regs->ip.cr0 = led_cr0_base;
}

void
led_blink(bool enabled, int on_time_ms, int off_time_ms)
{
	/* Disable EXE before doing any change */
	led_regs->csr = LED_CSR_RGBLEDEN | LED_CSR_CURREN;

	/* Load new config */
	if (enabled) {
		led_regs->ip.onr = LEDDA_IP_ONOFF_TIME_MS(on_time_ms);
		led_regs->ip.ofr = LEDDA_IP_ONOFF_TIME_MS(off_time_ms);
	} else {
		led_regs->ip.onr = 0;
		led_regs->ip.ofr = 0;
	}

	/* Re-enable execution */
	led_regs->csr = LED_CSR_LEDDEXE | LED_CSR_RGBLEDEN | LED_CSR_CURREN;
}

void
led_breathe(bool enabled, int rise_time_ms, int fall_time_ms)
{
	if (enabled) {
		led_regs->ip.bcrr = LEDDA_IP_BREATHE_ENABLE |
		                    LEDDA_IP_BREATHE_MODULATE |
		                    LEDDA_IP_BREATHE_TIME_MS(rise_time_ms);
		led_regs->ip.bcfr = LEDDA_IP_BREATHE_ENABLE |
		                    LEDDA_IP_BREATHE_MODULATE |
		                    LEDDA_IP_BREATHE_TIME_MS(fall_time_ms);
	} else {
		led_regs->ip.bcrr = 0;
		led_regs->ip.bcfr = 0;
	}
}
