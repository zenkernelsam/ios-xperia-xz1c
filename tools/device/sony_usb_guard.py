"""Verified Sony-only selection when unrelated fastboot devices remain attached."""
import argparse
import hashlib
from pathlib import Path

SERIAL = 'BH905ND69G'
VID, PID = 0x0fce, 0x0dde
XPERABLE_SHA256 = 'f9fd2196f0bebd4c53077ddfb3db765eb1cf22409c311ca7c502b04b0c150a6d'

def target_fastboot_lines(text):
    rows = [line.split() for line in text.splitlines() if line.strip()]
    target = [row for row in rows if row[0] == SERIAL]
    if len(target) > 1 or (target and (len(target[0]) != 2 or target[0][1] != 'fastboot')):
        raise RuntimeError('Ambiguous or invalid Sony fastboot entry')
    return '\n'.join('\t'.join(row) for row in target)

def validate_binary(path):
    if hashlib.sha256(Path(path).read_bytes()).hexdigest() != XPERABLE_SHA256:
        raise RuntimeError('xperable binary differs from the audited Sony VID/PID selector')

def validate_sony_records(records):
    if len(records) != 1:
        raise RuntimeError('Need exactly one Sony 0fce:0dde USB device')
    row = records[0]
    if (row.get('vid'), row.get('pid'), row.get('serial')) != (VID, PID, SERIAL):
        raise RuntimeError('Sony USB identity is unreadable or does not match BH905ND69G')
    return SERIAL

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', required=True)
    args = ap.parse_args()
    validate_binary(args.binary)
    import libusb_package
    import usb.core
    import usb.util
    import usb.backend.libusb1 as L
    backend = L.get_backend(find_library=libusb_package.find_library)
    devices = list(usb.core.find(find_all=True, idVendor=VID, idProduct=PID, backend=backend))
    records = []
    for device in devices:
        try:
            serial = usb.util.get_string(device, device.iSerialNumber)
        except Exception:
            serial = None
        records.append({'vid':device.idVendor, 'pid':device.idProduct, 'serial':serial})
    print(validate_sony_records(records))

if __name__ == '__main__':
    main()
