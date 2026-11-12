#!/usr/bin/env python3
# v193: give the secure GPU streams (SID 1/2) CB1/CB2 identity tables and
# point SMR1/S2CR1 and SMR2/S2CR2 at them before the A540 start.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'a540 secure cb' in s:
    print('v193 already applied')
    raise SystemExit(0)

old = '''	printf("SWEET GPU: a540 aligned fw pm4=%#llx pfp=%#llx\\n",
	    pm4_base, pfp_base);
'''
new = old + '''
	/* secure streams may use CB1/CB2, not CB0: give both the same map. */
	vm_offset_t smmu = ml_io_map(0x05040000u, 0x00010000u);
	uint64_t pt = (uint64_t)kvtophys((vm_offset_t)smmu_l1);
	if (smmu && pt) {
		vm_offset_t gr1 = smmu + 0x1000u;
		for (uint32_t cb = 1; cb <= 2; cb++) {
			vm_offset_t cbb = smmu + 0x8000u + cb * 0x1000u;
			gpu_wr(cbb, 0x0, 0);
			gpu_wr(cbb, 0x10, (7u << 15) | 1u);
			gpu_wr(cbb, 0x30, 28u | (1u << 23));
			gpu_wr(cbb, 0x38, 0x44u);
			gpu_wr(cbb, 0x20, (uint32_t)pt);
			gpu_wr(cbb, 0x24, (uint32_t)(pt >> 32));
			gpu_wr(cbb, 0x58, 0xffffffffu);
			gpu_wr(cbb, 0x0, 7u);
			gpu_wr(gr1, 4u * cb, 1u << 16);
			gpu_wr(gr1, 0x800u + 4u * cb, 1u);
			gpu_wr(smmu, 0x800u + 4u * cb, (1u << 31) | cb);
			gpu_wr(smmu, 0xc00u + 4u * cb, cb);
		}
		gpu_wr(smmu, 0x68, 0);
		gpu_wr(smmu, 0x70, 0);
		printf("SWEET GPU: a540 secure cb SMR0=%#x SMR1=%#x SMR2=%#x S2CR0=%#x S2CR1=%#x S2CR2=%#x CB2=%#x CBAR2=%#x\\n",
		    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0x804), gpu_rd(smmu, 0x808),
		    gpu_rd(smmu, 0xc00), gpu_rd(smmu, 0xc04), gpu_rd(smmu, 0xc08),
		    gpu_rd(smmu, 0xa000), gpu_rd(gr1, 0x8));
	}
'''
assert s.count(old) == 1
open(p, 'w').write(s.replace(old, new))
print('v193 applied')
