#!/usr/bin/env python3
# v166: correct SPMI read opcode (addr <= 0xFF uses EXT_READ, > 0xFF uses
# EXT_READL, per CAF regmap/arbiter) and cross-check PM8005 S1 identity:
# type/subtype, revid, and the same control byte through both address forms.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
n = 0


def edit(rel, old, new):
    global n
    p = os.path.join(ROOT, rel)
    s = open(p).read()
    assert s.count(old) == 1, (rel, s.count(old), old[:80])
    s = s.replace(old, new)
    open(p, 'w').write(s)
    n += 1


edit('bsd/dev/sweet_spmi.c', """	uint32_t cmd = (1u << 27) | ((addr & 0xffu) << 4);
""", """	/* CAF regmap: 8-bit addresses use EXT_READ, 16-bit use EXT_READL. */
	unsigned opc = addr > 0xffu ? 1u : 13u;
	uint32_t cmd = (opc << 27) | ((addr & 0xffu) << 4);
""")

GPU_C = r'''/*
 * Sweet GPU bring-up. First stage is read-only: sample the supplies that
 * feed the GPU before any GPUCC MMIO (unclocked accesses can hang the bus).
 * Drivers live behind boot-arg sweetgpu=<level>.
 */
#if defined(ARM64_BOARD_CONFIG_SWEET)
#include <sys/param.h>
#include <sys/systm.h>
#include <kern/clock.h>
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>

extern int sweet_spmi_init(void);
extern int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
extern int sweet_spmi_describe(unsigned ppid);

#define PM8998_S1_PPID 0x014u	/* CX; PM8998 enumerates at SPMI slave 0 */
#define PM8998_S9_PPID 0x02cu	/* MX */
#define PM8005_S1_PPID 0x514u	/* VDD_GFX, two-phase with S4 */
#define PM8005_S4_PPID 0x51du
#define PM8005_REVID_PPID 0x501u

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
	sweet_spmi_describe(PM8005_REVID_PPID);
	sweet_spmi_describe(PM8005_S1_PPID);
	sweet_spmi_describe(PM8005_S4_PPID);
	sweet_spmi_describe(PM8998_S9_PPID);

	/* Identity + self-test: same byte, 8-bit and 16-bit read forms. */
	int type = sweet_gpu_reg(PM8005_S1_PPID, 0x1404);
	int type8 = sweet_gpu_reg(PM8005_S1_PPID, 0x0004);
	int sub = sweet_gpu_reg(PM8005_S1_PPID, 0x1405);
	int revid = sweet_gpu_reg(PM8005_REVID_PPID, 0x0100);
	int rng = sweet_gpu_reg(PM8005_S1_PPID, 0x1440);
	int rng8 = sweet_gpu_reg(PM8005_S1_PPID, 0x0040);
	int set = sweet_gpu_reg(PM8005_S1_PPID, 0x1441);
	int set8 = sweet_gpu_reg(PM8005_S1_PPID, 0x0041);
	int mode = sweet_gpu_reg(PM8005_S1_PPID, 0x1445);
	int en = sweet_gpu_reg(PM8005_S1_PPID, 0x1446);
	int en8 = sweet_gpu_reg(PM8005_S1_PPID, 0x0046);
	int s4_en = sweet_gpu_reg(PM8005_S4_PPID, 0x1d46);
	int mx_en = sweet_gpu_reg(PM8998_S9_PPID, 0x2c46);

	printf("SWEET GPU: S1 type=%d/%d sub=%d revid=%d\n",
	    type, type8, sub, revid);
	printf("SWEET GPU: S1 rng=%d/%d set=%d/%d mode=%d en=%d/%d\n",
	    rng, rng8, set, set8, mode, en, en8);
	printf("SWEET GPU: S4 en=%d | MX S9 en=%d\n", s4_en, mx_en);

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
open(os.path.join(ROOT, 'bsd/dev/sweet_gpu.c'), 'w').write(GPU_C)
print('v166 edits applied:', n)
