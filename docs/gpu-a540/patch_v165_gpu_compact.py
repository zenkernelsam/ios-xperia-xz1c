#!/usr/bin/env python3
# v165: compact GPU probe output + optional post-probe pause for camera reads.
#   sweetgpu=1        run the read-only probe
#   sweetgpupause=N   busy-wait N seconds (1..30) after printing, so the
#                     console stays visible before SpringBoard takes over
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')

GPU_C = r'''/*
 * Sweet GPU bring-up. First stage is read-only: sample the supplies that
 * feed the GPU before any GPUCC MMIO (unclocked accesses can hang the bus).
 * Drivers live behind boot-arg sweetgpu=<level>.
 */
#if defined(ARM64_BOARD_CONFIG_SWEET)
#include <sys/param.h>
#include <sys/systm.h>
#include <pexpert/pexpert.h>

extern int sweet_spmi_init(void);
extern int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
extern int sweet_spmi_describe(unsigned ppid);

#define PM8998_S1_PPID 0x014u	/* CX; PM8998 enumerates at SPMI slave 0 */
#define PM8998_S9_PPID 0x02cu	/* MX */
#define PM8005_S1_PPID 0x514u	/* VDD_GFX, two-phase with S4 */
#define PM8005_S2_PPID 0x517u
#define PM8005_S3_PPID 0x51au
#define PM8005_S4_PPID 0x51du

/* PMIC4 regulator block: 0x40 range, 0x41 voltage-set, 0x45 mode,
 * 0x46 enable. Values are raw codes; -1 means the read failed. */
static int
sweet_gpu_reg(unsigned ppid, unsigned addr)
{
	uint8_t value = 0;

	if (sweet_spmi_read_ppid(ppid, addr, &value) != 0) {
		return -1;
	}
	return value;
}

void
sweet_gpu_probe(void)
{
	int level = 0;

	if (!PE_parse_boot_argn("sweetgpu", &level, (int)sizeof(level)) ||
	    level < 1) {
		return;
	}
	printf("SWEET GPU: probe level %d (SPMI read-only)\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\n");
		return;
	}
	sweet_spmi_describe(PM8998_S1_PPID);
	sweet_spmi_describe(PM8998_S9_PPID);
	sweet_spmi_describe(PM8005_S1_PPID);
	sweet_spmi_describe(PM8005_S4_PPID);

	int gfx_rng = sweet_gpu_reg(PM8005_S1_PPID, 0x1440);
	int gfx_set = sweet_gpu_reg(PM8005_S1_PPID, 0x1441);
	int gfx_mode = sweet_gpu_reg(PM8005_S1_PPID, 0x1445);
	int gfx_en = sweet_gpu_reg(PM8005_S1_PPID, 0x1446);
	int gfx4_set = sweet_gpu_reg(PM8005_S4_PPID, 0x1d41);
	int gfx4_en = sweet_gpu_reg(PM8005_S4_PPID, 0x1d46);
	int cx_set = sweet_gpu_reg(PM8998_S1_PPID, 0x1441);
	int cx_en = sweet_gpu_reg(PM8998_S1_PPID, 0x1446);
	int mx_set = sweet_gpu_reg(PM8998_S9_PPID, 0x2c41);
	int mx_en = sweet_gpu_reg(PM8998_S9_PPID, 0x2c46);

	printf("SWEET GPU: GFX S1 rng=%d set=%d mode=%d en=%d\n",
	    gfx_rng, gfx_set, gfx_mode, gfx_en);
	printf("SWEET GPU: GFX S4 set=%d en=%d | CX S1 set=%d en=%d | MX S9 set=%d en=%d\n",
	    gfx4_set, gfx4_en, cx_set, cx_en, mx_set, mx_en);

	int pause = 0;
	if (PE_parse_boot_argn("sweetgpupause", &pause, (int)sizeof(pause)) &&
	    pause > 0 && pause <= 30) {
		printf("SWEET GPU: holding console %d s\n", pause);
		delay(pause * 1000000);
	}
	printf("SWEET GPU: probe done\n");
}
#endif /* ARM64_BOARD_CONFIG_SWEET */
'''
open(p, 'w').write(GPU_C)
print('v165 rewrote bsd/dev/sweet_gpu.c')
