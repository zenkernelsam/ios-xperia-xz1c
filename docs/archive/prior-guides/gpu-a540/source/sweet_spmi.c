/*
 * Battery-only adaptation of input/sweet_keys.c, observer EXT_READL.
 * PMIC READ ONLY. The only MMIO store issues a one-byte read command.
 * No interrupt clear, peripheral enable, charger config, or PMIC writes.
 * Init is serialized by the single built-in SweetBattery personality.
 * Battery APIDs are disjoint from the existing keys' 008/4c1 APIDs.
 */
#if defined(ARM64_BOARD_CONFIG_SWEET)
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/machine_routines.h>
#include <IOKit/IOLocks.h>
#include "sweet_spmi.h"

extern vm_offset_t ml_io_map(vm_offset_t, vm_size_t);

#define SP_CORE_PA 0x0800f000u
#define SP_OBS_PA  0x09400000u
#define SP_CFG_PA  0x0800a000u
/* EE0 channel base is 0, one 0x8000 slot per APID (shim-verified). */
#define SP_CHAN_STRIDE 0x8000u
#define SP_NO_APID 0xffffu

#define SP_WR_PA 0x08400000u
static vm_offset_t sp_core, sp_cfg;
static vm_offset_t sp_obs_ch[512];	/* lazily mapped EE0 observer lanes */
static vm_offset_t sp_wr_ch[512];	/* lazily mapped EE0 write lanes */
static IOLock *sp_lock;
static unsigned sp_count;
static bool sp_attempted, sp_ready;
/* A timed-out command may complete late: never reuse that lane this boot. */
static bool sp_poisoned[512];

static uint32_t
sp_rd(vm_offset_t base, unsigned off)
{
	uint32_t value = *(volatile uint32_t *)(base + off);
	__asm__ volatile("dmb oshld" ::: "memory");
	return value;
}

/* The observer window is 16 MB; kernel VA cannot spare it. Map only the
 * 0x8000 lane of the APID in use, once. Caller holds sp_lock. */
static vm_offset_t
sp_obs_lane(unsigned apid)
{
	if (!sp_obs_ch[apid]) {
		sp_obs_ch[apid] = ml_io_map(
		    SP_OBS_PA + SP_CHAN_STRIDE * (vm_offset_t)apid,
		    SP_CHAN_STRIDE);
	}
	return sp_obs_ch[apid];
}

static vm_offset_t
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
{
	uint16_t found = SP_NO_APID;
	for (unsigned i = 0; i < sp_count; ++i) {
		uint32_t map = sp_rd(sp_core, 0x800u + 4u * i);
		if (!map || ((map >> 8) & 0xfffu) != ppid) {
			continue;
		}
		unsigned owner = sp_rd(sp_cfg, 0x700u + 4u * i) & 7u;
		if (found == SP_NO_APID || owner == 0) {
			found = (uint16_t)i;
		}
	}
	return found;
}

int
sweet_spmi_init(void)
{
	if (sp_attempted) {
		return sp_ready ? 0 : -1;
	}
	sp_attempted = true;
	printf("SWEET SPMI: init begin\n");
	sp_lock = IOLockAlloc();
	printf("SWEET SPMI: lock %p\n", sp_lock);
	if (!sp_lock) {
		return -1;
	}
	sp_core = ml_io_map(SP_CORE_PA, 0x1000);
	printf("SWEET SPMI: core %#lx\n", sp_core);
	sp_cfg = ml_io_map(SP_CFG_PA, 0x1000);
	printf("SWEET SPMI: cfg %#lx\n", sp_cfg);
	if (!sp_core || !sp_cfg) {
		printf("SWEET SPMI: mapping failed\n");
		return -1;
	}
	uint32_t arb_version = sp_rd(sp_core, 0);
	sp_count = 512;	/* map occupies core[0x800..0xfff], 4 bytes each */
	printf("SWEET SPMI: arb %#x apids %u\n", arb_version, sp_count);
	if (arb_version < 0x20000000u || arb_version >= 0x70000000u) {
		printf("SWEET SPMI: unsupported arb=%#x\n", arb_version);
		return -1;
	}
	sp_ready = true;
	printf("SWEET BATTERY: SPMI arb=%#x APIDs=%u, observer reads only\n",
	    arb_version, sp_count);
	return 0;
}

int
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
	vm_offset_t ch = sp_obs_lane(apid);
	if (!ch) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	/* CAF regmap: 8-bit addresses use EXT_READ, 16-bit use EXT_READL. */
	unsigned opc = addr > 0xffu ? 1u : 13u;
	uint32_t cmd = (opc << 27) | ((addr & 0xffu) << 4);
	__asm__ volatile("dmb oshst" ::: "memory");
	*(volatile uint32_t *)(ch) = cmd; /* EXT_READL, BC=0 */
	__asm__ volatile("dsb osh" ::: "memory");
	for (unsigned i = 0; i < 400; ++i) {
		uint32_t status = sp_rd(ch, 0x08u);
		if (status & 1u) {
			if (status & 0x0eu) {
				IOLockUnlock(sp_lock);
				return -1;
			}
			*out = (uint8_t)sp_rd(ch, 0x18u);
			IOLockUnlock(sp_lock);
			return 0;
		}
		delay(1);
	}
	sp_poisoned[apid] = true;
	printf("SWEET SPMI: timeout ppid=%#x addr=%#x APID=%u disabled\n",
	    ppid, addr, apid);
	IOLockUnlock(sp_lock);
	return -1;
}

int
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
				printf("SWEET SPMI: write %#x/%#x status %#x\n",
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
	printf("SWEET SPMI: write timeout ppid=%#x addr=%#x APID=%u\n",
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
		uint32_t map = sp_rd(sp_core, 0x800u + 4u * i);
		if (!map || ((map >> 8) & 0xfffu) != ppid) {
			continue;
		}
		printf("SWEET SPMI: ppid %#x APID %u owner %u\n", ppid, i,
		    sp_rd(sp_cfg, 0x700u + 4u * i) & 7u);
		++found;
	}
	IOLockUnlock(sp_lock);
	if (!found) {
		printf("SWEET SPMI: ppid %#x not mapped\n", ppid);
	}
	return (int)found;
}
#endif /* ARM64_BOARD_CONFIG_SWEET */
