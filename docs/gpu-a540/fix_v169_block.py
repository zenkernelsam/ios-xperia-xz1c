#!/usr/bin/env python3
# Repair the level>=5 block (v169 patch wrote real newlines into C strings).
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()
start = s.index('\tif (level >= 5) {')
end = s.index('\tif (level >= 4) {', start)
block = (
    '\tif (level >= 5) {\n'
    '\t\tvm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);\n'
    '\t\tif (gpuc) {\n'
    '\t\t\tprintf("SWEET GPU: PLL mode=%#x opmode=%#x status=%#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x002c),\n'
    '\t\t\t    gpu_rd(gpuc, 0x0024));\n'
    '\t\t\tprintf("SWEET GPU: PLL L=%#x cal=%#x frac=%#x uctl=%#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x0004), gpu_rd(gpuc, 0x0008),\n'
    '\t\t\t    gpu_rd(gpuc, 0x0038), gpu_rd(gpuc, 0x000c));\n'
    '\t\t\tprintf("SWEET GPU: RCG cmd=%#x cfg=%#x m=%#x n=%#x d=%#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1074),\n'
    '\t\t\t    gpu_rd(gpuc, 0x1078), gpu_rd(gpuc, 0x107c),\n'
    '\t\t\t    gpu_rd(gpuc, 0x1080));\n'
    '\t\t\tprintf("SWEET GPU: rbbm_rcg=%#x br=%#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x10b0), gpu_rd(gpuc, 0x10d0));\n'
    '\t\t}\n'
    '\t}\n\n'
)
s = s[:start] + block + s[end:]
open(p, 'w').write(s)
print('block repaired')
