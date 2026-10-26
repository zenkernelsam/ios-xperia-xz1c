import os
p='/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/bsd/dev/sweet_gpu.c'
s=open(p).read()
line='static uint64_t smmu_l1[2048] __attribute__((aligned(16384)));\n'
assert s.count(line)==1, s.count(line)
s=s.replace(line,'')
anchor='static uint32_t a540_scratch[4096] __attribute__((aligned(16384)));\n'
assert s.count(anchor)==1
s=s.replace(anchor, anchor+line)
open(p,'w').write(s)
print('moved smmu_l1')
