#!/usr/bin/env python3
"""Recount unified-diff hunk headers in a patch file (fixes 'corrupt patch').

Reads the patch, recomputes the -old/+new line counts of every hunk from the
actual body lines, and rewrites the headers. Bodies are preserved verbatim.
"""
import re, sys

path = sys.argv[1]
data = open(path, encoding='utf-8', newline='').read().split('\n')
if data and data[-1] == '':
    data = data[:-1]

HUNK = re.compile(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(.*)')

out = []
i = 0
changed = 0
while i < len(data):
    m = HUNK.match(data[i])
    if not m:
        out.append(data[i]); i += 1; continue
    os_, oc, ns, nc, tail = m.groups()
    j = i + 1
    body = []
    while j < len(data):
        line = data[j]
        # a context/deleted/added line always starts with ' ', '-', '+', or '\';
        # anything else (next hunk, file header, diff header) ends the hunk.
        if line[:1] not in (' ', '-', '+', '\\') or HUNK.match(line):
            break
        body.append(line)
        j += 1
    o = sum(1 for l in body if l[:1] in (' ', '-'))
    n = sum(1 for l in body if l[:1] in (' ', '+'))
    new = f'@@ -{os_},{o} +{ns},{n} @@{tail}'
    if new != data[i]:
        print(f'{path}:{i+1}: {data[i]}  ->  {new}')
        changed += 1
    out.append(new)
    out.extend(body)
    i = j

open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
print(f'{path}: {changed} hunk(s) recounted')
