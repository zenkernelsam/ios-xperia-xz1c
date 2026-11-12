#!/usr/bin/env python3
# Repair the RBBM printf (heredoc turned \n into a real newline).
import os

p = os.path.join('/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu', 'bsd/dev/sweet_gpu.c')
s = open(p).read()
bad = ('\t\t\t\tprintf("SWEET GPU: RBBM_STATUS %#x ERR %#x ST3 %#x\n",\n')
good = '\t\t\t\tprintf("SWEET GPU: RBBM_STATUS %#x ERR %#x ST3 %#x' + chr(92) + 'n",\n'
assert s.count(bad) == 1, s.count(bad)
open(p, 'w').write(s.replace(bad, good))
print('fixed')
