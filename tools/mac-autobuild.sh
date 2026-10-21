#!/bin/bash
# mac-autobuild.sh — Mac-side xnu build slave.
# Polls origin/main every XNU_POLL seconds. On a new commit: pull, refresh
# patches in dxb/, rebuild kernel.development.sweet, drop the result into the
# Fn-Sync shared XNU folder so the Windows side can pick it up.
# Install as a launchd agent via tools/mac-autobuild-install.sh, or run
# directly:  bash tools/mac-autobuild.sh
set -u

REPO="${XNU_REPO:-$HOME/Desktop/ios-xperia-xz1c}"
SYNC_DIR="${XNU_SYNC_DIR:-$HOME/Library/CloudStorage/飞牛同步-Home-Nas/XNU}"
LOG="${XNU_BUILD_LOG:-$HOME/xnu-autobuild.log}"
POLL="${XNU_POLL:-60}"
KERNEL_REL="dxb/fakeroot/System/Library/Kernels/kernel.development.sweet"

export MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT MACHINE_CONFIG=SWEET
export PATH="/usr/local/bin:/opt/homebrew/bin:$PATH"

log() { printf '%s %s\n' "$(date '+%m-%d %H:%M:%S')" "$*" >>"$LOG"; }

status() { printf '%s | %s | %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$1" "$2" >"$SYNC_DIR/BUILD_STATUS.txt" 2>/dev/null; }

mkdir -p "$SYNC_DIR"
cd "$REPO" || { log "FATAL repo missing: $REPO"; exit 1; }
log "=== autobuild start (poll ${POLL}s, repo $REPO) ==="

while :; do
	# Environment gate: only build under Xcode 15.x (16+/26 iig breaks xnu-8796)
	XCV=$(xcodebuild -version 2>/dev/null | head -1)
	case "$XCV" in
		"Xcode 15."*) ;;
		*) log "skip: '$XCV' is not Xcode 15.x — select Xcode 15.4 first"; status "idle" "waiting for Xcode 15.x"; sleep "$POLL"; continue ;;
	esac

	git fetch -q origin main >>"$LOG" 2>&1
	REMOTE=$(git rev-parse origin/main 2>/dev/null) || REMOTE=""
	LOCAL=$(git rev-parse HEAD)
	[ -z "$REMOTE" ] && { sleep "$POLL"; continue; }

	if [ "$REMOTE" != "$LOCAL" ]; then
		SHORT=${REMOTE:0:8}
		log "new commit $SHORT — rebuilding"
		status "building" "$SHORT"

		if ! git pull --ff-only -q origin main >>"$LOG" 2>&1; then
			log "pull --ff-only failed (dirty tree?) — skipping this round"
			status "skip" "dirty tree"
			sleep "$POLL"; continue
		fi

		cp xnu/patches/*.patch xnu/patches/*.sh dxb/patches/ 2>>"$LOG"
		# idempotent: ensure plain arm64 in the arch lists
		sed -i '' 's/ARCHS="x86_64 arm64e" VALID_ARCHS="x86_64 arm64e"/ARCHS="x86_64 arm64e arm64" VALID_ARCHS="x86_64 arm64e arm64"/' dxb/build.sh 2>/dev/null

		(cd dxb && bash build.sh) >>"$LOG" 2>&1

		if [ -f "$KERNEL_REL" ]; then
			OUT="$SYNC_DIR/kernel-${SHORT}-$(date '+%m%d-%H%M').sweet"
			cp "$KERNEL_REL" "$OUT" && log "OK -> $OUT" && status "ok" "$SHORT $(basename "$OUT")"
		else
			log "BUILD FAILED ($SHORT) — tail: $(tail -5 "$LOG" | tr '\n' '~')"
			status "fail" "$SHORT"
		fi
	fi
	sleep "$POLL"
done
