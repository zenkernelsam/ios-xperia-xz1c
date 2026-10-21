#!/usr/bin/env python3
"""Recount unified-diff hunk headers in a patch file (fixes 'corrupt patch')."""
import re, sys

path = sys.argv[1]
data = open(path, newline='').read().split('\n')
if data and data[-1] == '':
    data = data[:-1]
out = []
i = 0
changed = 0
while i < len(data):
    m = re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(.*)', data[i])
    if not m:
        out.append(data[i]); i += 1; continue
    os_, oc, ns, nc, tail = m.groups()
    j = i + 1; o = n = 0
    while j < len(data):
        line = data[j]
        if line.startswith('@@ ') or line.startswith('diff --git') or line.startswith('--- '):
            break
        c = line[:1]
        if c == '-':
            o += 1
        elif c == '+':
            n += 1
        elif c == '\\':
            pass
        else:
            o += 1; n += 1
        j += 1
    new = f'@@ -{os_},{o} +{ns},{n} @@{tail}'
    if new != data[i]:
        print(f'{path}:{i+1}: {data[i]}  ->  {new}')
        changed += 1
    out.append(new)
    i = j
open(path, 'w', newline='\n').write('\n'.join(out) + '\n')
print(f'{path}: {changed} hunk(s) recounted')
