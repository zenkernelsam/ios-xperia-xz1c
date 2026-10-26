#!/usr/bin/env python3
import os
ROOT='/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p=os.path.join(ROOT,'bsd/dev/sweet_gpu.c')
s=open(p).read()
if 'S2CR test' in s:
    print('already'); raise SystemExit(0)
old='\t\t\t\tgpu_wr(smmu, 0xc00, 0u);\n\t\t\t\tgpu_wr(smmu, 0x68, 0u);'
new=('\t\t\t\tgpu_wr(smmu, 0xc00, 0x2u);\n'
     '\t\t\t\tuint32_t s2f = gpu_rd(smmu, 0xc00);\n'
     '\t\t\t\tgpu_wr(smmu, 0xc00, 0x0u);\n'
     '\t\t\t\tuint32_t s2t = gpu_rd(smmu, 0xc00);\n'
     '\t\t\t\tgpu_wr(smmu, 0xc00, 0x1u);\n'
     '\t\t\t\tuint32_t s2b = gpu_rd(smmu, 0xc00);\n'
     '\t\t\t\tprintf("SWEET GPU: S2CR test fault=%#x trans=%#x bypass=%#x\n",\n'
     '\t\t\t\t    s2f, s2t, s2b);\n'
     '\t\t\t\tgpu_wr(smmu, 0xc00, 0x0u);\n'
     '\t\t\t\tgpu_wr(smmu, 0x68, 0u);')
assert s.count(old)==1
open(p,'w').write(s.replace(old,new))
print('v189 applied')
