#!/usr/bin/env python3
# v182: use the standard ARM SMMU CB0 offset 0x8000 (0x48000 reads hung),
# and shrink the SMMU mapping to 64 KB.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'CB0@0x8000' in s:
    print('v182 already applied')
    raise SystemExit(0)

n = s.count('smmu + 0x48000u')
assert n == 2, n
s = s.replace('smmu + 0x48000u', 'smmu + 0x8000u')

n = s.count('0x05040000u, 0x00050000u')
assert n == 2, n
s = s.replace('0x05040000u, 0x00050000u', '0x05040000u, 0x00010000u')

# mark the change
old = '\t\t\tvm_offset_t cb = smmu + 0x8000u;\n\t\t\tprintf("SWEET GPU: SMMU CB0 SCTLR=%#x TTBR0=%#x TCR=%#x FSR=%#x\\n",\n'
new = '\t\t\t/* CB0@0x8000 (standard ARM SMMU); 0x48000 is not readable here. */\n' + old
assert s.count(old) == 1
s = s.replace(old, new)

open(p, 'w').write(s)
print('v182 applied')
