#!/usr/bin/env python3
# v169 (corrected): read-only PLL0 + gfx3d RCG status (level 5), no clock writes.
# NOTE: use a separate file (not a shell heredoc) so C "\n" stays escaped.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()
if 'SWEET GPU: PLL mode=' in s:
    print('v169 block already present')
    raise SystemExit(0)
old = '\tif (level >= 4) {\n'
new = (
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
assert s.count(old) == 1
open(p, 'w').write(s.replace(old, new))
print('v169 applied')
