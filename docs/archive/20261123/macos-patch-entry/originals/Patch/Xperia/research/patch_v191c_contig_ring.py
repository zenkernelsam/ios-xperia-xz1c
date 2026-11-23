#!/usr/bin/env python3
# v191c: allocate physically contiguous ring (32 KiB) and scratch (16 KiB).
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'a540 ring_va' in s:
    print('v191c already applied')
    raise SystemExit(0)

old = '''static void gpu_wr(vm_offset_t base, unsigned off, uint32_t v);
static uint32_t gpu_rd(vm_offset_t base, unsigned off);
'''
new = old + 'static uint64_t gpu_alloc(vm_offset_t *va, vm_size_t size);\n'
assert s.count(old) == 1
s = s.replace(old, new)

old = '''	uint64_t ring_pa = (uint64_t)kvtophys((vm_offset_t)a540_ring);
	uint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)a540_scratch);
	memset(a540_ring, 0, sizeof(a540_ring));
	memset(a540_scratch, 0, sizeof(a540_scratch));
'''
new = '''	vm_offset_t ring_va = 0, scratch_va = 0;
	uint64_t ring_pa = gpu_alloc(&ring_va, 32768);
	uint64_t scratch_pa = gpu_alloc(&scratch_va, 16384);
	if (!ring_pa || !scratch_pa) {
		printf("SWEET GPU: a540 contig alloc failed\\n");
		return 0;
	}
	uint32_t *ring = (uint32_t *)ring_va;
	uint32_t *scratch = (uint32_t *)scratch_va;
	memset(ring, 0, 32768);
	memset(scratch, 0, 16384);
'''
assert s.count(old) == 1
s = s.replace(old, new)

s = s.replace('a540_ring[n++]', 'ring[n++]')
s = s.replace('flush_dcache((vm_offset_t)a540_ring, PAGE_SIZE * 2, FALSE);',
              'flush_dcache(ring_pa, 32768, TRUE);')
s = s.replace('flush_dcache((vm_offset_t)a540_scratch, PAGE_SIZE, FALSE);',
              'flush_dcache(scratch_pa, PAGE_SIZE, TRUE);')
s = s.replace('uint32_t got = a540_scratch[0];', 'uint32_t got = scratch[0];')

open(p, 'w').write(s)
print('v191c applied')
