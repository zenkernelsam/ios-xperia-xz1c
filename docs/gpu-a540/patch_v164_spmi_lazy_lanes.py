#!/usr/bin/env python3
# v164: map SPMI observer channel lanes lazily, one 0x8000 window per APID.
# A 16 MB EE0 window exhausted kernel VA space and kernel_alloc failed.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_spmi.c')
s = open(p).read()
n = 0


def repl(old, new):
    global s, n
    assert s.count(old) == 1, (s.count(old), old[:80])
    s = s.replace(old, new)
    n += 1


repl("""static vm_offset_t sp_core, sp_obs, sp_cfg;
static IOLock *sp_lock;
static unsigned sp_count;
static bool sp_attempted, sp_ready;
/* A timed-out command may complete late: never reuse that lane this boot. */
static bool sp_poisoned[512];
""", """static vm_offset_t sp_core, sp_cfg;
static vm_offset_t sp_obs_ch[512];	/* lazily mapped EE0 lanes */
static IOLock *sp_lock;
static unsigned sp_count;
static bool sp_attempted, sp_ready;
/* A timed-out command may complete late: never reuse that lane this boot. */
static bool sp_poisoned[512];
""")

repl("""static uint32_t
sp_rd(vm_offset_t base, unsigned off)
{
	uint32_t value = *(volatile uint32_t *)(base + off);
	__asm__ volatile("dmb oshld" ::: "memory");
	return value;
}
""", """static uint32_t
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
""")

repl("""	sp_obs = ml_io_map(SP_OBS_PA, 0x1000000); /* EE 0, 512 * 0x8000 */
	printf("SWEET SPMI: obs %#lx\\n", sp_obs);
	if (!sp_core || !sp_cfg || !sp_obs) {
		printf("SWEET BATTERY: SPMI mapping failed\\n");
		return -1;
	}
""", """	if (!sp_core || !sp_cfg) {
		printf("SWEET SPMI: mapping failed\\n");
		return -1;
	}
""")

repl("""	unsigned off = SP_CHAN_STRIDE * apid;
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
""", """	vm_offset_t ch = sp_obs_lane(apid);
	if (!ch) {
		IOLockUnlock(sp_lock);
		return -1;
	}
	uint32_t cmd = (1u << 27) | ((addr & 0xffu) << 4);
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
""")

repl("""	printf("SWEET BATTERY: SPMI timeout ppid=%#x addr=%#x APID=%u disabled\\n",
""", """	printf("SWEET SPMI: timeout ppid=%#x addr=%#x APID=%u disabled\\n",
""")

open(p, 'w').write(s)
print('v164 edits applied:', n)
