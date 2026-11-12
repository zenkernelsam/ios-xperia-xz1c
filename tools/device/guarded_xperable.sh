#!/bin/bash
# Safety lock around xperable (which cannot select a device by serial):
# by default refuse unless fastboot lists exactly BH905ND69G and libusb sees exactly
# one 0fce:0dde, whose serial (when readable) is BH905ND69G.
# usage: guarded_xperable.sh <xperable args...>
# Explicit audited mode: guarded_xperable.sh --verified-sony-only <args...>
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
PATCH="$(cd "$HERE/../.." && pwd)"
EXPECT=BH905ND69G

# test1 retries the same command after a response error, including boot.
# Boot handoff must use lilac-loop/boot_once.py, which records one write.
want_command=0
for arg in "$@"; do
    if [ "$want_command" -eq 1 ] && [ "$arg" = "boot" ]; then
        echo "REFUSE: xperable boot has an internal retry; use the Sony boot-once controller" >&2
        exit 14
    fi
    want_command=0
    if [ "$arg" = "-c" ]; then want_command=1; fi
    if [ "$arg" = "-cboot" ]; then
        echo "REFUSE: legacy xperable boot is retired" >&2
        exit 14
    fi
done

if [ "${1-}" = "--verified-sony-only" ]; then
	# Explicit user-authorized mode for an unrelated Google fastboot device.
	# The audited executable opens ONLY Sony 0fce:0dde; require readable identity.
	shift
	health=$(fastboot -s "$EXPECT" getvar version-bootloader 2>&1) || exit 10
	case "$health" in
	*version-bootloader:*) ;;
	*) echo "REFUSE: target Sony fastboot endpoint is unhealthy" >&2; exit 10 ;;
	esac
	guard_py="$HERE/sony_usb_guard.py"
	guard_binary="$PATCH/assets/device-tools/xperable.exe"
	if command -v cygpath >/dev/null 2>&1; then
		guard_py=$(cygpath -w "$guard_py")
		guard_binary=$(cygpath -w "$guard_binary")
	fi
	sn=$(python "$guard_py" --binary "$guard_binary") || exit 11
	if [ "$sn" != "$EXPECT" ]; then
		echo "REFUSE: audited Sony USB identity=$sn" >&2
		exit 12
	fi
else
lines=$(fastboot devices 2>/dev/null | awk 'NF {print $1}')
nfb=$(printf '%s\n' "$lines" | grep -c . || true)
if [ "$nfb" -ne 1 ] || [ "$lines" != "$EXPECT" ]; then
	echo "REFUSE: fastboot set=[$lines] (need exactly $EXPECT)" >&2
	exit 10
fi

sn=$(python - "$EXPECT" <<'PY'
import sys
import libusb_package, usb.core, usb.util, usb.backend.libusb1 as L
expect = sys.argv[1]
b = L.get_backend(find_library=libusb_package.find_library)
devs = [d for d in usb.core.find(find_all=True, backend=b)
        if d.idVendor == 0x0fce and d.idProduct == 0x0dde]
if len(devs) != 1:
    print('COUNT=%d' % len(devs))
    sys.exit(0)
try:
    print(usb.util.get_string(devs[0], devs[0].iSerialNumber))
except Exception:
    print('UNREADABLE')
PY
)
case "$sn" in
COUNT=*)
	echo "REFUSE: libusb sees $sn fastboot devices" >&2
	exit 11
	;;
UNREADABLE)
	# one device only, fastboot already confirmed the serial
	;;
"$EXPECT")
	;;
*)
	echo "REFUSE: libusb serial=$sn" >&2
	exit 12
	;;
esac
fi

cd "$PATCH/assets/device-tools" || exit 13
exec ./xperable.exe "$@"
