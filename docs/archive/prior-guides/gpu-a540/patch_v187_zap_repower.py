#!/usr/bin/env python3
# v187: re-run GPUCC GDSC/PLL/RCG bring-up after zap auth_and_reset.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 're-power after zap' in s:
    print('v187 already applied')
    raise SystemExit(0)

old = '''		zap_ok = sweet_zap_load();
		printf("SWEET GPU: zap %s\\n", zap_ok ? "OK" : "FAIL");
'''
new = '''		zap_ok = sweet_zap_load();
		printf("SWEET GPU: zap %s\\n", zap_ok ? "OK" : "FAIL");
		if (zap_ok) {
			/* re-power after zap: auth_and_reset may reset clocks. */
			vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
			if (gpuc) {
				gpu_wr(gpuc, 0x1004, 0x222000u);
				delay(10);
				gpu_wr(gpuc, 0x1090, 0x1u);
				delay(1);
				gpu_wr(gpuc, 0x1090, 0x0u);
				gpu_wr(gpuc, 0x0130, 0x11u);
				delay(1);
				gpu_wr(gpuc, 0x0130, 0x01u);
				delay(1);
				gpu_wr(gpuc, 0x0130, 0x00u);
				gpu_wr(gpuc, 0x1094, 0x222000u);
				delay(10);
				gpu_wr(gpuc, 0x0004, 0x12u);
				gpu_wr(gpuc, 0x0008, 0x3fu);
				gpu_wr(gpuc, 0x0038, 0xc00u);
				gpu_wr(gpuc, 0x0014, 0x20485699u);
				gpu_wr(gpuc, 0x0018, 0x2067u);
				gpu_wr(gpuc, 0x0010, 0x4805u);
				gpu_wr(gpuc, 0x000c, 0x7u);
				uint32_t m = gpu_rd(gpuc, 0x0000);
				gpu_wr(gpuc, 0x0000, m & ~1u);
				gpu_wr(gpuc, 0x002c, 0x0u);
				delay(1);
				gpu_wr(gpuc, 0x0000, (m & ~1u) | 0x4u);
				gpu_wr(gpuc, 0x002c, 0x1u);
				delay(100);
				gpu_wr(gpuc, 0x000c, gpu_rd(gpuc, 0x000c) | 0x7u);
				gpu_wr(gpuc, 0x0000, gpu_rd(gpuc, 0x0000) | 0x1u);
				gpu_wr(gpuc, 0x1074, 0x101u);
				gpu_wr(gpuc, 0x1070, 0x0u);
				delay(10);
				gpu_wr(gpuc, 0x1098, 0x1u);
				delay(10);
				printf("SWEET GPU: re-power GDSC=%#x/%#x PLL=%#x RCG=%#x br=%#x\\n",
				    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094),
				    gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x1074),
				    gpu_rd(gpuc, 0x1098));
			}
		}
'''
assert s.count(old) == 1
open(p, 'w').write(s.replace(old, new))
print('v187 applied')
