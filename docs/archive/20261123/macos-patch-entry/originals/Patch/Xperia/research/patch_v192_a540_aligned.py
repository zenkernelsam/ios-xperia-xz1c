#!/usr/bin/env python3
# v192: page-aligned ucode buffers + halt ACK waits + TrustControl print.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'a540 aligned fw' in s:
    print('v192 already applied')
    raise SystemExit(0)

# aligned firmware copies
old = '''	uint64_t pm4_base = pm4 + 4, pfp_base = pfp + 4;
	uint32_t pm4_bytes = pm4_len - 4, pfp_bytes = pfp_len - 4;
'''
new = '''	vm_offset_t pm4_va = 0, pfp_va = 0;
	uint64_t pm4_base = gpu_alloc(&pm4_va, 0x8000);
	uint64_t pfp_base = gpu_alloc(&pfp_va, 0x8000);
	if (!pm4_base || !pfp_base) {
		printf("SWEET GPU: a540 fw alloc failed\\n");
		return 0;
	}
	memcpy((void *)pm4_va, (const void *)phystokv(pm4 + 4), pm4_len - 4);
	memcpy((void *)pfp_va, (const void *)phystokv(pfp + 4), pfp_len - 4);
	uint32_t pm4_bytes = pm4_len - 4, pfp_bytes = pfp_len - 4;
	printf("SWEET GPU: a540 aligned fw pm4=%#llx pfp=%#llx\\n",
	    pm4_base, pfp_base);
'''
assert s.count(old) == 1
s = s.replace(old, new)

# halt ACK wait + TrustControl print
old = '''	gpu_wr(gpu, 0x0832u * 4u, A540_ME_HALT);	/* MEControl halt */
	gpu_wr(gpu, 0x3080u * 4u, 15u);			/* VBIF halt */
	delay(100);
'''
new = '''	gpu_wr(gpu, 0x0832u * 4u, A540_ME_HALT);	/* MEControl halt */
	gpu_wr(gpu, 0x3080u * 4u, 15u);			/* VBIF halt */
	int halted = 0;
	for (int i = 0; i < 100; ++i) {
		if ((gpu_rd(gpu, 0x0832u * 4u) & A540_ME_HALT) == A540_ME_HALT &&
		    (gpu_rd(gpu, 0x3081u * 4u) & 15u) == 15u) {
			halted = 1; break;
		}
		delay(1000);
	}
	printf("SWEET GPU: a540 halt=%d ME=%#x VBIF=%#x trust=%#x\\n",
	    halted, gpu_rd(gpu, 0x0832u * 4u), gpu_rd(gpu, 0x3081u * 4u),
	    gpu_rd(gpu, 0xf400u * 4u));
'''
assert s.count(old) == 1
s = s.replace(old, new)

open(p, 'w').write(s)
print('v192 applied')
