#!/usr/bin/env python3
# v180: persistent step telemetry in PMIC PON scratch 0x88e + early readback.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'last step' in s:
    print('v180 already applied')
    raise SystemExit(0)

# helper after the write declaration block
anchor = 'extern int sweet_spmi_describe(unsigned ppid);\n'
assert s.count(anchor) == 1
s = s.replace(anchor, anchor +
    'extern int sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value);\n'
    '\n#define PON_PPID 0x008u\n#define PON_SCRATCH 0x088eu\n'
    'static void\nsweet_gpu_step(uint8_t code)\n{\n'
    '\t(void)sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, code);\n}\n')

# early readback + hold, right after SPMI init success in the probe
old = '''	printf("SWEET GPU: probe level %d\\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\\n");
		return;
	}
'''
new = '''	printf("SWEET GPU: probe level %d\\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\\n");
		return;
	}
	{
		uint8_t last = 0;
		if (sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &last) == 0) {
			printf("SWEET GPU: last step=%#x\\n", last);
			delay(15 * 1000000);
		}
	}
'''
assert s.count(old) == 1
s = s.replace(old, new)

# step codes in level 8
repls = [
    ('\t\tprintf("SWEET GPU: l8 enter\\n");\n',
     '\t\tsweet_gpu_step(0x80);\n\t\tprintf("SWEET GPU: l8 enter\\n");\n'),
    ('\t\tprintf("SWEET GPU: l8 smmu=%#lx\\n", smmu);\n',
     '\t\tsweet_gpu_step(0x81);\n\t\tprintf("SWEET GPU: l8 smmu=%#lx\\n", smmu);\n'),
    ('\t\t\tprintf("SWEET GPU: l8 pt=%#llx\\n", pt);\n',
     '\t\t\tsweet_gpu_step(0x82);\n\t\t\tprintf("SWEET GPU: l8 pt=%#llx\\n", pt);\n'),
    ('\t\t\t\tgpu_wr(cb, 0x0, 0);\n',
     '\t\t\t\tsweet_gpu_step(0x83);\n\t\t\t\tgpu_wr(cb, 0x0, 0);\n'),
    ('\t\t\t\tprintf("SWEET GPU: l8 cb done\\n");\n',
     '\t\t\t\tsweet_gpu_step(0x84);\n\t\t\t\tprintf("SWEET GPU: l8 cb done\\n");\n'),
    ('\t\t\t\tgpu_wr(smmu, 0x800, 1u << 31);\n',
     '\t\t\t\tsweet_gpu_step(0x85);\n\t\t\t\tgpu_wr(smmu, 0x800, 1u << 31);\n'),
    ('\t\t\t\tgpu_wr(smmu, 0xc00, 0u);\n',
     '\t\t\t\tsweet_gpu_step(0x86);\n\t\t\t\tgpu_wr(smmu, 0xc00, 0u);\n'),
    ('\t\t\t\tprintf("SWEET GPU: SMMU identity SMR0=%#x S2CR0=%#x CBAR0=%#x CBA2R0=%#x\\n",\n',
     '\t\t\t\tsweet_gpu_step(0x87);\n\t\t\t\tprintf("SWEET GPU: SMMU identity SMR0=%#x S2CR0=%#x CBAR0=%#x CBA2R0=%#x\\n",\n'),
]
for old, new in repls:
    assert s.count(old) == 1, (s.count(old), old[:50])
    s = s.replace(old, new)

# pause cap 30 -> 150
assert s.count('pause > 0 && pause <= 30') == 1
s = s.replace('pause > 0 && pause <= 30', 'pause > 0 && pause <= 150')

open(p, 'w').write(s)
print('v180 applied')
