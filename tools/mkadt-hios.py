#!/usr/bin/env python3
"""Build the Apple-format device tree a hackios (BCM2837-base) xnu expects,
with lilac/MSM8998 values — a port of xnuloader/loader.c's dt_writer
sequence (hackios-xnu-kernel, oosexgang) onto tools/adt.py.

  mkadt-hios.py OUT.adt [--dump] [--highmem]

Differences from mkadt.py (SWEET kernel): this kernel reads a
loader-generated tree, not iBoot's — so we reproduce the loader's exact
node set: /chosen with loader/NVRAM/panic props, /defaults, /product
(real D22AP), /options, /pram, /cpus (MPIDR regs, psci-method=smc),
/arm-io {ranges, gic} — nothing SWEET-specific (no sweet-fbcon/disp0).
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import adt  # noqa: E402
from mkadt import d22ap_product, node, cstr, u32, u64

# ---- lilac (MSM8998) constants -------------------------------------------
PHYS_BASE = 0x9f800000            # free RAM above splash_region
MEM_SIZE  = 0x5b800000            # to debug_region@0xffb00000
HIGH_PHYS_BASE, HIGH_MEM_SIZE = 0x100000000, 0xfcc00000
IO_BASE, IO_SIZE = 0x00400000, 0x1FC00000   # covers GIC/UFS/IMEM/RPM/PM8998
GICD, GICD_SIZE = 0x17a00000, 0x10000
GICR, GICR_SIZE = 0x17b00000, 0x100000      # lilac DTB gic reg[1]
PRAM_BASE, PRAM_SIZE = 0xfb000000, 0x80000  # outside the kernel window
CNTFRQ = 19200000                 # 19.2 MHz armv8 timer
CORE_MPIDR = [0, 1, 2, 3, 0x100, 0x101, 0x102, 0x103]  # cpu@0-3 + cpu@100-103
SMALL_CORES = 0x0f                # low four are the Silver cluster

PANIC_LOG_SIZE = PRAM_SIZE
NVRAM_BANK_SIZE = 0x2000


def chrp_header(sig, nbytes, name):
    """CHRP partition header: sig, adler-style 8-bit checksum, size in
    16-byte blocks (u16 BE), name — as the hackios loader writes it."""
    h = bytearray(16)
    h[0] = sig
    blocks = nbytes // 16
    h[2] = blocks & 0xff
    h[3] = (blocks >> 8) & 0xff
    h[4:4 + len(name)] = name.encode()
    s = h[0] + sum(h[2:16])
    while s > 0xff:
        s = (s & 0xff) + (s >> 8)
    h[1] = s & 0xff
    return bytes(h)


def nvram_image():
    """An empty but well-formed NVRAM bank — IOKit refuses to start without it."""
    img = bytearray(NVRAM_BANK_SIZE)
    img[0:16] = chrp_header(0x5a, 0x20, 'nvram')
    img[0x20:0x30] = chrp_header(0x70, NVRAM_BANK_SIZE - 0x20, 'common')
    struct.pack_into('<I', img, 20, 1)          # generation
    lo, hi = 1, 0                               # Adler-32 over [20:]
    for b in img[20:]:
        lo = (lo + b) % 65521
        hi = (hi + lo) % 65521
    struct.pack_into('<I', img, 16, (hi << 16) | lo)
    return bytes(img)


def padded(s, size):
    f = bytearray(size)
    f[:len(s)] = s.encode()
    return bytes(f)


def build(seed=None, highmem=False):
    phys_base, mem_size = (HIGH_PHYS_BASE, HIGH_MEM_SIZE) if highmem else (PHYS_BASE, MEM_SIZE)
    seed = seed if seed is not None else os.urandom(64)
    phandle = iter(range(1, 1000))
    ph = lambda: ('AAPL,phandle', u32(next(phandle)))

    chosen = node('chosen', [
        ph(),
        ('random-seed', seed),
        ('dram-base', u64(phys_base)),
        ('dram-size', u64(mem_size)),
        ('debug-enabled', u32(1)),
        ('display-scale', u32(3)),
        ('display-rotation', u32(0)),
        ('psci-method', cstr('smc')),
        ('unique-chip-id', u64(0x000052445730001)),
        ('chip-id', u32(0x8015)),
        ('board-id', u32(0x06)),
        ('mac-address-wifi0', bytes([0x02, 0x52, 0x44, 0x57, 0x00, 0x01])),
        ('mac-address-bluetooth0', bytes([0x02, 0x52, 0x44, 0x57, 0x00, 0x02])),
        ('boot-manifest-hash', b'redwood' + b'\0' * 41),
        ('software-behavior', b'\0' * 16),
        ('firmware-version', cstr('iBoot-8422.100.650')),
        ('certificate-production-status', u32(1)),
        ('certificate-security-mode', u32(1)),
        ('effective-production-status-ap', u32(1)),
        ('effective-security-mode-ap', u32(1)),
        ('secure-boot', u32(1)),
        ('production-cert', u32(1)),
        ('development-cert', u32(0)),
        ('embedded-panic-log-size', u32(PANIC_LOG_SIZE)),
        ('nvram-bank-size', u32(NVRAM_BANK_SIZE)),
        ('nvram-bank-count', u32(1)),
        ('nvram-current-bank', u32(0)),
        ('nvram-proxy-data', nvram_image()),
    ], [node('memory-map', [ph()])])

    defaults = node('defaults', [ph(), ('kern.vm_compressor', u32(2))])

    product_children = [node('haptics', [ph()])]
    product_props = d22ap_product() or []
    product = node('product', [ph()] + product_props, product_children)

    options = node('options', [ph()])

    pram = node('pram', [ph(), ('reg', u64(PRAM_BASE, PRAM_SIZE))])

    cpu_nodes = []
    for i, mpidr in enumerate(CORE_MPIDR):
        props = [
            ph(),
            ('device_type', cstr('cpu')),
            ('state', cstr('running' if i == 0 else 'waiting')),
            ('reg', u32(mpidr)),
            ('cpu-id', u32(i)),
            ('timebase-frequency', u64(CNTFRQ)),
            ('fixed-frequency', u64(CNTFRQ)),
            ('clock-frequency', u64(1800000000)),
            ('bus-frequency', u64(100000000)),
            ('memory-frequency', u64(100000000)),
            ('peripheral-frequency', u64(24000000)),
        ]
        if SMALL_CORES & (1 << ((mpidr >> 8) & 31)):
            props.append(('small-core', u32(1)))
        cpu_nodes.append(node(f'cpu{i}', props))
    cpus = node('cpus', [ph()], cpu_nodes)

    gic = node('gic', [
        ph(),
        ('device_type', cstr('interrupt-controller')),
        ('compatible', cstr('arm,gic-v3')),
        ('reg', u64(GICD - IO_BASE, GICD_SIZE, GICR - IO_BASE, GICR_SIZE)),
    ])
    arm_io = node('arm-io', [
        ph(),
        ('device_type', cstr('lilac')),
        ('ranges', u64(0, IO_BASE, IO_SIZE)),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
    ], [gic])

    # Root identity = iPhone X (D22AP) — same spoofing hackios uses.
    return node('device-tree', [
        ph(),
        ('target-type', cstr('D22')),
        ('manufacturer', cstr('Apple Inc.')),
        ('target-sub-type', cstr('D22AP')),
        ('compatible', cstr('D22AP') + cstr('iPhone10,3')),
        ('model', cstr('iPhone10,3')),
        ('device_type', cstr('bootrom')),
        ('platform-name', cstr('t8015')),
        ('serial-number', padded('LLC00XZ1CMPCT', 32)),
        ('mlb-serial-number', padded('LLC00XZ1CMPCTMLB00', 32)),
        ('model-number', padded('MQAC2', 32)),
        ('region-info', padded('LL/A', 32)),
        ('regulatory-model-number', padded('A1865', 32)),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
    ], [chosen, defaults, product, options, pram, cpus, arm_io])


if __name__ == '__main__':
    root = build(highmem='--highmem' in sys.argv)
    blob = adt.serialize(root)
    open(sys.argv[1], 'wb').write(blob)
    if '--dump' in sys.argv:
        adt.dump(adt.parse(blob)[0])
    print(f'{sys.argv[1]}: {len(blob)} bytes', file=sys.stderr)
