#!/usr/bin/env python3
"""v151: kernel exec ring (readable from fastboot via DDR) + panel status hook.

Adds to sweet_early.c:
  - char sweet_exec_ring[1024] filled by sweet_note_exec() on every exec
    ("<seq> <comm>\\n", rolling, dc civac flushed so it survives reset)
Hooks set_proc_name() in bsd/kern/kern_exec.c.
"""
import re

xu = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = xu + '/osfmk/arm64/sweet_early.c'
s = open(p).read()

anchor = '''void
sweet_early_putc(char c)
{'''
assert s.count(anchor) == 1
ring = '''/* Rolling log of every exec name; flushed to DDR so the next fastboot can
 * read it (headless bring-up observability). PA = symbol VA - virt_base +
 * phys_base, i.e. 0xA0000000 + (VA & 0x1fffffff). */
#define SWEET_EXEC_RING_SIZE 1024
char sweet_exec_ring[SWEET_EXEC_RING_SIZE];
static unsigned int sweet_exec_ring_len;
static uint32_t sweet_exec_seq;

void
sweet_note_exec(const char *name)
{
\tchar line[48];
\tunsigned int n = 0, i;
\tuint32_t v = ++sweet_exec_seq;
\tchar d[10];
\tint t = 0;

\tdo {
\t\td[t++] = (char)('0' + (v % 10));
\t\tv /= 10;
\t} while (v);
\twhile (t > 0) {
\t\tline[n++] = d[--t];
\t}
\tline[n++] = ' ';
\tfor (i = 0; name != NULL && name[i] != 0 && i < 23; i++) {
\t\tline[n++] = name[i];
\t}
\tline[n++] = '\\n';
\tif (sweet_exec_ring_len + n > SWEET_EXEC_RING_SIZE) {
\t\tunsigned int drop = sweet_exec_ring_len + n - SWEET_EXEC_RING_SIZE;
\t\tif (drop > sweet_exec_ring_len) {
\t\t\tdrop = sweet_exec_ring_len;
\t\t}
\t\t__builtin_memmove(sweet_exec_ring, sweet_exec_ring + drop,
\t\t    sweet_exec_ring_len - drop);
\t\tsweet_exec_ring_len -= drop;
\t}
\tfor (i = 0; i < n; i++) {
\t\tsweet_exec_ring[sweet_exec_ring_len++] = line[i];
\t}
\tfor (uintptr_t a = (uintptr_t)sweet_exec_ring & ~63UL;
\t    a < (uintptr_t)sweet_exec_ring + SWEET_EXEC_RING_SIZE; a += 64) {
\t\t__asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
\t}
\t__asm__ volatile("dsb sy" ::: "memory");
}

'''
s = s.replace(anchor, ring + anchor)
open(p, 'w').write(s)

k = xu + '/bsd/kern/kern_exec.c'
t = open(k).read()
old = '\tp->p_comm[imgp->ip_ndp->ni_cnd.cn_namelen] = \'\\0\';\n'
assert t.count(old) == 1, t.count(old)
new = old + '''
#if defined(SWEET)
\t{
\t\textern void sweet_note_exec(const char *);
\t\tsweet_note_exec(p->p_comm);
\t}
#endif
'''
t = t.replace(old, new)
open(k, 'w').write(t)
print('v151 patch applied')
