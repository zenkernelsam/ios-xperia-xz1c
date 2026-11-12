#!/usr/bin/env python3
# v161: read-only GPU power probe.
#  - sweet_spmi: ppid-level read + APID owner dump (battery path unchanged)
#  - new bsd/dev/sweet_gpu.c: PM8005 S1/S4, PM8998 S1/S9 regulator state
#    behind boot-arg sweetgpu=1. No GPUCC MMIO yet (unclocked reads can hang).
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
    print('patched', rel)


HDR_OLD = """int sweet_spmi_init(void);
/* Thread context only; 0 success, -1 failure; *out unchanged on failure. */
int sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out);
"""
HDR_NEW = """int sweet_spmi_init(void);
/* Thread context only; 0 success, -1 failure; *out unchanged on failure. */
int sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out);
/* PMIC ppid form: ppid is SPMI's 12-bit (sid << 8) | peripheral. */
int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
/* Read-only diagnostic: list APIDs mapped to ppid, with owner EE bits. */
int sweet_spmi_describe(unsigned ppid);
"""
edit('bsd/dev/sweet_spmi.h', HDR_OLD, HDR_NEW)

TAIL_OLD = """int
sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out)
{
	/* Restrict access to the battery blocks; keys retain their own lanes. */
	unsigned peripheral = addr >> 8;
	if (!sp_ready || !out || sid != 0 || addr > 0xffffu ||
	    (peripheral != 0x10 && peripheral != 0x11 && peripheral != 0x12 &&
	    peripheral != 0x13 && peripheral != 0x48)) {
		return -1;
	}
	IOLockLock(sp_lock);
	uint16_t apid = sp_find_apid((sid << 8) | peripheral);
	if (apid == SP_NO_APID || sp_poisoned[apid]) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	unsigned off = 0x80u * apid;
	uint32_t cmd = (1u << 27) | ((addr & 0xffu) << 4);
	__asm__ volatile("dmb oshst" ::: "memory");
	*(volatile uint32_t *)(sp_obs + off) = cmd; /* EXT_READL, BC=0 */
	__asm__ volatile("dsb osh" ::: "memory");
	for (unsigned i = 0; i < 1000; ++i) {
		uint32_t status = sp_rd(sp_obs, off + 0x08u);
		if (status & 1u) {
			if (status & 0x0eu) {
				IOLockUnlock(sp_lock);
				return -1;
			}
			*out = (uint8_t)sp_rd(sp_obs, off + 0x18u);
			IOLockUnlock(sp_lock);
			return 0;
		}
		delay(1);
	}
	sp_poisoned[apid] = true;
	printf("SWEET BATTERY: SPMI timeout sid=%u addr=%#x APID=%u disabled\\n",
	    sid, addr, apid);
	IOLockUnlock(sp_lock);
	return -1;
}
"""
TAIL_NEW = """int
sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out)
{
	if (!sp_ready || !out || ppid > 0xfffu || addr > 0xffffu) {
		return -1;
	}
	IOLockLock(sp_lock);
	uint16_t apid = sp_find_apid(ppid);
	if (apid == SP_NO_APID || sp_poisoned[apid]) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	unsigned off = 0x80u * apid;
	uint32_t cmd = (1u << 27) | ((addr & 0xffu) << 4);
	__asm__ volatile("dmb oshst" ::: "memory");
	*(volatile uint32_t *)(sp_obs + off) = cmd; /* EXT_READL, BC=0 */
	__asm__ volatile("dsb osh" ::: "memory");
	for (unsigned i = 0; i < 1000; ++i) {
		uint32_t status = sp_rd(sp_obs, off + 0x08u);
		if (status & 1u) {
			if (status & 0x0eu) {
				IOLockUnlock(sp_lock);
				return -1;
			}
			*out = (uint8_t)sp_rd(sp_obs, off + 0x18u);
			IOLockUnlock(sp_lock);
			return 0;
		}
		delay(1);
	}
	sp_poisoned[apid] = true;
	printf("SWEET BATTERY: SPMI timeout ppid=%#x addr=%#x APID=%u disabled\\n",
	    ppid, addr, apid);
	IOLockUnlock(sp_lock);
	return -1;
}

int
sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out)
{
	/* Restrict access to the battery blocks; keys retain their own lanes. */
	unsigned peripheral = addr >> 8;
	if (sid != 0 || (peripheral != 0x10 && peripheral != 0x11 &&
	    peripheral != 0x12 && peripheral != 0x13 && peripheral != 0x48)) {
		return -1;
	}
	return sweet_spmi_read_ppid((sid << 8) | peripheral, addr, out);
}

int
sweet_spmi_describe(unsigned ppid)
{
	if (!sp_ready || ppid > 0xfffu) {
		return -1;
	}
	unsigned found = 0;
	IOLockLock(sp_lock);
	for (unsigned i = 0; i < sp_count; ++i) {
		uint32_t map = sp_rd(sp_core, 0x900u + 4u * i);
		if (!map || ((map >> 8) & 0xfffu) != ppid) {
			continue;
		}
		printf("SWEET SPMI: ppid %#x APID %u owner %u\\n", ppid, i,
		    sp_rd(sp_cfg, 0x700u + 4u * i) & 7u);
		++found;
	}
	IOLockUnlock(sp_lock);
	if (!found) {
		printf("SWEET SPMI: ppid %#x not mapped\\n", ppid);
	}
	return (int)found;
}
"""
edit('bsd/dev/sweet_spmi.c', TAIL_OLD, TAIL_NEW)

edit('bsd/conf/files',
     'bsd/dev/sweet_spmi.c\t\t\tstandard\n',
     'bsd/dev/sweet_spmi.c\t\t\tstandard\nbsd/dev/sweet_gpu.c\t\t\tstandard\n')

edit('bsd/kern/bsd_init.c',
     '''		sweet_autoreset_extend((uint64_t)sweetar);
		printf("SWEET: root up, auto-reset deadline -> %d s\\n", sweetar);
''',
     '''		sweet_autoreset_extend((uint64_t)sweetar);
		printf("SWEET: root up, auto-reset deadline -> %d s\\n", sweetar);
		{
			extern void sweet_gpu_probe(void);
			sweet_gpu_probe();
		}
''')

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

#define PM8998_S1_PPID 0x114u	/* CX */
#define PM8998_S9_PPID 0x119u	/* MX */
#define PM8005_S1_PPID 0x514u	/* VDD_GFX, two-phase with S4 */
#define PM8005_S4_PPID 0x517u
#define PM8005_S2_PPID 0x515u
#define PM8005_S3_PPID 0x516u

static void
sweet_gpu_reg(const char *name, unsigned ppid, unsigned addr)
{
	uint8_t value = 0;

	if (sweet_spmi_read_ppid(ppid, addr, &value) == 0) {
		printf("SWEET GPU: %s[%#05x] = %#04x\n", name, addr, value);
	} else {
		printf("SWEET GPU: %s[%#05x] read failed\n", name, addr);
	}
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
	sweet_spmi_describe(PM8005_S2_PPID);
	sweet_spmi_describe(PM8005_S3_PPID);
	sweet_spmi_describe(PM8005_S4_PPID);
	/* PMIC4 regulator block: 0x40 step size, 0x45 mode, 0x46 enable. */
	sweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1440);
	sweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1445);
	sweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1446);
	sweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x1940);
	sweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x1945);
	sweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x1946);
	sweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1440);
	sweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1445);
	sweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1446);
	sweet_gpu_reg("pm8005-s2", PM8005_S2_PPID, 0x1540);
	sweet_gpu_reg("pm8005-s2", PM8005_S2_PPID, 0x1546);
	sweet_gpu_reg("pm8005-s3", PM8005_S3_PPID, 0x1640);
	sweet_gpu_reg("pm8005-s3", PM8005_S3_PPID, 0x1646);
	sweet_gpu_reg("pm8005-s4", PM8005_S4_PPID, 0x1740);
	sweet_gpu_reg("pm8005-s4", PM8005_S4_PPID, 0x1746);
	printf("SWEET GPU: probe done\n");
}
#endif /* ARM64_BOARD_CONFIG_SWEET */
'''
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
assert not os.path.exists(p), 'sweet_gpu.c already exists'
open(p, 'w').write(GPU_C)
print('created bsd/dev/sweet_gpu.c')

print('v161 edits applied:', n)
