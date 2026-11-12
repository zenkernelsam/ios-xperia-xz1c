#!/usr/bin/env python3
import os
ROOT='/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p=os.path.join(ROOT,'bsd/dev/sweet_gpu.c')
s=open(p).read()
if 'variants' in s:
    print('already'); raise SystemExit(0)
old='gpu_wr(gpu, R_CP_RB_CNTL * 4u, 11u | (2u << 8) | (1u << 27));'
new='gpu_wr(gpu, R_CP_RB_CNTL * 4u, 11u | (2u << 8)); /* variants: no NO_UPDATE */'
assert s.count(old)==1
s=s.replace(old,new)
# no 4-byte header strip
s=s.replace('(uint32_t)(pm4 + 4)','(uint32_t)pm4')
s=s.replace('(uint32_t)((pm4 + 4) >> 32)','(uint32_t)(pm4 >> 32)')
s=s.replace('(uint32_t)(pfp + 4)','(uint32_t)pfp')
s=s.replace('(uint32_t)((pfp + 4) >> 32)','(uint32_t)(pfp >> 32)')
s=s.replace('flush_dcache(pm4 + 4, pm4_len - 4, TRUE);','flush_dcache(pm4, pm4_len, TRUE);')
s=s.replace('flush_dcache(pfp + 4, pfp_len - 4, TRUE);','flush_dcache(pfp, pfp_len, TRUE);')
open(p,'w').write(s)
print('v186 applied')
