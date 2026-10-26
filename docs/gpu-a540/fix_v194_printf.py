#!/usr/bin/env python3
import os
p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()
bad = '\tprintf("SWEET GPU: a540 trust after reset=%#x\n",\n'
good = '\tprintf("SWEET GPU: a540 trust after reset=%#x' + chr(92) + 'n",\n'
assert s.count(bad) == 1, s.count(bad)
open(p, 'w').write(s.replace(bad, good))
print('fixed')
