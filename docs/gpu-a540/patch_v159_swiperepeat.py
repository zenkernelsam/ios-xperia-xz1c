#!/usr/bin/env python3
"""v159: repeat synthetic swipes — sweetswipecount=N (default 1), 30 s apart.

With the hello animation taking minutes on the CPU renderer, one swipe at a
fixed time misses it. Repeating lets the wizard advance whenever it is ready.
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/hid/SweetHID.cpp'
s = open(p).read()

old = '''	int fXform;
	bool fSwipe;
};'''
assert s.count(old) == 1
s = s.replace(old, '''	int fXform;
	bool fSwipe;
	UInt32 fSwipeRepeat, fSwipeCount;
};''')

old2 = '''	if (tapSecs) {
		fTapStep = 0;'''
assert s.count(old2) == 1
s = s.replace(old2, '''	fSwipeRepeat = 1;
	PE_parse_boot_argn("sweetswipecount", &fSwipeRepeat, sizeof(fSwipeRepeat));
	if (fSwipeRepeat < 1) {
		fSwipeRepeat = 1;
	}
	if (tapSecs) {
		fTapStep = 0;''')

old3 = '''		if (n < sizeof(steps) / sizeof(steps[0])) {
			self->touch(0, true, sx, sy);
			if (n == 0) {
				IOLog("SWEET HID: synthetic swipe start (%d,%d)\\n", sx, sy);
			}
			self->fTapStep = n + 1;
			sender->setTimeoutMS(70);
		} else {
			self->touch(0, false, ex, ey);
			IOLog("SWEET HID: synthetic swipe done\\n");
			self->fTapStep = n + 1;
		}'''
assert s.count(old3) == 1
new3 = '''		if (n == 7) {
			/* idle between repeats */
			if (self->fSwipeCount < self->fSwipeRepeat) {
				self->fTapStep = 0;
				sender->setTimeoutMS(30000);
			}
			return;
		}
		if (n < sizeof(steps) / sizeof(steps[0])) {
			self->touch(0, true, sx, sy);
			if (n == 0) {
				IOLog("SWEET HID: synthetic swipe start %u/%u (%d,%d)\\n",
				    self->fSwipeCount + 1, self->fSwipeRepeat, sx, sy);
			}
			self->fTapStep = n + 1;
			sender->setTimeoutMS(70);
		} else {
			self->touch(0, false, ex, ey);
			IOLog("SWEET HID: synthetic swipe done\\n");
			self->fTapStep = 7;
			self->fSwipeCount++;
			if (self->fSwipeCount < self->fSwipeRepeat) {
				sender->setTimeoutMS(30000);
			}
		}'''
s = s.replace(old3, new3)
open(p, 'w').write(s)
print('v159 patched')
