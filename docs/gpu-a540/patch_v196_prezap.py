#!/usr/bin/env python3
# v196: run A540 startup BEFORE zap (level 13), matching sagit's order.
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'A540 pre-zap' in s:
    print('v196 already applied')
    raise SystemExit(0)

# 1) remove the TrustControl write block from sweet_a540_start (line based)
lines = s.split('\n')
out = []
removed = 0
i = 0
while i < len(lines):
    if 'non-secure transition: sagit bridge' in lines[i]:
        # comment, gpu_wr, delay, printf, gpu_rd -> 5 lines
        del lines[i:i + 5]
        removed += 1
        continue
    i += 1
assert removed == 1, removed

# 2) zap only for levels 10..12
zap = '\tif (level >= 10) {'
assert lines.count(zap) == 1
lines[lines.index(zap)] = '\tif (level >= 10 && level < 13) {'

# 3) A540 start at level 12 runs post-zap; add pre-zap level 13 block
hdr = '\tif (level >= 12) {'
assert lines.count(hdr) == 1
idx = lines.index(hdr)
lines[idx] = '\tif (level >= 12 && level != 13) {'
pre = [
    '\tif (level >= 13) {',
    '\t\tsweet_gpu_step(0xb1);',
    '\t\tprintf("SWEET GPU: A540 pre-zap A540 start' + chr(92) + 'n");',
    '\t\tint ok13 = sweet_a540_start();',
    '\t\tprintf("SWEET GPU: A540 pre-zap %s' + chr(92) + 'n", ok13 ? "OK" : "FAIL");',
    '\t}',
    '',
]
lines[idx:idx] = pre

s = '\n'.join(lines)
open(p, 'w').write(s)
print('v196 applied')
