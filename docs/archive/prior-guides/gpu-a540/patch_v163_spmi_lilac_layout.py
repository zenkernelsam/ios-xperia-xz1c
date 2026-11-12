#!/usr/bin/env python3
# v163: fix kernel SPMI to the shim-verified lilac/msm8998 layout:
#   core 0x0800f000 (APID map at +0x800, 4B/entry, 512 entries)
#   cnfg 0x0800a000 (ownership at +0x700)
#   obs  0x09400000, chnls 0x08400000, channel stride 0x8000 (EE0)
# The old bases (0x0c440000/0x0e600000) are not SPMI on this SoC; the first
# register read hung the bus and the WDT reset the phone.
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


edit('bsd/dev/sweet_spmi.c', """#define SP_CORE_PA 0x0c440000u
#define SP_OBS_PA  0x0e600000u
#define SP_CFG_PA  0x0c40a000u
""", """#define SP_CORE_PA 0x0800f000u
#define SP_OBS_PA  0x09400000u
#define SP_CFG_PA  0x0800a000u
/* EE0 channel base is 0, one 0x8000 slot per APID (shim-verified). */
#define SP_CHAN_STRIDE 0x8000u
""")

edit('bsd/dev/sweet_spmi.c',
     "\t\tuint32_t map = sp_rd(sp_core, 0x900u + 4u * i);\n",
     "\t\tuint32_t map = sp_rd(sp_core, 0x800u + 4u * i);\n")
edit('bsd/dev/sweet_spmi.c',
     "\t\tuint32_t map = sp_rd(sp_core, 0x900u + 4u * i);\n",
     "\t\tuint32_t map = sp_rd(sp_core, 0x800u + 4u * i);\n")

edit('bsd/dev/sweet_spmi.c', """	sp_core = ml_io_map(SP_CORE_PA, 0x2000);
""", """	sp_core = ml_io_map(SP_CORE_PA, 0x1000);
""")
edit('bsd/dev/sweet_spmi.c',
     "\tsp_obs = ml_io_map(SP_OBS_PA, 0x10000); /* EE 0, 512 * 0x80 */\n",
     "\tsp_obs = ml_io_map(SP_OBS_PA, 0x1000000); /* EE 0, 512 * 0x8000 */\n")

edit('bsd/dev/sweet_spmi.c', """	uint32_t arb_version = sp_rd(sp_core, 0);
	printf("SWEET SPMI: arb %#x\\n", arb_version);
	sp_count = sp_rd(sp_core, 4) & 0x7ffu;
	printf("SWEET SPMI: apids %u\\n", sp_count);
	if (arb_version < 0x50000000u || arb_version >= 0x70000000u ||
	    !sp_count || sp_count > 512) {
		printf("SWEET BATTERY: unsupported SPMI arb=%#x APIDs=%u\\n",
		    arb_version, sp_count);
		return -1;
	}
""", """	uint32_t arb_version = sp_rd(sp_core, 0);
	sp_count = 512;	/* map occupies core[0x800..0xfff], 4 bytes each */
	printf("SWEET SPMI: arb %#x apids %u\\n", arb_version, sp_count);
	if (arb_version < 0x20000000u || arb_version >= 0x70000000u) {
		printf("SWEET SPMI: unsupported arb=%#x\\n", arb_version);
		return -1;
	}
""")

edit('bsd/dev/sweet_spmi.c',
     "\tunsigned off = 0x80u * apid;\n",
     "\tunsigned off = SP_CHAN_STRIDE * apid;\n")

print('v163 edits applied:', n)
