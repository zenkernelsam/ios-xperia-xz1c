# Handover: xnu build on the Hackintosh (macOS 15)

Goal: replace the 25-min GitHub Actions loop with local incremental builds
(~3-5 min per patch iteration). Windows side stays the flash/test station —
kernels built here get scp'd over for `mkpayload` + `fastboot boot`.

## One-time setup

```bash
# 1) Xcode 15.4 — REQUIRED, Xcode 16+/26 iig breaks xnu-8796 IORPC ABI.
#    developer.apple.com/download/all → Xcode_15.4.xip → unxip to /Applications
sudo xcode-select -s /Applications/Xcode_15.4.app/Contents/Developer
xcodebuild -version          # must say 15.4
xcodebuild -license accept

# 2) deps
brew install jq gum bash cmake ninja
printf '#!/bin/sh\nexit 0\n' | sudo tee /usr/local/bin/xcodes && sudo chmod +x /usr/local/bin/xcodes

# 3) repo + pinned build script
git clone https://github.com/zenkernelsam/ios-xperia-xz1c.git
cd ios-xperia-xz1c
git init -q dxb
git -C dxb fetch -q --depth 1 https://github.com/blacktop/darwin-xnu-build \
    663906528d1cd5ea8fde7de6f16de73db9511549
git -C dxb checkout -q FETCH_HEAD
cp xnu/patches/*.patch xnu/patches/*.sh dxb/patches/
sed -i '' 's/ARCHS="x86_64 arm64e" VALID_ARCHS="x86_64 arm64e"/ARCHS="x86_64 arm64e arm64" VALID_ARCHS="x86_64 arm64e arm64"/' dxb/build.sh

# 4) build (~20 min first time: KDK 13.5 auto-downloads + full tree)
cd dxb
MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT MACHINE_CONFIG=SWEET bash build.sh
```

Success = `fakeroot/System/Library/Kernels/kernel.development.sweet`.

Sanity check:
```bash
file fakeroot/System/Library/Kernels/kernel.development.sweet
# → Mach-O 64-bit arm64 executable  (cpusub must be plain arm64, not arm64e)
```

## Iteration loop (the whole point)

```bash
cd ios-xperia-xz1c
git pull                              # new patches from Windows side
cp xnu/patches/*.patch xnu/patches/*.sh dxb/patches/
cd dxb && MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT MACHINE_CONFIG=SWEET bash build.sh
```
Incremental relink/compile ~minutes.

## Get the kernel back to Windows

On the Mac, enable Remote Login (System Settings → Sharing → Remote Login),
then from Windows Git Bash:

```bash
scp macuser@<mac-ip>:~/ios-xperia-xz1c/dxb/fakeroot/System/Library/Kernels/kernel.development.sweet \
    "C:/Users/Cisco He/Desktop/Git/ios-xperia-xz1c/xnu/out-mac/kernel.development.sweet"
```

Then Windows side runs the existing chain:
`tools/mkpayload.py <kernel> artifacts/lilac.adt shim/out/payload.bin --cmdline "..."`
→ `shim/build.sh` → `fastboot boot`.

## Gotchas

- macOS checkout: `.gitattributes` forces `*.sh`/`*.patch` to LF — the CRLF
  disaster that killed CI run #1 cannot recur on Mac.
- Do NOT let Xcode auto-update to 16+; keep 15.4 selected.
- `KEEP_GOING`: `MAKEFLAGS=-k` collects all compile errors in one run —
  use it when a patch is new; drop it for speed otherwise.
- The `xnu/out-*/` and `artifacts/` dirs are git-ignored; kernels never
  go into git.
- GH PAT for CI/artifacts is in `.github-token.env` on the Windows clone;
  copying it to the Mac enables `gh run watch`/`gh run download` there too.

## Current state (handover written 2026-10-22)

- CI run `39631647523` building the RR-telemetry kernel (commit 7622103).
  If it finished before your local build, just download artifact
  `xnu-kernel` from that run instead of building.
- First XNU boot already attempted: jumped, hung, WDT bite → fastboot.
  The telemetry patches (0010/0015) report progress via
  `warmboot=0x7766xxxx` in the next Android cmdline.
