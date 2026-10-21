/*
 * Milestone 4: act as iBoot for xnu.
 *
 * mkpayload.py already resolved the kernel's chained fixups and built the
 * ADT, so this only has to: find the payload in the ABL ramdisk, copy the
 * segments to phys_base + (vmaddr - virt_base), place the ADT and
 * boot_args behind the kernel, and jump to the entry point with the MMU
 * and caches off, x0 = boot_args (physical).
 */
#include <stdint.h>
#include "console.h"
#include "fdt.h"
#include "loader.h"

#define BOOT_LINE_LENGTH	608
#define PAGE_16K		0x4000UL

struct payload_seg {
	uint64_t vmaddr, vmsize, file_off, file_size;
};

struct payload {
	char magic[8];
	uint32_t version, nsegs;
	uint64_t virt_base, entry_va, adt_off, adt_size, kernel_end;
	uint64_t phys_base, mem_size;
	char cmdline[BOOT_LINE_LENGTH];
	struct payload_seg segs[];
};

/* pexpert/pexpert/arm64/boot.h, Revision/Version 2 */
struct boot_video {
	uint64_t v_baseAddr, v_display, v_rowBytes, v_width, v_height, v_depth;
};

struct boot_args {
	uint16_t Revision, Version;
	uint64_t virtBase, physBase, memSize, topOfKernelData;
	struct boot_video Video;
	uint32_t machineType;
	uint64_t deviceTreeP;
	uint32_t deviceTreeLength;
	char CommandLine[BOOT_LINE_LENGTH];
	uint64_t bootFlags, memSizeActual;
};

_Static_assert(__builtin_offsetof(struct payload, segs) == 0x2a8, "payload header layout");
_Static_assert(__builtin_offsetof(struct boot_args, Video) == 40, "boot_args layout");
_Static_assert(__builtin_offsetof(struct boot_args, CommandLine) == 108, "boot_args layout");
_Static_assert(sizeof(struct boot_args) == 736, "boot_args layout");

/* MMU off: everything is Device memory, so copy in aligned 8-byte words. */
static void copy64(uintptr_t dst, uintptr_t src, uint64_t n)
{
	volatile uint64_t *d = (volatile uint64_t *)dst;
	volatile const uint64_t *s = (volatile const uint64_t *)src;
	for (uint64_t i = 0; i < (n + 7) / 8; i++)
		d[i] = s[i];
}

static void zero64(uintptr_t dst, uint64_t n)
{
	volatile uint64_t *d = (volatile uint64_t *)dst;
	for (uint64_t i = 0; i < n / 8; i++)
		d[i] = 0;
}

static int magic_ok(const char *m)
{
	static const char want[] = "SWEETXNU";
	for (int i = 0; i < 8; i++)
		if (m[i] != want[i])
			return 0;
	return 1;
}

typedef void (*xnu_entry_t)(uint64_t boot_args_pa);

const char *loader_boot(uintptr_t dtb)
{
	uint64_t rd_start, rd_end;

	if (!fdt_chosen_u64(dtb, "linux,initrd-start", &rd_start) ||
	    !fdt_chosen_u64(dtb, "linux,initrd-end", &rd_end))
		return "no initrd in /chosen";
	con_hex2("INITRD", "lo=", rd_start, "hi=", rd_end);

	const struct payload *p = (const struct payload *)rd_start;
	if (!magic_ok(p->magic))
		return "no SWEETXNU payload";
	if (p->version != 1 || (p->adt_off & 7) || (rd_start & 7))
		return "bad payload header";

	uint64_t pa_of_va_base = p->phys_base - p->virt_base;	/* pa = va + this */
	con_hex2("PHYS", "base=", p->phys_base, "size=", p->mem_size);
	con_hex("VIRT", p->virt_base);
	con_hex("ENTRY", p->entry_va);

	/* The payload must not overlap where the kernel goes. */
	if (rd_end > p->phys_base && rd_start < p->phys_base + p->mem_size)
		return "ramdisk overlaps kernel memory";

	for (uint32_t i = 0; i < p->nsegs; i++) {
		const struct payload_seg *s = &p->segs[i];
		uintptr_t dst = s->vmaddr + pa_of_va_base;
		copy64(dst, rd_start + s->file_off, s->file_size);
		if (s->vmsize > s->file_size)
			zero64(dst + ((s->file_size + 7) & ~7UL), s->vmsize - ((s->file_size + 7) & ~7UL));
	}
	con_hex2("SEGS", "n=", p->nsegs, "end=", p->kernel_end + pa_of_va_base);

	/* ADT, then boot_args, behind the kernel; everything below topOfKernelData is ours. */
	uintptr_t adt_pa = p->kernel_end + pa_of_va_base;
	copy64(adt_pa, rd_start + p->adt_off, p->adt_size);
	uintptr_t ba_pa = (adt_pa + p->adt_size + PAGE_16K - 1) & ~(PAGE_16K - 1);
	uintptr_t top = ba_pa + PAGE_16K;

	zero64(ba_pa, PAGE_16K);
	volatile struct boot_args *ba = (volatile struct boot_args *)ba_pa;
	ba->Revision = 2;
	ba->Version = 2;
	ba->virtBase = p->virt_base;
	ba->physBase = p->phys_base;
	ba->memSize = p->mem_size;
	ba->topOfKernelData = top;
	ba->Video.v_baseAddr = FB_BASE;
	ba->Video.v_display = 1;
	ba->Video.v_rowBytes = FB_STRIDE;
	ba->Video.v_width = FB_W;
	ba->Video.v_height = FB_H;
	ba->Video.v_depth = 32;
	ba->deviceTreeP = adt_pa - pa_of_va_base;	/* xnu reads it with the MMU on: a VA */
	ba->deviceTreeLength = (uint32_t)p->adt_size;
	for (int i = 0; i < BOOT_LINE_LENGTH; i++)
		ba->CommandLine[i] = p->cmdline[i];
	ba->memSizeActual = p->mem_size;
	con_hex2("BOOTARG", "pa=", ba_pa, "top=", top);

	uintptr_t entry_pa = p->entry_va + pa_of_va_base;
	con_hex("JUMP", entry_pa);
	con_str("XNU", "jumping (fbcon clears screen)");

	/* Pre-jump mortem probes. The only channel proven to survive a WDT
	 * bite is the IMEM restart_reason field → next cmdline warmboot=;
	 * DDR stash regions get rewritten by the boot chain. Marks are
	 * 0x7766D0xx (last write wins): a hang leaves the mark of the
	 * step that killed us. */
#define RR_MARK(x) (*(volatile uint32_t *)0x146bf65cUL = (x))
#define PSHOLD_REG (*(volatile uint32_t *)0x10ac000UL)

	/* 1) entry placement: first 64 words at entry_pa must match the
	 *    payload source segment containing entry_va. Mismatch → reset
	 *    with 0x7766D0<idx> (idx = 1..64, 0xff = entry not in any seg). */
	RR_MARK(0x7766D010);
	{
		const uint32_t *src = 0;
		for (uint32_t i = 0; i < p->nsegs; i++) {
			const struct payload_seg *s = &p->segs[i];
			uintptr_t lo = s->vmaddr + pa_of_va_base;
			if (entry_pa >= lo && entry_pa + 4 <= lo + s->file_size) {
				src = (const uint32_t *)(rd_start + s->file_off +
							 (entry_pa - lo));
				break;
			}
		}
		uint32_t bad = src ? 0 : 0xff;
		for (uint32_t i = 0; src && i < 64; i++)
			if (((volatile uint32_t *)entry_pa)[i] != src[i]) {
				bad = i + 1;
				break;
			}
		if (bad) {
			RR_MARK(0x7766D000 | bad);
			__asm__ volatile("dsb sy" ::: "memory");
			PSHOLD_REG = 0;
			for (;;)
				;
		}
	}

	/* 2) FB band: kernel Stage-0 stores to 0x9d400000+1248*2880 before AA01 */
	RR_MARK(0x7766D011);
	*(volatile uint32_t *)(0x9d400000UL + 1248 * 2880) = 0x00ff00ff;
	__asm__ volatile("dsb sy" ::: "memory");
	RR_MARK(0x7766D012);

	/* 3) OSLAR_EL1: first insn of start_first_cpu, suspect on QC debug-gated SoCs */
	RR_MARK(0x7766D013);
	__asm__ volatile("msr OSLAR_EL1, xzr" ::: "memory");
	RR_MARK(0x7766D014);

	/* 4) install mark-writing exception vectors for the kernel's blind
	 *    window: XNU doesn't set VBAR until AFTER AA01, so any exception
	 *    between our jump and that point lands wherever the previous
	 *    VBAR pointed (stale => wild => silent death). Each slot writes
	 *    0x7766F0<idx> to restart_reason and spins, so warmboot reveals
	 *    the exact exception class: 0/4=sync, 1/5=IRQ, 2/6=FIQ, 3/7=SErr. */
	RR_MARK(0x7766D015);
	{
		static uint8_t vt[2048] __attribute__((aligned(2048)));
		for (int i = 0; i < 16; i++) {
			uint32_t *s = (uint32_t *)(vt + i * 0x80);
			uint32_t mark = 0xF000u | (uint32_t)i;
			s[0] = 0x52800000u | (mark << 5) | 9u;		/* mov w9, #mark */
			s[1] = 0x72A00000u | (0x7766u << 5) | 9u;		/* movk w9, #0x7766, lsl#16 */
			s[2] = 0x52800000u | (0xf65cu << 5) | 10u;	/* mov w10, #0xf65c */
			s[3] = 0x72A00000u | (0x146bu << 5) | 10u;	/* movk w10, #0x146b, lsl#16 */
			s[4] = 0xB9000149u;				/* str w9, [x10] */
			s[5] = 0x14000000u;				/* b . */
		}
		uint64_t cel;
		__asm__ volatile("mrs %0, CurrentEL" : "=r"(cel));
		__asm__ volatile("msr VBAR_EL1, %0" :: "r"((uint64_t)vt));
		if (cel >= (2u << 2))
			__asm__ volatile("msr VBAR_EL2, %0" :: "r"((uint64_t)vt));
		__asm__ volatile("dsb sy; isb" ::: "memory");
	}
	RR_MARK(0x7766D016);

	/* Dump the console log into a pstore prz record before jumping. The
	 * only existing writer runs at the end of shim_main and is never
	 * reached on the xnu path (we don't return). Whether prz survives
	 * a WDT bite is UNVERIFIED — if it does, /sys/fs/pstore shows the
	 * full shim log (INITRD/PHYS/ENTRY/SEGS/BOOTARG/JUMP + marks). */
	{
		volatile uint32_t *prz = (volatile uint32_t *)0xffcc0000UL;
		uint32_t ln;
		char *lb = con_log(&ln);
		volatile char *dst = (volatile char *)(prz + 3);
		for (uint32_t i = 0; i < ln; i++)
			dst[i] = lb[i];
		prz[2] = ln;
		prz[1] = 0;
		prz[0] = 0x43474244;
	}

	/* Leave a legal route in restart_reason: once xnu takes over, the only
	   remaining checkpoint is C2 (0x7766C032), which ABL cannot route — a
	   watchdog bite or xnu reset would land in fastboot instead of Android.
	   0x77665501 makes every later reset recover to Android + adb. */
	*(volatile uint32_t *)0x146bf65cUL = 0x77665501u;
	__asm__ volatile("dsb sy" ::: "memory");

	/* Everything was written with the MMU and caches off; drop any stale I-cache lines too. */
	__asm__ volatile("dsb sy; ic iallu; dsb sy; isb" ::: "memory");
	((xnu_entry_t)entry_pa)(ba_pa);
	return "xnu returned";
}
