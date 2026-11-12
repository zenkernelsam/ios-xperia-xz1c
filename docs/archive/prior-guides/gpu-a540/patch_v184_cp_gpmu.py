#!/usr/bin/env python3
# v184: add GPMU RB/CCU/SP power and CP_RB_RPTR_ADDR to the CP test.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'GPMU rbccu' in s:
    print('v184 already applied')
    raise SystemExit(0)

# constants
old = '#define R_GPMU_SP_PWR_CLK_STATUS 0xa88bu\n'
assert s.count(old) == 1
s = s.replace(old, old +
    '#define R_CP_RB_RPTR_ADDR_LO 0x0804u\n'
    '#define R_CP_RB_RPTR_ADDR_HI 0x0805u\n')

# GPMU power + RPTR_ADDR after ring/scratch setup
old = '''			uint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)cp_scratch);
			printf("SWEET GPU: CP ring=%#llx scratch=%#llx\\n", ring_pa, scratch_pa);
			sweet_gpu_step(0x92);
'''
new = '''			uint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)cp_scratch);
			printf("SWEET GPU: CP ring=%#llx scratch=%#llx\\n", ring_pa, scratch_pa);
			sweet_gpu_step(0x92);
			/* a5xx_pm_resume: power the RB/CCU and SP islands. */
			gpu_wr(gpu, R_GPMU_RBCCU_POWER_CNTL * 4u, 0x778000u);
			int rbccu = 0;
			for (int i = 0; i < 200; ++i) {
				if (gpu_rd(gpu, R_GPMU_RBCCU_PWR_CLK_STATUS * 4u) & (1u << 20)) {
					rbccu = 1; break;
				}
				delay(100);
			}
			gpu_wr(gpu, R_GPMU_SP_POWER_CNTL * 4u, 0x778000u);
			int sp = 0;
			for (int i = 0; i < 200; ++i) {
				if (gpu_rd(gpu, R_GPMU_SP_PWR_CLK_STATUS * 4u) & (1u << 20)) {
					sp = 1; break;
				}
				delay(100);
			}
			printf("SWEET GPU: GPMU rbccu=%d(%#x) sp=%d(%#x)\\n",
			    rbccu, gpu_rd(gpu, R_GPMU_RBCCU_PWR_CLK_STATUS * 4u),
			    sp, gpu_rd(gpu, R_GPMU_SP_PWR_CLK_STATUS * 4u));
'''
assert s.count(old) == 1
s = s.replace(old, new)

# set RPTR_ADDR next to RB base
old = '''			gpu_wr(gpu, R_CP_RB_BASE * 4u, (uint32_t)ring_pa);
'''
new = '''			gpu_wr(gpu, R_CP_RB_RPTR_ADDR_LO * 4u, (uint32_t)(scratch_pa + 0x10));
			gpu_wr(gpu, R_CP_RB_RPTR_ADDR_HI * 4u,
			    (uint32_t)((scratch_pa + 0x10) >> 32));
			gpu_wr(gpu, R_CP_RB_BASE * 4u, (uint32_t)ring_pa);
'''
assert s.count(old) == 1
s = s.replace(old, new)

# extra status
old = '''			printf("SWEET GPU: CP RBBM=%#x HW_FAULT=%#x INT=%#x\\n",
'''
new = '''			printf("SWEET GPU: CP ME_CNTL=%#x RPTR=%u RPTR_ADDR=%#x%08x\\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u),
			    gpu_rd(gpu, R_CP_RB_RPTR * 4u),
			    gpu_rd(gpu, R_CP_RB_RPTR_ADDR_HI * 4u),
			    gpu_rd(gpu, R_CP_RB_RPTR_ADDR_LO * 4u));
			printf("SWEET GPU: CP RBBM=%#x HW_FAULT=%#x INT=%#x\\n",
'''
assert s.count(old) == 1
s = s.replace(old, new)

open(p, 'w').write(s)
print('v184 applied')
