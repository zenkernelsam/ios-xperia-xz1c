#!/usr/bin/env python3
# v177: read-only GPU SMMU probe (level 7). lilac: GR0 at 0x5040000,
# gfx3d_user context bank at +0x48000 (qcom,gpu-offset), micro-MMU at 0x6000.
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'SWEET GPU: SMMU ID0' in s:
    print('v177 already applied')
    raise SystemExit(0)

anchor = '\tint pause = 0;\n'
block = (
    '\tif (level >= 7) {\n'
    '\t\tvm_offset_t smmu = ml_io_map(0x05040000u, 0x00050000u);\n'
    '\t\tprintf("SWEET GPU: smmu=%#lx\\n", smmu);\n'
    '\t\tif (smmu) {\n'
    '\t\t\tprintf("SWEET GPU: SMMU ID0=%#x ID1=%#x SCR0=%#x\\n",\n'
    '\t\t\t    gpu_rd(smmu, 0x20), gpu_rd(smmu, 0x24),\n'
    '\t\t\t    gpu_rd(smmu, 0x0));\n'
    '\t\t\tprintf("SWEET GPU: SMMU SMR0=%#x S2CR0=%#x micro=%#x\\n",\n'
    '\t\t\t    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0xc00),\n'
    '\t\t\t    gpu_rd(smmu, 0x6000));\n'
    '\t\t\tvm_offset_t cb = smmu + 0x48000u;\n'
    '\t\t\tprintf("SWEET GPU: SMMU CB0 SCTLR=%#x TTBR0=%#x TCR=%#x FSR=%#x\\n",\n'
    '\t\t\t    gpu_rd(cb, 0x0), gpu_rd(cb, 0x20), gpu_rd(cb, 0x30),\n'
    '\t\t\t    gpu_rd(cb, 0x58));\n'
    '\t\t}\n'
    '\t}\n\n'
)
assert s.count(anchor) == 1
open(p, 'w').write(s.replace(anchor, block + anchor))
print('v177 applied')
