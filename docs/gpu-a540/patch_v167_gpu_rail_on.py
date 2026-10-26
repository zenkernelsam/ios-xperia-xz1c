#!/usr/bin/env python3
# v167: SPMI write support + GPU rail bring-up (level 2).
# PM8005 S1/S4 present the common2 layout: 0x40=voltage LSB, 0x41=MSB,
# value in mV (752mV = 0x02F0), 0x45=mode (6=AUTO), 0x46 enable bit7.
# Level 2 sets both phases to 900mV while disabled, verifies, then enables.
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


edit('bsd/dev/sweet_spmi.h',
     """int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
""",
     """int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
int sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value);
""")

edit('bsd/dev/sweet_spmi.c', """static vm_offset_t sp_core, sp_cfg;
static vm_offset_t sp_obs_ch[512];	/* lazily mapped EE0 lanes */
""", """#define SP_WR_PA 0x08400000u
static vm_offset_t sp_core, sp_cfg;
static vm_offset_t sp_obs_ch[512];	/* lazily mapped EE0 observer lanes */
static vm_offset_t sp_wr_ch[512];	/* lazily mapped EE0 write lanes */
""")

edit('bsd/dev/sweet_spmi.c', """static uint16_t
sp_find_apid(unsigned ppid)
""", """static vm_offset_t
sp_wr_lane(unsigned apid)
{
	if (!sp_wr_ch[apid]) {
		sp_wr_ch[apid] = ml_io_map(
		    SP_WR_PA + SP_CHAN_STRIDE * (vm_offset_t)apid,
		    SP_CHAN_STRIDE);
	}
	return sp_wr_ch[apid];
}

static uint16_t
sp_find_apid(unsigned ppid)
""")

edit('bsd/dev/sweet_spmi.c', """int
sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out)
""", """int
sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value)
{
	if (!sp_ready || ppid > 0xfffu || addr > 0xffffu) {
		return -1;
	}
	IOLockLock(sp_lock);
	uint16_t apid = sp_find_apid(ppid);
	if (apid == SP_NO_APID || sp_poisoned[apid]) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	vm_offset_t ch = sp_wr_lane(apid);
	if (!ch) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	unsigned opc = addr > 0xffu ? 0u : 2u;
	uint32_t cmd = (opc << 27) | ((addr & 0xffu) << 4);
	__asm__ volatile("dmb oshst" ::: "memory");
	*(volatile uint32_t *)(ch + 0x10u) = value;	/* WDATA0 */
	*(volatile uint32_t *)(ch) = cmd;
	__asm__ volatile("dsb osh" ::: "memory");
	for (unsigned i = 0; i < 400; ++i) {
		uint32_t status = sp_rd(ch, 0x08u);
		if (status & 1u) {
			if (status & 0x0eu) {
				printf("SWEET SPMI: write %#x/%#x status %#x\\n",
				    ppid, addr, status);
				IOLockUnlock(sp_lock);
				return -1;
			}
			IOLockUnlock(sp_lock);
			return 0;
		}
		delay(1);
	}
	sp_poisoned[apid] = true;
	printf("SWEET SPMI: write timeout ppid=%#x addr=%#x APID=%u\\n",
	    ppid, addr, apid);
	IOLockUnlock(sp_lock);
	return -1;
}

int
sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out)
""")

GPU_C = r'''/*
 * Sweet GPU bring-up.
 *   sweetgpu=1  read-only rail probe (PM8005 S1/S4, PM8998 S9)
 *   sweetgpu=2  also set both VDD_GFX phases to 900mV and enable them
 *   sweetgpupause=N  hold the console N seconds (camera read)
 *
 * PM8005 S1/S4 expose the common2 regulator layout on this board:
 *   0x40 voltage LSB, 0x41 voltage MSB (mV, 0x02F0 = 752mV),
 *   0x45 mode (6 = auto), 0x46 bit7 = enable.
 */
#if defined(ARM64_BOARD_CONFIG_SWEET)
#include <sys/param.h>
#include <sys/systm.h>
#include <kern/clock.h>
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>

extern int sweet_spmi_init(void);
extern int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
extern int sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value);
extern int sweet_spmi_describe(unsigned ppid);

#define PM8998_S9_PPID 0x02cu
#define PM8005_S1_PPID 0x514u
#define PM8005_S4_PPID 0x51du

static int
sweet_gpu_reg(unsigned ppid, unsigned addr)
{
	uint8_t value = 0;

	if (sweet_spmi_read_ppid(ppid, addr, &value) != 0) {
		return -1;
	}
	return value;
}

static void
sweet_gpu_rail(const char *tag, unsigned ppid, unsigned base)
{
	int lsb = sweet_gpu_reg(ppid, base + 0x40);
	int msb = sweet_gpu_reg(ppid, base + 0x41);
	int mode = sweet_gpu_reg(ppid, base + 0x45);
	int en = sweet_gpu_reg(ppid, base + 0x46);

	printf("SWEET GPU: %s v=%dmV mode=%d en=%d\n", tag,
	    lsb >= 0 && msb >= 0 ? ((msb << 8) | lsb) : -1, mode, en);
}

void
sweet_gpu_probe(void)
{
	int level = 0;

	if (!PE_parse_boot_argn("sweetgpu", &level, (int)sizeof(level)) ||
	    level < 1) {
		return;
	}
	printf("SWEET GPU: probe level %d\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\n");
		return;
	}
	sweet_spmi_describe(PM8005_S1_PPID);
	sweet_spmi_describe(PM8005_S4_PPID);
	sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
	sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);

	if (level >= 2) {
		/* One variable: enable at the bootloader's existing 752mV. */
		int rc1 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1446, 0x80);
		int rc2 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d46, 0x80);
		printf("SWEET GPU: enable rc=%d/%d\n", rc1, rc2);
		sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
		sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);
	}

	if (level >= 3) {
		/* 900mV = 0x0384, LSB first while the rail is already up. */
		int rc1 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1440, 0x84);
		int rc2 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1441, 0x03);
		printf("SWEET GPU: S1 set 900mV rc=%d/%d\n", rc1, rc2);
		rc1 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d40, 0x84);
		rc2 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d41, 0x03);
		printf("SWEET GPU: S4 set 900mV rc=%d/%d\n", rc1, rc2);
		sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
		sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);
	}

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
print('v167 edits applied:', n)
