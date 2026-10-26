#!/usr/bin/env python3
"""v158: swipe direction follows the transform.

xf>=4 (transposed presentation): the UI vertical runs along panel x, so
swipe along x. xf<4 (the camera-corrected default): swipe along panel y
from the bottom (y=1000) to the top (y=300).
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/hid/SweetHID.cpp'
s = open(p).read()

old = '''	{
		static const int steps[] = { 300, 380, 460, 540, 620, 680 };
		UInt32 n = self->fTapStep;
		if (n < sizeof(steps) / sizeof(steps[0])) {
			self->touch(0, true, steps[n], 640);
			if (n == 0) {
				IOLog("SWEET HID: synthetic swipe start\\n");
			}
			self->fTapStep = n + 1;
			sender->setTimeoutMS(70);
		} else {
			self->touch(0, false, 680, 640);
			IOLog("SWEET HID: synthetic swipe done\\n");
			self->fTapStep = n + 1;
		}
	}'''
assert s.count(old) == 1
new = '''	{
		static const int steps[] = { 300, 380, 460, 540, 620, 680 };
		UInt32 n = self->fTapStep;
		bool tx = self->fXform >= 4;
		int sx = tx ? steps[n < 6 ? n : 5] : 360;
		int sy = tx ? 640 : (n < 6 ? 1000 - (n * 140) : 300);
		int ex = tx ? 680 : 360;
		int ey = tx ? 640 : 300;
		if (n < sizeof(steps) / sizeof(steps[0])) {
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
		}
	}'''
s = s.replace(old, new)
open(p, 'w').write(s)
print('v158 patched')
