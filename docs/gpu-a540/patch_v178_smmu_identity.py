#!/usr/bin/env python3
# v178: GPU SMMU identity map for gfx3d_user (level 8).
#   - 16 KB page for a 64-entry L1 (1 GB blocks, VA=PA, normal NC)
#   - CB0 at 0x48000 (qcom,gpu-offset), GR1 at 0x1000
#   - SMR0 = stream 0, S2CR0 = translate to CB0
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'SMMU identity' in s:
    print('v178 already applied')
    raise SystemExit(0)

old_inc = '#include <machine/machine_routines.h>\n#include <pexpert/pexpert.h>\n'
new_inc = ('#include <machine/machine_routines.h>\n'
           '#include <vm/vm_kern.h>\n'
           '#include <vm/pmap.h>\n'
           '#include <pexpert/pexpert.h>\n'
           'extern void flush_dcache(vm_offset_t, unsigned, int);\n')
assert s.count(old_inc) == 1
s = s.replace(old_inc, new_inc)

old_fn = '''static void
gpu_wr(vm_offset_t base, unsigned off, uint32_t v)
{
	*(volatile uint32_t *)(base + off) = v;
	__asm__ volatile("dsb osh" ::: "memory");
}
'''
new_fn = old_fn + '''
/* Physically contiguous, zeroed kernel memory; returns its PA (0 failure). */
static uint64_t
gpu_alloc(vm_offset_t *va, vm_size_t size)
{
	if (kmem_alloc_contig(kernel_map, va, size, PAGE_MASK, 0, 0,
	    KMA_DATA | KMA_ZERO | KMA_PERMANENT, VM_KERN_MEMORY_IOKIT) !=
	    KERN_SUCCESS) {
		return 0;
	}
	return ptoa_64(pmap_find_phys(kernel_pmap, (addr64_t)*va));
}
'''
assert s.count(old_fn) == 1
s = s.replace(old_fn, new_fn)

anchor = '\tint pause = 0;\n'
block = (
    '\tif (level >= 8) {\n'
    '\t\tvm_offset_t smmu = ml_io_map(0x05040000u, 0x00050000u);\n'
    '\t\tif (smmu) {\n'
    '\t\t\tvm_offset_t pt_va = 0;\n'
    '\t\t\tuint64_t pt = gpu_alloc(&pt_va, PAGE_SIZE);\n'
    '\t\t\tprintf("SWEET GPU: SMMU pt pa=%#llx va=%#lx\\n", pt, pt_va);\n'
    '\t\t\tif (pt) {\n'
    '\t\t\t\tuint64_t *l1 = (uint64_t *)pt_va;\n'
    '\t\t\t\tfor (uint64_t gb = 2; gb < 8; gb++) {\n'
    '\t\t\t\t\tl1[gb] = (gb << 30) | 1u | (1u << 6) |\n'
    '\t\t\t\t\t    (2u << 8) | (1u << 10);\n'
    '\t\t\t\t}\n'
    '\t\t\t\tflush_dcache(pt_va, PAGE_SIZE, FALSE);\n'
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
assert s.count(anchor) == 1
open(p, 'w').write(s.replace(anchor, block + anchor))
print('v178 applied')
