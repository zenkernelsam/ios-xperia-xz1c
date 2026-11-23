#!/bin/bash
# One-shot after the phone is powered back into fastboot:
# patch xperable (require both "patched successfully"), boot the GPU image,
# capture frames via the single-frame camera endpoint.
# Camera rule: never use grab_cam.py; /override only to revive a stuck camera.
set -u
cd "$(dirname "$0")/.." || exit 1
IMG="${1:-shim-4k-v198-nosec-gzdtb.img}"
TMP="${TEMP:-/c/Users/Cisco He/AppData/Local/Temp}"
CAM="http://192.168.68.57:4747/cam/1/frame.jpg"
TAG="${2:-v198}"
export MSYS_NO_PATHCONV=1

if ! fastboot devices 2>/dev/null | grep -q "BH905ND69G"; then
	echo "phone is not in fastboot; power it on / adb reboot bootloader first"
	exit 1
fi

echo "== xperable patch =="
PATCH_OUT=$(cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6 2>&1)
echo "$PATCH_OUT" | tail -6
if [ "$(echo "$PATCH_OUT" | grep -c 'patched successfully')" -lt 2 ]; then
	echo "patch NOT confirmed; not booting (fastboot reboot bootloader, then retry)"
	exit 1
fi
sleep 2

echo "== boot $IMG =="
(cd unlock && ./xperable.exe -I "$(pwd)/../$IMG" -1 -c boot -1) || true
sleep 26

echo "== capture =="
for i in 1 2 3 4 5 6 7 8; do
	curl -s -m 10 -o "$TMP/$TAG-$i.jpg" "$CAM"
	sleep 6
done
ls -l "$TMP/$TAG"-*.jpg
