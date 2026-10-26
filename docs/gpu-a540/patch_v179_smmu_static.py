#!/usr/bin/env python3
# v179: static 16 KB SMMU L1 page + step markers (find the level-8 stall).
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'l8 enter' in s:
    print('v179 already applied')
    raise SystemExit(0)

anchor = '#define PM8005_S4_PPID 0x51du\n'
assert s.count(anchor) == 1, s.count(anchor)
s = s.replace(anchor, anchor +
              '\nstatic uint64_t smmu_l1[2048] __attribute__((aligned(16384)));\n')

start = s.index('\tif (level >= 8) {')
end = s.index('\tint pause = 0;', start)
block = (
    '\tif (level >= 8) {\n'
    '\t\tprintf("SWEET GPU: l8 enter\\n");\n'
    '\t\tvm_offset_t smmu = ml_io_map(0x05040000u, 0x00050000u);\n'
    '\t\tprintf("SWEET GPU: l8 smmu=%#lx\\n", smmu);\n'
    '\t\tif (smmu) {\n'
    '\t\t\tuint64_t pt = (uint64_t)kvtophys((vm_offset_t)smmu_l1);\n'
    '\t\t\tprintf("SWEET GPU: l8 pt=%#llx\\n", pt);\n'
    '\t\t\tif (pt) {\n'
    '\t\t\t\tfor (uint64_t gb = 2; gb < 8; gb++) {\n'
    '\t\t\t\t\tsmmu_l1[gb] = (gb << 30) | 1u | (1u << 6) |\n'
    '\t\t\t\t\t    (2u << 8) | (1u << 10);\n'
    '\t\t\t\t}\n'
    '\t\t\t\tflush_dcache((vm_offset_t)smmu_l1, PAGE_SIZE, FALSE);\n'
    '\t\t\t\tvm_offset_t cb = smmu + 0x48000u;\n'
    '\t\t\t\tvm_offset_t gr1 = smmu + 0x1000u;\n'
    '\t\t\t\tgpu_wr(cb, 0x0, 0);\n'
    '\t\t\t\tgpu_wr(gr1, 0x0, 1u << 16);\n'
    '\t\t\t\tgpu_wr(gr1, 0x800, 1u);\n'
    '\t\t\t\tgpu_wr(cb, 0x10, (7u << 15) | 1u);\n'
    '\t\t\t\tgpu_wr(cb, 0x30, 28u | (1u << 23));\n'
    '\t\t\t\tgpu_wr(cb, 0x38, 0x44u);\n'
    '\t\t\t\tgpu_wr(cb, 0x20, (uint32_t)pt);\n'
    '\t\t\t\tgpu_wr(cb, 0x24, (uint32_t)(pt >> 32));\n'
    '\t\t\t\tgpu_wr(cb, 0x58, 0xffffffffu);\n'
    '\t\t\t\tgpu_wr(cb, 0x0, (1u << 2) | (1u << 1) | 1u);\n'
    '\t\t\t\tprintf("SWEET GPU: l8 cb done\\n");\n'
    '\t\t\t\tgpu_wr(smmu, 0x800, 1u << 31);\n'
    '\t\t\t\tgpu_wr(smmu, 0xc00, 0u);\n'
    '\t\t\t\tgpu_wr(smmu, 0x68, 0u);\n'
    '\t\t\t\tgpu_wr(smmu, 0x70, 0u);\n'
    '\t\t\t\tint sync = 0;\n'
    '\t\t\t\tfor (int i = 0; i < 100; ++i) {\n'
    '\t\t\t\t\tif (!(gpu_rd(smmu, 0x74) & 1u)) { sync = 1; break; }\n'
    '\t\t\t\t\tdelay(1);\n'
    '\t\t\t\t}\n'
    '\t\t\t\tprintf("SWEET GPU: SMMU identity SMR0=%#x S2CR0=%#x CBAR0=%#x CBA2R0=%#x\\n",\n'
    '\t\t\t\t    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0xc00),\n'
    '\t\t\t\t    gpu_rd(gr1, 0x0), gpu_rd(gr1, 0x800));\n'
    '\t\t\t\tprintf("SWEET GPU: SMMU CB0 SCTLR=%#x TCR=%#x TTBR0=%#x FSR=%#x sync=%d\\n",\n'
    '\t\t\t\t    gpu_rd(cb, 0x0), gpu_rd(cb, 0x30), gpu_rd(cb, 0x20),\n'
    '\t\t\t\t    gpu_rd(cb, 0x58), sync);\n'
    '\t\t\t}\n'
    '\t\t}\n'
    '\t}\n\n'
)
s = s[:start] + block + s[end:]
open(p, 'w').write(s)
print('v179 applied')
