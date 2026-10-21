#!/usr/bin/env python3
"""Edit verify-tree sources to the final 0015 state, then emit the patch."""
import sys, subprocess

SRC = "/root/xnu-verify/xnu-xnu-8796.141.3"

def patch(path, old, new):
    p = f"{SRC}/{path}"
    t = open(p).read()
    assert t.count(old) == 1, f"{path}: pattern count {t.count(old)}"
    open(p, "w").write(t.replace(old, new, 1))
    print("edit ok:", path)

# --- 1. start.s: AA01 dual write ---
patch("osfmk/arm64/start.s",
"""	MOV64	x10, 0x146bf65c
	MOV64	x11, 0x7766AA01
	str		w11, [x10]
	dsb		sy""",
"""	MOV64	x10, 0x146bf65c
	MOV64	x11, 0x7766AA01
	str		w11, [x10]
	/* durable mirror: debug_region tail survives ABL's field rewrites */
	MOV64	x12, 0xffbff000
	str		w11, [x12]
	dsb		sy""")

# --- 2. start.s: third bootstrap slot for debug_region/pstore block ---
patch("osfmk/arm64/start.s",
"""	MOV64	x10, 0x14000000
	MOV64	x11, (ARM_TTE_TYPE_BLOCK | ARM_TTE_VALID | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) | ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) | ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX)
	orr		x11, x11, x10
	and		x12, x10, #(ARM_TT_L2_INDEX_MASK)
	lsr		x12, x12, #(ARM_TT_L2_SHIFT)
	str		x11, [x13, x12, lsl #TTE_SHIFT]
#endif""",
"""	MOV64	x10, 0x14000000
	MOV64	x11, (ARM_TTE_TYPE_BLOCK | ARM_TTE_VALID | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) | ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) | ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX)
	orr		x11, x11, x10
	and		x12, x10, #(ARM_TT_L2_INDEX_MASK)
	lsr		x12, x12, #(ARM_TT_L2_SHIFT)
	str		x11, [x13, x12, lsl #TTE_SHIFT]
	/* Third slot: the 32MB block at 0xfe000000 (covers reserved
	 * debug_region 0xffb00000 and pstore 0xffc00000) as Device --
	 * durable mirror cell at 0xffbff000. */
	MOV64	x10, 0xfe000000
	MOV64	x11, (ARM_TTE_TYPE_BLOCK | ARM_TTE_VALID | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) | ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) | ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX)
	orr		x11, x11, x10
	and		x12, x10, #(ARM_TT_L2_INDEX_MASK)
	lsr		x12, x12, #(ARM_TT_L2_SHIFT)
	str		x11, [x13, x12, lsl #TTE_SHIFT]
#endif""")

# --- 3. arm_vm_init.c: slot[1] IMEM + slot[2] durable ---
patch("osfmk/arm64/arm_vm_init.c",
"""	*slot = 0x9d400000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_WRITECOMB) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	__asm__ volatile("dsb ishst" ::: "memory");""",
"""	*slot = 0x9d400000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_WRITECOMB) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	/* IMEM restart_reason (0x146bf65c): 16K pages => 32 MiB L2 blocks,
	 * so the aligned block is 0x14000000 and sweet_early_relocate()
	 * points early_rr at (block VA + 0x6bf65c). Device memory. */
	slot[1] = 0x14000000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	/* debug_region tail (0xffbff000): durable checkpoint mirror that ABL
	 * never clobbers -- read it back via fmem from Android. The 32 MiB
	 * block aligns to 0xfe000000. */
	slot[2] = 0xfe000000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	__asm__ volatile("dsb ishst" ::: "memory");""")

# --- 4. sweet_early.c: declarations ---
patch("osfmk/arm64/sweet_early.c",
"""#include <stdint.h>
#include <kern/misc_protos.h>

static volatile uint32_t *early_fb = (volatile uint32_t *)(uintptr_t)0x9d400000;
#define EARLY_FB early_fb""",
"""#include <stdint.h>
#include <kern/misc_protos.h>
#include <arm64/proc_reg.h>

static volatile uint32_t *early_fb = (volatile uint32_t *)(uintptr_t)0x9d400000;
#define EARLY_FB early_fb
/* IMEM restart_reason: mapped VA=PA by start.s, remapped beside the
 * framebuffer alias by arm_vm_init. ABL echoes it into the next Android
 * cmdline as warmboot=0x........  0x77664Dxx = 'M' + stage,
 * 0x776650xx = 'P' panic.  BUT ABL rewrites the official field on
 * `fastboot reboot`, so every mark is mirrored into early_durable --
 * reserved debug_region DDR tail at phys 0xffbff000 (inside the
 * 0xfe000000 32 MiB block) that nothing else touches; read it from
 * Android via fmem. */
static volatile uint32_t *early_rr = (volatile uint32_t *)(uintptr_t)0x146bf65c;
#define EARLY_RR early_rr
static volatile uint32_t *early_durable =
	(volatile uint32_t *)(uintptr_t)0xffbff000;
#define EARLY_DURABLE early_durable
static unsigned int early_mark_seq;""")

patch("osfmk/arm64/sweet_early.c",
"""static unsigned int early_started, early_disabled, early_panicking;""",
"""static unsigned int early_started, early_disabled, early_panicking;
static unsigned int early_last_stage;""")

# --- 5. sweet_early.c: mark ---
patch("osfmk/arm64/sweet_early.c",
"""	if (early_disabled || stage > 20) {
		return;
	}
	uint32_t color""",
"""	if (early_disabled || stage > 20) {
		return;
	}
	early_last_stage = stage;
	*EARLY_RR = 0x77664D00u | stage;
	*EARLY_DURABLE = 0x77664D00u | stage;
	early_durable[1] = ++early_mark_seq;
	__asm__ volatile("dsb sy" ::: "memory");
	uint32_t color""")

# --- 6. sweet_early.c: panic ---
patch("osfmk/arm64/sweet_early.c",
"""	early_panicking = 1;
	sweet_early_string(""",
"""	early_panicking = 1;
	*EARLY_RR = 0x77665000u | (early_last_stage & 0xff);
	*EARLY_DURABLE = 0x77665000u | (early_last_stage & 0xff);
	__asm__ volatile("dsb sy" ::: "memory");
	sweet_early_string(""")

# --- 7. sweet_early.c: relocate ---
patch("osfmk/arm64/sweet_early.c",
"""	early_fb = (volatile uint32_t *)(uintptr_t)base;
	__asm__ volatile("dsb sy" ::: "memory");""",
"""	early_fb = (volatile uint32_t *)(uintptr_t)(base + 0x1400000);
	early_rr = (volatile uint32_t *)(uintptr_t)(base + ARM_TT_L2_SIZE + 0x6bf65c);
	early_durable = (volatile uint32_t *)(uintptr_t)(base + 2 * ARM_TT_L2_SIZE + 0x1bff000);
	__asm__ volatile("dsb sy" ::: "memory");""")

out = subprocess.run(
    ["git", "-C", SRC, "diff", "--",
     "osfmk/arm64/start.s", "osfmk/arm64/arm_vm_init.c", "osfmk/arm64/sweet_early.c"],
    capture_output=True, text=True).stdout
sys.stdout.write(out)
