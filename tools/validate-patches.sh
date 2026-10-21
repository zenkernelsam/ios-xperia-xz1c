#!/bin/bash
# validate-patches.sh — apply the full xnu patch series on a clean
# xnu-8796.141.3 tree inside WSL. Usage (from WSL):
#   bash tools/validate-patches.sh /root/xnu-verify
set -u
ROOT="${1:-/root/xnu-verify}"
SRC="$ROOT/xnu-xnu-8796.141.3"
PATCHES="$(cd "$(dirname "$0")/../xnu/patches" && pwd)"

mkdir -p /tmp/stub /tmp/stub-sdk/usr/include /root
printf '#!/bin/sh\necho /tmp/stub-sdk\n' > /tmp/stub/xcrun
chmod +x /tmp/stub/xcrun
: > /tmp/stub-sdk/usr/include/TargetConditionals.h
export PATH="/tmp/stub:$PATH"
# the staging .sh scripts gate on these
export MACHINE_CONFIG=SWEET MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT

if [ ! -d "$SRC/.git" ]; then
	mkdir -p "$ROOT"
	tar xzf /root/xnu-8796.tar.gz -C "$ROOT"
	cd "$SRC" || exit 1
	git init -q .
	git add -A .
	git -c user.email=v@v -c user.name=v commit -qm base
	git tag base
	for s in 0019-stage-libpthread.sh 0026-stage-hfs.sh; do
		bash "$PATCHES/$s" >/dev/null 2>&1
	done
	git add -A .
	git -c user.email=v@v -c user.name=v commit -qm staged
	git tag staged
fi
cd "$SRC"
git reset -q --hard staged && git clean -fdxq

fail=0
for p in "$PATCHES"/*.patch; do
	if git apply --check "$p" >/dev/null 2>&1; then
		git apply "$p"
	else
		echo "FAIL: $(basename "$p")"
		git apply --check "$p" 2>&1 | head -3
		fail=1
	fi
done
echo "== apply-all fail=$fail =="
