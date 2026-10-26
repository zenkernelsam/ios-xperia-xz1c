#!/usr/bin/env python3
"""v142 diagnostics + suppression at reboot_kernel level:
- SweetDDR mark helper writes ('RBT1', howto) / ('ARST', 0) at cell+0x1A0.
- reboot_kernel(): with sweetnoreboot=1, mark and return without resetting.
- sweet_reset_to_fastboot(): mark ARST before PSCI reset.
"""
XNU = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
P_EARLY = XNU + '/osfmk/arm64/sweet_early.c'
P_SHUT = XNU + '/bsd/kern/kern_shutdown.c'

e = open(P_EARLY).read()
anchor = '''void
sweet_reset_to_fastboot(void)
{
'''
assert e.count(anchor) == 1, 'reset_to_fastboot anchor'
add = '''void
sweet_ddr_mark(uint64_t tag, uint64_t v)
{
\textern volatile char sweet_panic_cell[];
\tvolatile uint64_t *c = (volatile uint64_t *)(void *)sweet_panic_cell;
\tuintptr_t line = ((uintptr_t)c + 0x1A0) & ~(uintptr_t)63;

\tc[0x34] = tag;
\tc[0x35] = v;
\t__asm__ volatile("dc civac, %0" :: "r"(line) : "memory");
\t__asm__ volatile("dsb sy" ::: "memory");
}

void
sweet_reset_to_fastboot(void)
{
\tsweet_ddr_mark(0x54535241ULL, 0);\t/* 'ARST' */
'''
e = e.replace(anchor, add, 1)
open(P_EARLY, 'w').write(e)
print('sweet_early.c: sweet_ddr_mark + ARST')

s = open(P_SHUT).read()
anchor2 = '''reboot_kernel(int howto, char *message)
{
'''
assert s.count(anchor2) == 1, 'reboot_kernel anchor'
add2 = '''reboot_kernel(int howto, char *message)
{
#if defined(ARM64_BOARD_CONFIG_SWEET)
\t{
\t\textern void sweet_ddr_mark(uint64_t tag, uint64_t v);
\t\textern boolean_t PE_parse_boot_argn(const char *arg_string,
\t\t    void *arg_ptr, int max_arg);
\t\tuint32_t noreboot = 0;

\t\tsweet_ddr_mark(0x31544252ULL, (uint64_t)(unsigned)howto);\t/* 'RBT1' */
\t\tif (PE_parse_boot_argn("sweetnoreboot", &noreboot, sizeof(noreboot)) &&
\t\t    noreboot) {
\t\t\tprintf("SWEET: reboot_kernel(0x%x) suppressed\\n", howto);
\t\t\treturn;
\t\t}
\t}
#endif
'''
s = s.replace(anchor2, add2, 1)
open(P_SHUT, 'w').write(s)
print('kern_shutdown.c: reboot_kernel suppressed with sweetnoreboot')
