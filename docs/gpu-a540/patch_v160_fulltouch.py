#!/usr/bin/env python3
"""v160: full-scene touch mapping + bottom-up swipe.

The panel shows only the centre 720x1280 band of the 1125x2436 scene, so a
gesture that must start near the display bottom (home indicator, hello
"swipe up") cannot be expressed in panel pixels. sweetfulltouch=1 maps the
whole panel onto the whole scene (x/720, y/1280) for synthetic input, and the
swipe starts at 94% height.
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/hid/SweetHID.cpp'
s = open(p).read()

old = '''	int fXform;
	bool fSwipe;
	UInt32 fSwipeRepeat, fSwipeCount;
};'''
assert s.count(old) == 1
s = s.replace(old, '''	int fXform;
	bool fSwipe;
	bool fFullTouch;
	UInt32 fSwipeRepeat, fSwipeCount;
};''')

old2 = '''	fXform = 0;
	PE_parse_boot_argn("sweetfb_xf", &fXform, sizeof(fXform));'''
assert s.count(old2) == 1
s = s.replace(old2, old2 + '''
	fFullTouch = false;
	PE_parse_boot_argn("sweetfulltouch", &fFullTouch, sizeof(fFullTouch));''')

old3 = '''	if (fXform >= 4) {'''
assert s.count(old3) == 1
s = s.replace(old3, '''	if (fFullTouch) {
		nx = (IOFixed)((int64_t)x << 16) / 720;
		ny = (IOFixed)((int64_t)y << 16) / 1280;
		if (ny > 0xffff) {
			ny = 0xffff;
		}
	} else if (fXform >= 4) {''')

old4 = '''		int sx = tx ? steps[n < 6 ? n : 5] : 360;
		int sy = tx ? 640 : (n < 6 ? 1000 - (n * 140) : 300);'''
assert s.count(old4) == 1
s = s.replace(old4, '''		int sx = tx ? steps[n < 6 ? n : 5] : 360;
		int sy = tx ? 640 : (n < 6 ? 1200 - (n * 150) : 450);''')

old5 = '''		int ex = tx ? 680 : 360;
		int ey = tx ? 640 : 300;'''
assert s.count(old5) == 1
s = s.replace(old5, '''		int ex = tx ? 680 : 360;
		int ey = tx ? 640 : 400;''')

open(p, 'w').write(s)
print('v160 patched')
