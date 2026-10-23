#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
CC=aarch64-linux-gnu-gcc
OBJCOPY=aarch64-linux-gnu-objcopy
CFLAGS="-O2 -ffreestanding -nostdlib -fno-pic -mgeneral-regs-only -mstrict-align -fno-stack-protector -Wall -Wextra"
[ "${PING:-0}" = 1 ] && CFLAGS="$CFLAGS -DSHIM_PING"
[ "${PROBE:-0}" = 1 ] && CFLAGS="$CFLAGS -DSHIM_PROBE"
[ "${PURE:-0}" != 0 ] && CFLAGS="$CFLAGS -DSHIM_PURE=$PURE"
[ -n "${BEACON:-}" ] && CFLAGS="$CFLAGS -DSHIM_BEACON=$BEACON"
[ -n "${POFF:-}" ] && CFLAGS="$CFLAGS -DPAYLOAD_OFF=$POFF"
[ -n "${KOFF:-}" ] && CFLAGS="$CFLAGS -DKENTRY_OFF=$KOFF"
mkdir -p out
for f in start.S shim.c console.c mmu.c fdt.c loader.c ufs.c wdt.c spmi.c; do
	$CC $CFLAGS -c $f -o out/${f%.*}.o
done
abs=$(aarch64-linux-gnu-readelf -r out/shim.o out/console.o out/mmu.o out/fdt.o out/loader.o out/ufs.o out/wdt.o out/spmi.o | grep -c ABS64 || true)
[ "$abs" = 0 ] || { echo "error: $abs absolute relocations" >&2; exit 1; }
$CC $CFLAGS -static -T shim.ld -Wl,--build-id=none -Wl,-N out/start.o out/shim.o out/console.o out/mmu.o out/fdt.o out/loader.o out/ufs.o out/wdt.o out/spmi.o -o out/shim.elf
$OBJCOPY -O binary out/shim.elf out/Image
size=$(aarch64-linux-gnu-nm out/shim.elf | awk '$3=="_end"{print strtonum("0x"$1)}')
truncate -s "$size" out/Image
if [ -n "${1:-}" ]; then
	cp "$1" out/ramdisk.bin
else
	(cd out && rm -rf rd && mkdir rd && cd rd && find . | cpio -o -H newc --quiet | gzip -9 > ../ramdisk.bin)
fi
# ABL's LinuxLoader only jumps when the kernel area is QCOM's Image.gz-dtb:
# gzip(Image) followed by the *raw* DTB. Appending the DTB before gzip (or
# omitting it) makes ABL log "DTB offset is incorrect" and stay in fastboot.
KERNEL=out/Image
SUFFIX=
if [ "${GZ:-0}" = 1 ]; then
	gzip -9 -n -c out/Image > out/Image.gz
	KERNEL=out/Image.gz
	SUFFIX=-gz
	if [ "${DTB_RAW:-1}" = 1 ] && [ -f ../dtb.img ]; then
		cat out/Image.gz ../dtb.img > out/Image.gz-dtb
		KERNEL=out/Image.gz-dtb
		SUFFIX=-gzdtb
	fi
fi
IMG=out/shim-boot.img
[ "${PING:-0}" = 1 ] && IMG=out/shim-ping.img
[ "${PROBE:-0}" = 1 ] && IMG=out/shim-probe.img
[ "${PURE:-0}" != 0 ] && IMG=out/shim-pure.img
[ -n "${BEACON:-}" ] && IMG=out/shim-beacon.img
IMG="${IMG%.img}${SUFFIX}.img"
mkbootimg --kernel "$KERNEL" --ramdisk out/ramdisk.bin --base 0 --kernel_offset 0x8000 --ramdisk_offset 0x1000000 --second_offset 0 --tags_offset 0x100 --pagesize 4096 --os_version 8.0.0 --os_patch_level 2018-11 --cmdline 'androidboot.hardware=qcom' -o "$IMG"
gcc -O1 -Wall -o out/host_test console.c host_test.c && ./out/host_test > /dev/null
echo "kernel package: $KERNEL"
ls -l out/Image "$IMG"
