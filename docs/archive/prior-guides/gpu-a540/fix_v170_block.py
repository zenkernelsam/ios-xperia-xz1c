#!/usr/bin/env python3
# Repair the level>=4 block (heredoc lost the C string escapes).
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()
start = s.index('\tif (level >= 4) {')
end = s.index('\tint pause = 0;', start)
block = (
    '\tif (level >= 4) {\n'
    '\t\tvm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);\n'
    '\t\tprintf("SWEET GPU: gpuc=%#lx\\n", gpuc);\n'
    '\t\tif (gpuc) {\n'
    '\t\t\tprintf("SWEET GPU: CX_GDSCR %#x GX_GDSCR %#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094));\n'
    '\t\t\tprintf("SWEET GPU: GX_BCR %#x clamp %#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x1090), gpu_rd(gpuc, 0x0130));\n'
    '\t\t\tprintf("SWEET GPU: gfx3d_rcg %#x branch %#x\\n",\n'
    '\t\t\t    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1098));\n'
    '\t\t}\n'
    '\t}\n\n'
)
s = s[:start] + block + s[end:]
open(p, 'w').write(s)
print('block repaired')
