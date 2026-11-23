#!/usr/bin/env python3
# v198: pre-auth (pre-zap) A540 startup must not touch secure CB1/2 or
# SMMU secure registers (0x68 sACR / 0x70 sCTLR): doing so resets the SoC.
# sagit's bridge only owns the non-secure CB0 for SID 0 before PAS auth.
import sys

p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/bsd/dev/sweet_gpu.c'
s = open(p).read()

if 'secure_cb' in s:
    print('v198 already applied')
    sys.exit(0)

old = 'sweet_a540_start(void)'
assert s.count(old) == 1
s = s.replace(old, 'sweet_a540_start(int secure_cb)')

old = '\t/* secure streams may use CB1/CB2, not CB0: give both the same map. */\n\tvm_offset_t smmu = ml_io_map(0x05040000u, 0x00010000u);\n\tuint64_t pt = (uint64_t)kvtophys((vm_offset_t)smmu_l1);\n\tif (smmu && pt) {'
assert s.count(old) == 1, s.count(old)
new = '\t/* Pre-auth the SMMU is secure-owned: touching CB1/2 or sACR/sCTLR\n\t * (0x68/0x70) trips a secure violation and resets the SoC. */\n\tvm_offset_t smmu = secure_cb ? ml_io_map(0x05040000u, 0x00010000u) : 0;\n\tuint64_t pt = smmu ? (uint64_t)kvtophys((vm_offset_t)smmu_l1) : 0;\n\tif (secure_cb && smmu && pt) {'
s = s.replace(old, new)

old = '\tvm_offset_t ring_va = 0, scratch_va = 0;'
assert s.count(old) == 1
new = '\tif (!secure_cb)\n\t\tprintf("SWEET GPU: a540 secure cb skipped (pre-auth)\\n");\n\tvm_offset_t ring_va = 0, scratch_va = 0;'
s = s.replace(old, new)

old = '\t\tint ok13 = sweet_a540_start();'
assert s.count(old) == 1
s = s.replace(old, '\t\tint ok13 = sweet_a540_start(0);')

old = '\t\tint ok = sweet_a540_start();'
assert s.count(old) == 1
s = s.replace(old, '\t\tint ok = sweet_a540_start(1);')

open(p, 'w').write(s)
print('v198 applied')
