#!/usr/bin/env python3
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/osfmk/arm64/sleh.c'
s = open(p).read()

old = '''#if defined(SWEET)
\tpanic_plain("%s pc %llx lr %llx esr %x far %llx x20 %llx x21 %llx x8 %llx x9 %llx x10 %llx ss %p%s\\n"
\t    "x0 %llx x1 %llx x2 %llx x3 %llx x4 %llx x5 %llx x6 %llx x7 %llx x11 %llx x12 %llx\\n",
\t    msg, state->pc, state->lr, state->esr, state->far,
\t    state->x[20], state->x[21], state->x[8], state->x[9], state->x[10],
\t    ss, (ss_valid ? "" : " INVALID"),
\t    state->x[0], state->x[1], state->x[2], state->x[3],
\t    state->x[4], state->x[5], state->x[6], state->x[7],
\t    state->x[11], state->x[12]);
#else'''
assert s.count(old) == 1, s.count(old)

new = '''#if defined(SWEET)
\t/* The panic banner draws the formatted string as-is and does not wrap:
\t * keep every diagnostic line short enough to fit the 720 px console. */
\tpanic_plain("%s\\npc %llx lr %llx esr %x\\nfar %llx\\nx20 %llx x21 %llx\\n"
\t    "x8 %llx x9 %llx\\nx10 %llx ss %p%s\\nx0 %llx x1 %llx\\n"
\t    "x2 %llx x3 %llx x4 %llx x5 %llx\\n",
\t    msg, state->pc, state->lr, state->esr, state->far,
\t    state->x[20], state->x[21],
\t    state->x[8], state->x[9], state->x[10],
\t    ss, (ss_valid ? "" : " INVALID"),
\t    state->x[0], state->x[1], state->x[2], state->x[3],
\t    state->x[4], state->x[5]);
#else'''
s = s.replace(old, new)
open(p, 'w').write(s)
print('patched v148')
