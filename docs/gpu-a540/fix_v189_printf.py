#!/usr/bin/env python3
import os
p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()
bad = ('\t\t\t\tprintf("SWEET GPU: S2CR test fault=%#x trans=%#x bypass=%#x\n",\n')
good = '\t\t\t\tprintf("SWEET GPU: S2CR test fault=%#x trans=%#x bypass=%#x' + chr(92) + 'n",\n'
assert s.count(bad) == 1, s.count(bad)
open(p, 'w').write(s.replace(bad, good))
print('fixed')
