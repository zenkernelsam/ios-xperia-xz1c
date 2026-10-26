#!/usr/bin/env python3
"""v157: invert the sweetness transform for touch + synthetic swipe.

- SweetHIDTouch reads sweetfb_xf and, for xf>=4 (transpose family), maps
  panel pixels -> scene (u,v) with the inverse of the framebuffer transform.
- sweettap / sweetswipe=<seconds>: inject a tap or an upward swipe after N s
  (panel pixel space; the mapping above turns it into the right scene coords).
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/hid/SweetHID.cpp'
s = open(p).read()

# members
old = '''	static void tapFired(OSObject *owner, IOTimerEventSource *sender);
	IOTimerEventSource *fTap;
	UInt32 fTapStep;
	int fTapX, fTapY;
};'''
assert s.count(old) == 1
new = '''	static void tapFired(OSObject *owner, IOTimerEventSource *sender);
	IOTimerEventSource *fTap;
	UInt32 fTapStep;
	int fTapX, fTapY;
	int fXform;
	bool fSwipe;
};'''
s = s.replace(old, new)

# read xf in start (after fOffX/fOffY)
old2 = '''	fOffX = (fLogicalW - 720) / 2;
	fOffY = (fLogicalH - 1280) / 2;'''
assert s.count(old2) == 1
s = s.replace(old2, old2 + '''
	fXform = 0;
	PE_parse_boot_argn("sweetfb_xf", &fXform, sizeof(fXform));''')

# tap setup: add swipe flag
old3 = '''	UInt32 tapSecs = 0;
	PE_parse_boot_argn("sweettap", &tapSecs, sizeof(tapSecs));
	if (tapSecs) {
		fTapStep = 0;
		fTapX = 360;
		fTapY = 800;
		PE_parse_boot_argn("sweettapx", &fTapX, sizeof(fTapX));
		PE_parse_boot_argn("sweettapy", &fTapY, sizeof(fTapY));'''
assert s.count(old3) == 1
new3 = '''	UInt32 tapSecs = 0;
	fSwipe = false;
	if (!PE_parse_boot_argn("sweettap", &tapSecs, sizeof(tapSecs))) {
		if (PE_parse_boot_argn("sweetswipe", &tapSecs, sizeof(tapSecs))) {
			fSwipe = true;
		}
	}
	if (tapSecs) {
		fTapStep = 0;
		fTapX = 360;
		fTapY = 800;
		PE_parse_boot_argn("sweettapx", &fTapX, sizeof(fTapX));
		PE_parse_boot_argn("sweettapy", &fTapY, sizeof(fTapY));'''
s = s.replace(old3, new3)

# tapFired: swipe state machine (down at tapXY, moves toward the UI top, up)
old4 = '''	if (self->fTapStep == 0) {
		IOLog("SWEET HID: synthetic tap down %d,%d\\n", self->fTapX, self->fTapY);
		self->touch(0, true, self->fTapX, self->fTapY);
		self->fTapStep = 1;
		sender->setTimeoutMS(90);
	} else {
		IOLog("SWEET HID: synthetic tap up\\n");
		self->touch(0, false, self->fTapX, self->fTapY);
		self->fTapStep = 2;
	}'''
assert s.count(old4) == 1
new4 = '''	if (!self->fSwipe) {
		if (self->fTapStep == 0) {
			IOLog("SWEET HID: synthetic tap down %d,%d\\n", self->fTapX, self->fTapY);
			self->touch(0, true, self->fTapX, self->fTapY);
			self->fTapStep = 1;
			sender->setTimeoutMS(90);
		} else {
			IOLog("SWEET HID: synthetic tap up\\n");
			self->touch(0, false, self->fTapX, self->fTapY);
			self->fTapStep = 2;
		}
		return;
	}
	/* Swipe: panel x follows the UI's vertical axis under xf>=4; the
	 * hint is "swipe up to open", i.e. increasing panel x. */
	{
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
s = s.replace(old4, new4)

# touch(): invert transform for xf>=4
old5 = '''	/* Panel pixels -> the presented 1125x2436 display (SweetFramebuffer.cpp). */
	IOFixed nx = (IOFixed)(((int64_t)(x + fOffX) << 16) / fLogicalW);
	IOFixed ny = (IOFixed)(((int64_t)(y + fOffY) << 16) / fLogicalH);'''
assert s.count(old5) == 1
new5 = '''	/* Panel pixels -> the presented 1125x2436 display (SweetFramebuffer.cpp).
	 * With sweetfb_xf>=4 the panel image is a transposed view of the scene,
	 * so invert that mapping before normalizing. */
	IOFixed nx, ny;
	if (fXform >= 4) {
		int flipx = (fXform == 5 || fXform == 7);
		int flipy = (fXform == 6 || fXform == 7);
		int64_t u = ((int64_t)y * fLogicalW) / 1280;
		int64_t v = ((int64_t)x * fLogicalH) / 720;
		if (flipx) {
			u = fLogicalW - 1 - u;
		}
		if (flipy) {
			v = fLogicalH - 1 - v;
		}
		nx = (IOFixed)(u << 16) / fLogicalW;
		ny = (IOFixed)(v << 16) / fLogicalH;
	} else {
		nx = (IOFixed)(((int64_t)(x + fOffX) << 16) / fLogicalW);
		ny = (IOFixed)(((int64_t)(y + fOffY) << 16) / fLogicalH);
	}'''
s = s.replace(old5, new5)
open(p, 'w').write(s)
print('v157 patched')
