/*
 * Qualcomm APSS watchdog (qcom,msm-watchdog / watchdog_v2.c) dead-man switch.
 * Verified register map: RST=0x04 EN=0x08 STS=0x0C BARK=0x10 BITE=0x14,
 * tick = 32765 Hz sleep clock. lilac DTB: bark 12000ms, pet 10000ms,
 * base 0x17817000.
 */
#include <stdint.h>
#include "wdt.h"

#define WDT_BASE	0x17817000UL
#define WDT_RST		0x04
#define WDT_EN		0x08
#define WDT_STS		0x0c
#define WDT_BARK	0x10
#define WDT_BITE	0x14
#define WDT_HZ		32765UL

static void wr32(uint64_t a, uint32_t v)
{
	*(volatile uint32_t *)a = v;
	__asm__ volatile("dsb sy" ::: "memory");
}

void wdt_arm(unsigned bark_s, unsigned bite_s)
{
	wr32(WDT_BASE + WDT_BARK, WDT_HZ * bark_s);
	wr32(WDT_BASE + WDT_BITE, WDT_HZ * bite_s);
	wr32(WDT_BASE + WDT_EN, 1);
}

void wdt_pet(void)
{
	wr32(WDT_BASE + WDT_RST, 1);
}

void wdt_disarm(void)
{
	wr32(WDT_BASE + WDT_EN, 0);
}
