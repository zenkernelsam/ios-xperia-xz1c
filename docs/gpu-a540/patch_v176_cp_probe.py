#!/usr/bin/env python3
# v176: read CP block registers after RBBM (all word offsets *4).
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
lines = open(p).read().split('\n')
out = []
done = False
for l in lines:
    out.append(l)
    if '0x530u * 4u));' in l:
        ind = l[:len(l) - len(l.lstrip())]
        out.append(ind + 'printf("SWEET GPU: CP rb=%#x me=%#x pfp=%#x pm4=%#x' + chr(92) + 'n",')
        out.append(ind + '    gpu_rd(gpu, 0x802u * 4u), gpu_rd(gpu, 0x832u * 4u),')
        out.append(ind + '    gpu_rd(gpu, 0x835u * 4u), gpu_rd(gpu, 0x838u * 4u));')
        done = True
assert done
open(p, 'w').write('\n'.join(out))
print('v176 applied')
