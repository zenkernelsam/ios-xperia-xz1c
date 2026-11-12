#!/usr/bin/env python3
"""v141 bring-up switches:
- kern_xxx.c reboot(): with sweetnoreboot=1, log and return 0 instead of
  resetting, so launchd's sysstatuscheck userspace reboot does not kick us
  back to fastboot.
- sleh.c: sweetnoar=1 disables the 90 s auto-return timer entirely.
"""
XNU = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
P_REBOOT = XNU + '/bsd/kern/kern_xxx.c'
P_SLEH = XNU + '/osfmk/arm64/sleh.c'

r = open(P_REBOOT).read()
anchor = '''	AUDIT_ARG(cmd, uap->opt);

	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
'''
add = '''	AUDIT_ARG(cmd, uap->opt);

#if defined(ARM64_BOARD_CONFIG_SWEET)
	{
		extern int PE_parse_boot_argn(const char *arg_string, void *arg_ptr,
		    unsigned int max_len);
		uint32_t sweet_noreboot = 0;
		if (PE_parse_boot_argn("sweetnoreboot", &sweet_noreboot,
		    sizeof(sweet_noreboot)) && sweet_noreboot) {
			printf("SWEET: userspace reboot(0x%x) suppressed\\n", uap->opt);
			return 0;
		}
	}
#endif

	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
'''
assert r.count(anchor) == 1, 'reboot anchor'
open(P_REBOOT, 'w').write(r.replace(anchor, add, 1))
print('kern_xxx.c: sweetnoreboot switch added')

s = open(P_SLEH).read()
anchor2 = '''		{
			extern void nanoseconds_to_absolutetime(uint64_t, uint64_t *);
			extern void sweet_reset_to_fastboot(void);
			if (sweet_ar_deadline == 0) {
'''
add2 = '''		{
			extern void nanoseconds_to_absolutetime(uint64_t, uint64_t *);
			extern void sweet_reset_to_fastboot(void);
			extern int PE_parse_boot_argn(const char *arg_string, void *arg_ptr,
			    unsigned int max_len);
			static uint32_t sweet_noar = 0, sweet_noar_checked = 0;
			if (!sweet_noar_checked) {
				uint32_t v = 0;
				PE_parse_boot_argn("sweetnoar", &v, sizeof(v));
				sweet_noar = v;
				sweet_noar_checked = 1;
			}
			if (sweet_noar) {
				goto sweet_ar_done;
			}
			if (sweet_ar_deadline == 0) {
'''
assert s.count(anchor2) == 1, 'sleh anchor'
s = s.replace(anchor2, add2, 1)
# close the label after the existing block: find the end of that block
old_end = '''			} else if (mach_absolute_time() >= sweet_ar_deadline) {
				sweet_reset_to_fastboot();
			}
		}
'''
new_end = '''			} else if (mach_absolute_time() >= sweet_ar_deadline) {
				sweet_reset_to_fastboot();
			}
sweet_ar_done:;
		}
'''
assert s.count(old_end) == 1, 'sleh end anchor'
s = s.replace(old_end, new_end, 1)
open(P_SLEH, 'w').write(s)
print('sleh.c: sweetnoar switch added')
