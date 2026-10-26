#!/usr/bin/env python3
# v181: verify the PON scratch write path and print step write return codes.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'scratch write test' in s:
    print('v181 already applied')
    raise SystemExit(0)

# step helper: print rc
old = '''static void
sweet_gpu_step(uint8_t code)
{
	(void)sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, code);
}
'''
new = '''static void
sweet_gpu_step(uint8_t code)
{
	int rc = sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, code);
	printf("SWEET GPU: step %#x rc=%d\\n", code, rc);
}
'''
assert s.count(old) == 1
s = s.replace(old, new)

# early self-test after the last-step read/hold
old = '''	{
		uint8_t last = 0;
		if (sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &last) == 0) {
			printf("SWEET GPU: last step=%#x\\n", last);
			delay(15 * 1000000);
		}
	}
'''
new = '''	{
		uint8_t last = 0;
		if (sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &last) == 0) {
			printf("SWEET GPU: last step=%#x\\n", last);
		}
		/* scratch write test */
		int wrc = sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, 0xa5);
		uint8_t rb = 0;
		int rrc = sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &rb);
		printf("SWEET GPU: scratch write test rc=%d read=%d val=%#x\\n",
		    wrc, rrc, rb);
		delay(15 * 1000000);
	}
'''
assert s.count(old) == 1
s = s.replace(old, new)

open(p, 'w').write(s)
print('v181 applied')
