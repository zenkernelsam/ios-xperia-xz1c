#!/bin/bash
# macOS port of ios-sweet mkiosroot.sh — build iOS root HFS+ for lilac userdata
# usage: mkiosroot-mac.sh OS.dmg SYSOS.dmg APPOS.dmg OUT.img [SIZE]
set -euo pipefail
os=$(realpath "$1"); sysos=$(realpath "$2"); appos=$(realpath "$3")
out=$(realpath "$4"); size=${5:-16g}
[[ ! -e $out ]] || { echo "refusing to overwrite $out"; exit 1; }
work=$(mktemp -d); mounts=(); devs=()
cleanup(){
  for m in "${mounts[@]:-}"; do sudo umount "$m" 2>/dev/null||true; done
  for d in "${devs[@]:-}"; do hdiutil detach "$d" -force 2>/dev/null||true; done
  rm -rf "$work"
}
trap cleanup EXIT

apfs_mount(){ # raw APFS dmg -> mount at $2
  mkdir "$work/$2"
  dev=$(hdiutil attach -imagekey diskimages-class=CRawDiskImage -nomount -readonly "$1" | head -1 | awk '{print $1}')
  devs+=("$dev")
  slice=$(diskutil list "$dev" | awk '/APFS Volume/{print $NF}' | head -1)
  [[ -n $slice ]] || slice="${dev}s1"
  sudo mount_apfs -o ro "/dev/$slice" "$work/$2" 2>/dev/null || sudo mount -t apfs -o ro "/dev/$slice" "$work/$2"
  mounts+=("$work/$2")
  echo "mounted $1 -> $work/$2 ($slice)"
}

# UDIF OS dmg attaches directly
mkdir "$work/os"
dev=$(hdiutil attach -nomount -readonly "$os" | head -1 | awk '{print $1}')
devs+=("$dev")
slice=$(diskutil list "$dev" | awk '/APFS Volume/{print $NF}' | head -1)
[[ -n $slice ]] || slice="${dev}s1"
sudo mount_apfs -o ro "/dev/$slice" "$work/os" && mounts+=("$work/os") || { echo "OS mount failed"; exit 1; }
echo "OS -> $work/os ($slice)"
apfs_mount "$sysos" sysos
apfs_mount "$appos" appos

# case-sensitive journaled HFS+ image
hdiutil create -size "$size" -fs "Case-sensitive Journaled HFS+" -volname iOSRoot -type SPARSE -o "$out"
mv "$out.sparseimage" "$out" 2>/dev/null || true
hdev=$(hdiutil attach -nomount "$out" | head -1 | awk '{print $1}')
devs+=("$hdev")
mkdir "$work/root"
sudo mount -t hfs "/dev/${hdev}s2" "$work/root" 2>/dev/null || sudo mount -t hfs "/dev/$hdev" "$work/root"
mounts=("$work/root" "${mounts[@]}")

copy(){ echo "copy $1 -> ${2#$work/root}"; sudo mkdir -p "$2"; sudo rsync -aH "$1/" "$2/"; }
copy "$work/os" "$work/root"
copy "$work/sysos" "$work/root/private/preboot/Cryptexes/OS"
copy "$work/appos" "$work/root/private/preboot/Cryptexes/App"

dsc=private/preboot/Cryptexes/OS/System/Library/Caches/com.apple.dyld
sudo mkdir -p "$work/root/System/Library/Caches/com.apple.dyld"
for f in "$work/root/$dsc"/*; do
  sudo ln "$f" "$work/root/System/Library/Caches/com.apple.dyld/${f##*/}" 2>/dev/null||true
done

for c in OS App; do
  (cd "$work/root/private/preboot/Cryptexes/$c" && sudo find . ! -type d ! -path './System/Library/Caches/com.apple.dyld/*') |
  while IFS= read -r f; do
    f=${f#./}
    if ! sudo test -e "$work/root/$f" && ! sudo test -L "$work/root/$f"; then
      sudo mkdir -p "$work/root/$(dirname "$f")"
      src="$work/root/private/preboot/Cryptexes/$c/$f"
      if sudo test -L "$src"; then sudo cp -P "$src" "$work/root/$f"; else sudo ln "$src" "$work/root/$f"; fi
      echo "linked $c:/$f"
    fi
  done
done

db="$work/root/private/var/db"
sudo install -d -o 272 -g 272 -m 0751 "$db/diagnostics"
sudo install -d -o 272 -g 272 -m 0755 "$db/uuidtext" "$db/diagnostics/timesync"
for d in diagnostics uuidtext diagnostics/timesync; do
  sudo install -o 272 -g 272 -m 0644 /dev/null "$db/$d/.migrated"
done
sudo install -d -m 0700 -o 0 -g 0 "$work/root/private/var/keybags"
printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>' \
 '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
 '<plist version="1.0"><dict><key>KeyBagKeys</key><data>U1dFRVQ=</data></dict></plist>' |
 sudo install -m 0600 -o 0 -g 0 /dev/stdin "$work/root/private/var/keybags/systembag.kb"

df -h "$work/root"
sudo umount "$work/root"; mounts=("${mounts[@]:1}")
sudo fsck_hfs -fy "$hdev" || true
hdiutil detach "$hdev" -force; devs=("${devs[@]:1}")
echo "DONE: $out"
