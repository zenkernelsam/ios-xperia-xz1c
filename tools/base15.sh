#!/bin/bash
# rebuild the post-0014 baseline, restore the edited files, emit new 0015 diff
cd /root/xnu-verify/xnu-xnu-8796.141.3 || exit 1
git reset -q --hard staged
for p in /root/xz1c/xnu/patches/*.patch; do
  n=$(basename "$p")
  case "$n" in
    0015-lilac-imem*) break;;
  esac
  git apply "$p" || echo "FAIL $n"
done
git add -A
git -c user.email=v@v -c user.name=v commit -qm base15
git tag -f base15
cp /tmp/ns/sweet_early.c osfmk/arm64/sweet_early.c
cp /tmp/ns/start.s osfmk/arm64/start.s
cp /tmp/ns/arm_vm_init.c osfmk/arm64/arm_vm_init.c
git diff base15 -- osfmk/arm64/sweet_early.c osfmk/arm64/start.s osfmk/arm64/arm_vm_init.c > /tmp/0015.new.patch
wc -l /tmp/0015.new.patch
grep -c '^@@' /tmp/0015.new.patch
