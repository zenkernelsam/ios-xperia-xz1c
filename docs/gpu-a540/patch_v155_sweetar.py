#!/usr/bin/env python3
"""v155: make sweetar=<seconds> actually control the auto-reset deadline.

Replaces the whole SWEET block in bsd_init.c (the earlier sed never hit).
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/bsd/kern/bsd_init.c'
s = open(p).read()

old = '''#if defined(SWEET)
\t{
\t\textern void sweet_early_step_late(unsigned int id);
\t\textern void sweet_autoreset_extend(uint64_t seconds);
\t\textern void sweet_early_ddr_rooted(void);
\t\tsweet_early_step_late(158);
\t\tsweet_early_ddr_rooted();
\t\tsweet_autoreset_extend(2700);
\t\tprintf("SWEET: root up, auto-reset deadline -> 15 min\\n");
\t}
#endif'''
assert s.count(old) == 1, s.count(old)

new = '''#if defined(SWEET)
\t{
\t\textern void sweet_early_step_late(unsigned int id);
\t\textern void sweet_autoreset_extend(uint64_t seconds);
\t\textern void sweet_early_ddr_rooted(void);
\t\textern boolean_t PE_parse_boot_argn(const char *, void *, int);
\t\tint sweetar = 2700;

\t\tsweet_early_step_late(158);
\t\tsweet_early_ddr_rooted();
\t\tif (!PE_parse_boot_argn("sweetar", &sweetar, (int)sizeof(sweetar)) ||
\t\t    sweetar < 60 || sweetar > 3600) {
\t\t\tsweetar = 2700;
\t\t}
\t\tsweet_autoreset_extend((uint64_t)sweetar);
\t\tprintf("SWEET: root up, auto-reset deadline -> %d s\\n", sweetar);
\t}
#endif'''
s = s.replace(old, new)
open(p, 'w').write(s)
print('v155 sweetar patched')
