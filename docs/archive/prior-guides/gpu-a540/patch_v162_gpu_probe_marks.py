#!/usr/bin/env python3
# v162: localise the GPU-probe hang. Add step markers inside SPMI init and
# around the probe's init call. Read-only; no new MMIO ranges.
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


edit('bsd/dev/sweet_spmi.c', """	sp_attempted = true;
	sp_lock = IOLockAlloc();
	if (!sp_lock) {
		return -1;
	}
	sp_core = ml_io_map(SP_CORE_PA, 0x2000);
	sp_cfg = ml_io_map(SP_CFG_PA, 0x1000);
	sp_obs = ml_io_map(SP_OBS_PA, 0x10000); /* EE 0, 512 * 0x80 */
	if (!sp_core || !sp_cfg || !sp_obs) {
		printf("SWEET BATTERY: SPMI mapping failed\\n");
		return -1;
	}
	uint32_t arb_version = sp_rd(sp_core, 0);
	sp_count = sp_rd(sp_core, 4) & 0x7ffu;
""", """	sp_attempted = true;
	printf("SWEET SPMI: init begin\\n");
	sp_lock = IOLockAlloc();
	printf("SWEET SPMI: lock %p\\n", sp_lock);
	if (!sp_lock) {
		return -1;
	}
	sp_core = ml_io_map(SP_CORE_PA, 0x2000);
	printf("SWEET SPMI: core %#lx\\n", sp_core);
	sp_cfg = ml_io_map(SP_CFG_PA, 0x1000);
	printf("SWEET SPMI: cfg %#lx\\n", sp_cfg);
	sp_obs = ml_io_map(SP_OBS_PA, 0x10000); /* EE 0, 512 * 0x80 */
	printf("SWEET SPMI: obs %#lx\\n", sp_obs);
	if (!sp_core || !sp_cfg || !sp_obs) {
		printf("SWEET BATTERY: SPMI mapping failed\\n");
		return -1;
	}
	uint32_t arb_version = sp_rd(sp_core, 0);
	printf("SWEET SPMI: arb %#x\\n", arb_version);
	sp_count = sp_rd(sp_core, 4) & 0x7ffu;
	printf("SWEET SPMI: apids %u\\n", sp_count);
""")

edit('bsd/dev/sweet_gpu.c', """	printf("SWEET GPU: probe level %d (SPMI read-only)\\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\\n");
		return;
	}
	sweet_spmi_describe(PM8998_S1_PPID);
""", """	printf("SWEET GPU: probe level %d (SPMI read-only)\\n", level);
	printf("SWEET GPU: calling spmi init\\n");
	int rc = sweet_spmi_init();
	printf("SWEET GPU: spmi init rc=%d\\n", rc);
	if (rc != 0) {
		printf("SWEET GPU: SPMI unavailable\\n");
		return;
	}
	printf("SWEET GPU: describe pm8998-s1\\n");
	sweet_spmi_describe(PM8998_S1_PPID);
""")

print('v162 edits applied:', n)
