#!/usr/bin/env bash
set -euo pipefail
project=$(cd "$(dirname "$0")/../.." && pwd)
output="$project/build/geocompat"
mkdir -p "$output"
sdk=$(xcrun --sdk iphoneos --show-sdk-path)
xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
  -O1 -fblocks -fno-objc-arc -dynamiclib "$project/gpu/geocompat/geocompat.mm" \
  -framework CoreFoundation -framework CoreGraphics -lobjc \
  -Wl,-install_name,/dev/sglram/georead.dylib -o "$output/georead.dylib"
codesign --force --sign - "$output/georead.dylib"
codesign --verify "$output/georead.dylib"
