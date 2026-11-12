#!/usr/bin/env python3
"""v156: sweettap=<seconds> — synthetic tap on the built-in touch screen.

Reuses SweetHIDTouch::touch() (digitizer collection with finger children).
state machine: down at T, up 90 ms later.
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/hid/SweetHID.cpp'
s = open(p).read()

old_members = '''	IOFixed fX[SWEET_TOUCH_FINGERS];
	IOFixed fY[SWEET_TOUCH_FINGERS];
};'''
assert s.count(old_members) == 1
new_members = '''	IOFixed fX[SWEET_TOUCH_FINGERS];
	IOFixed fY[SWEET_TOUCH_FINGERS];
private:
	static void tapFired(OSObject *owner, IOTimerEventSource *sender);
	IOTimerEventSource *fTap;
	UInt32 fTapStep;
	int fTapX, fTapY;
};'''
s = s.replace(old_members, new_members)

old_start = '''	gSweetTouch = this;
	registerService();
	IOLog("SWEET HID: touch registered\\n");
	return true;
}'''
assert s.count(old_start) == 1
new_start = '''	gSweetTouch = this;
	UInt32 tapSecs = 0;
	PE_parse_boot_argn("sweettap", &tapSecs, sizeof(tapSecs));
	if (tapSecs) {
		fTapStep = 0;
		fTapX = 360;
		fTapY = 800;
		PE_parse_boot_argn("sweettapx", &fTapX, sizeof(fTapX));
		PE_parse_boot_argn("sweettapy", &fTapY, sizeof(fTapY));
		fTap = IOTimerEventSource::timerEventSource(this, &SweetHIDTouch::tapFired);
		if (fTap && _hidWorkLoop->addEventSource(fTap) == kIOReturnSuccess) {
			fTap->setTimeoutMS(tapSecs * 1000);
		}
	}
	registerService();
	IOLog("SWEET HID: touch registered (tap in %u s at %d,%d)\\n", tapSecs, fTapX, fTapY);
	return true;
}

void
SweetHIDTouch::tapFired(OSObject *owner, IOTimerEventSource *sender)
{
	SweetHIDTouch *self = (SweetHIDTouch *)owner;

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
}'''
s = s.replace(old_start, new_start)
open(p, 'w').write(s)
print('v156 sweettap patched')
