#!/usr/bin/env python3
# v163b: finish the SPMI layout fix (v163 aborted after the define block).
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_spmi.c')
s = open(p).read()
n = 0


def repl(old, new, want):
    global s, n
    got = s.count(old)
    assert got == want, (got, want, old[:70])
    s = s.replace(old, new)
    n += 1


assert '#define SP_CHAN_STRIDE 0x8000u' in s

repl('\t\tuint32_t map = sp_rd(sp_core, 0x900u + 4u * i);\n',
     '\t\tuint32_t map = sp_rd(sp_core, 0x800u + 4u * i);\n', 2)
repl('\tsp_core = ml_io_map(SP_CORE_PA, 0x2000);\n',
     '\tsp_core = ml_io_map(SP_CORE_PA, 0x1000);\n', 1)
repl('\tsp_obs = ml_io_map(SP_OBS_PA, 0x10000); /* EE 0, 512 * 0x80 */\n',
     '\tsp_obs = ml_io_map(SP_OBS_PA, 0x1000000); /* EE 0, 512 * 0x8000 */\n', 1)
repl("""	uint32_t arb_version = sp_rd(sp_core, 0);
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
""", 1)
repl('\tunsigned off = 0x80u * apid;\n',
     '\tunsigned off = SP_CHAN_STRIDE * apid;\n', 1)

open(p, 'w').write(s)
print('v163b edits applied:', n)
