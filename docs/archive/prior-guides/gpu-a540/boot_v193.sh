#!/bin/bash
# One-shot after the phone is powered back into fastboot:
# patch xperable (once), boot the v193 A540/secure-CB image, capture frames.
set -u
cd "$(dirname "$0")/.." || exit 1
IMG="${1:-shim-4k-v196-pre-zap-gzdtb.img}"
TMP="${TEMP:-/c/Users/Cisco He/AppData/Local/Temp}"
export MSYS_NO_PATHCONV=1

if ! fastboot devices 2>/dev/null | grep -q "BH905ND69G"; then
	echo "phone is not in fastboot; power it on / adb reboot bootloader first"
	exit 1
fi

echo "== xperable patch =="
(cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6) || {
	echo "patch send failed; unplug/replug or retry once, do NOT devreset"
	exit 1
}
sleep 2

echo "== boot $IMG =="
(cd unlock && ./xperable.exe -I "$(pwd)/../$IMG" -1 -c boot -1) || true
sleep 12

echo "== capture =="
for i in 1 2 3 4 5 6 7 8; do
	python3 research/grab_cam.py "$TMP/v196-$i.jpg" 2.5 >/dev/null 2>&1
	sleep 2
done
ls -l "$TMP"/v193-*.jpg
