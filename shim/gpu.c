/*
 * Platform-stage GPU PAS probe (sagit gpu_handoff.cpp model).
 *
 * Runs inside the shim, after the ADT has been copied to low RAM and before
 * XNU starts. Purpose: prove whether TZ accepts a GPU PAS init/mem_setup/
 * auth_and_reset round trip inside the ABL handoff window (shim EL=1).
 *
 * Everything is physical (identity-mapped, Device attributes). The ZAP MDT
 * and segment blobs are found through the payload ADT's
 * /chosen/memory-map entries GPU-ZAP-MDT and GPU-ZAP-SEG, exactly like the
 * kernel driver does at bsd_init.
 *
 * Gate: payload cmdline contains "shimgpu=1".
 */
#include <stdint.h>
#include "console.h"
#include "gpu.h"

#define GPU_PAS_ID	13u
#define ZAP_PA		0x95200000u
#define ZAP_MAX		0x00100000u

struct payload_hdr {
	char magic[8];
	uint32_t version, nsegs;
	uint64_t virt_base, entry_va, adt_off, adt_size, kernel_end;
	uint64_t phys_base, mem_size;
	char cmdline[608];
};

struct zap_phdr {
	uint32_t type, offset, vaddr, paddr, filesz, memsz, flags, align;
};

static uint32_t rd32(uintptr_t a) { return *(volatile uint32_t *)a; }
static uint64_t rd64u(uintptr_t a)
{
	/* ADT property values are only 4-byte aligned. */
	return (uint64_t)rd32(a) | ((uint64_t)rd32(a + 4) << 32);
}
static void wr32(uintptr_t a, uint32_t v) { *(volatile uint32_t *)a = v; }

static uint64_t rsr_ctr(void) { uint64_t v; __asm__ volatile("mrs %0, ctr_el0" : "=r"(v)); return v; }

static void dc_clean(uintptr_t s, uintptr_t e)
{
	uint64_t line = 4UL << ((rsr_ctr() >> 16) & 0xf);
	for (uintptr_t a = s & ~(line - 1); a < e; a += line)
		__asm__ volatile("dc cvac, %0" : : "r"(a) : "memory");
	__asm__ volatile("dsb sy" ::: "memory");
}

static int str_has(const char *s, const char *needle)
{
	for (; *s; s++) {
		const char *a = s, *b = needle;
		while (*b && *a == *b) { a++; b++; }
		if (!*b)
			return 1;
	}
	return 0;
}

/* ---- Apple device tree: just enough to read memory-map entries ---- */

static int adt_prop_name(uintptr_t node, const char *want, uintptr_t *val,
			 uint32_t *len)
{
	uint32_t nprops = rd32(node);
	uintptr_t off = node + 8;
	for (uint32_t i = 0; i < nprops && i < 4096; i++) {
		const char *name = (const char *)off;
		uint32_t size = rd32(off + 32);
		uint32_t flags = size >> 31, n = size & 0x7fffffff;
		uintptr_t v = off + 36;
		int match = 1;
		for (int k = 0; k < 32; k++) {
			char c = want[k];
			if (c == 0)
				break;
			if (name[k] != c) { match = 0; break; }
		}
		if (match) {
			if (val) *val = v;
			if (len) *len = n;
			return 1;
		}
		(void)flags;
		off = v + ((n + 3) & ~3u);
	}
	return 0;
}

static uintptr_t adt_first_child(uintptr_t node)
{
	uint32_t nprops = rd32(node);
	uintptr_t off = node + 8;
	for (uint32_t i = 0; i < nprops && i < 4096; i++) {
		uint32_t size = rd32(off + 32);
		uint32_t n = size & 0x7fffffff;
		off = off + 36 + ((n + 3) & ~3u);
	}
	return off;
}

static uintptr_t adt_next_sibling(uintptr_t child)
{
	/* skip this node's props and all descendants */
	uint32_t nprops = rd32(child), nchild = rd32(child + 4);
	uintptr_t off = child + 8;
	for (uint32_t i = 0; i < nprops && i < 4096; i++) {
		uint32_t n = rd32(off + 32) & 0x7fffffff;
		off += 36 + ((n + 3) & ~3u);
	}
	for (uint32_t i = 0; i < nchild; i++)
		off = adt_next_sibling(off);
	return off;
}

static int adt_child_named(uintptr_t node, const char *name, uintptr_t *out)
{
	uintptr_t c = adt_first_child(node);
	uint32_t nchild = rd32(node + 4);
	for (uint32_t i = 0; i < nchild && i < 4096; i++) {
		uintptr_t v; uint32_t n;
		if (adt_prop_name(c, "name", &v, &n)) {
			const char *s = (const char *)v;
			int match = 1;
			for (int k = 0; k < 32; k++) {
				if (name[k] == 0) break;
				if (s[k] != name[k]) { match = 0; break; }
			}
			if (match) { *out = c; return 1; }
		}
		c = adt_next_sibling(c);
	}
	return 0;
}

static int adt_mmap_entry(uintptr_t adt, const char *name, uint64_t *pa,
			  uint64_t *size)
{
	uintptr_t chosen, mmap, val;
	uint32_t len;
	if (!adt_child_named(adt, "chosen", &chosen))
		return 0;
	if (!adt_child_named(chosen, "memory-map", &mmap))
		return 0;
	if (!adt_prop_name(mmap, name, &val, &len) || len < 16)
		return 0;
	*pa = rd64u(val);
	*size = rd64u(val + 8);
	return 1;
}

/* ---- SCM ---- */

struct scm_ret { int64_t status; uint64_t value; };

static struct scm_ret scm_call(uint32_t svc, uint32_t cmd, uint32_t arginfo,
			       uint64_t a, uint64_t b, uint64_t c)
{
	uint64_t fn = (1ull << 30) | (2ull << 24) | ((uint64_t)svc << 8) | cmd;
	for (int busy = 0;; busy++) {
		uint64_t a0 = fn, a6 = 0;
		register uint64_t x0 __asm__("x0"), x1 __asm__("x1");
		register uint64_t x2 __asm__("x2"), x3 __asm__("x3");
		register uint64_t x4 __asm__("x4"), x5 __asm__("x5");
		register uint64_t x6 __asm__("x6"), x7 __asm__("x7");
		do {
			x0 = a0; x1 = arginfo; x2 = a; x3 = b; x4 = c;
			x5 = 0; x6 = a6; x7 = 0;
			__asm__ volatile("smc #0"
			    : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
			      "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
			    :
			    : "x8", "x9", "x10", "x11", "x12", "x13", "x14",
			      "x15", "x16", "x17", "memory");
			a0 = 1;			/* Linux continuation */
			a6 = x6;		/* cookie from monitor */
		} while (x0 == 1);
		struct scm_ret r = { (int64_t)x0, x1 };
		if (r.status != -12 || busy >= 20)	/* -EBUSY */
			return r;
		for (volatile int i = 0; i < 200000; i++)
			;
	}
}

/* ---- probe ---- */

static uint64_t zap_meta[2048] __attribute__((aligned(64)));

static void con_hex3(const char *label, const char *a, uint32_t va,
		     const char *b, uint32_t vb, const char *c, uint32_t vc)
{
	con_hex2(label, a, va, b, vb);
	con_hex2(" ", c, vc, "", 0);
}

static void gpu_delay_ms(unsigned ms)
{
	uint64_t f, t0, t;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
	do { __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t)); }
	while ((t - t0) * 1000 < f * ms);
}

int spmi_find_apid(uint16_t ppid);
int spmi_write_apid(int apid, uint16_t addr, uint8_t val);

static int shim_gpu_cp(const void *payload, uintptr_t adt_pa);

static void shim_gpu_power(void)
{
	/* GPUCC 0x05065000: CX/GX GDSC, clamps, GPLL0 and gfx3d RCG. */
	const uintptr_t cc = 0x05065000u;
	int a1 = spmi_find_apid(0x514), a4 = spmi_find_apid(0x51d);

	/* PM8005 S1 (GX) / S4 (CX) rails: enable at the inherited setpoint
	 * before any GDSC or SECVID access, as the kernel does at level 2. */
	if (a1 >= 0) {
		spmi_write_apid(a1, 0x1446, 0x80);
	}
	if (a4 >= 0) {
		spmi_write_apid(a4, 0x1d46, 0x80);
	}
	gpu_delay_ms(5);
	con_hex2("GPU RAIL", "s1=", (uint32_t)a1, "s4=", (uint32_t)a4);
	uint64_t f, t0, t;
	int locked = 0;
	uint32_t mode, uctl;

	wr32(cc + 0x1004, 0x222000u);
	gpu_delay_ms(10);
	wr32(cc + 0x1090, 1u);
	gpu_delay_ms(1);
	wr32(cc + 0x1090, 0u);
	wr32(cc + 0x0130, 0x11u);
	gpu_delay_ms(1);
	wr32(cc + 0x0130, 0x01u);
	gpu_delay_ms(1);
	wr32(cc + 0x0130, 0x00u);
	wr32(cc + 0x1094, 0x222000u);
	gpu_delay_ms(10);

	con_hex2("GPU PWR", "cx=", rd32(cc + 0x1004), "gx=", rd32(cc + 0x1094));

	wr32(cc + 0x0004, 0x12u);
	wr32(cc + 0x0008, 0x3fu);
	wr32(cc + 0x0038, 0xc00u);
	wr32(cc + 0x0014, 0x20485699u);
	wr32(cc + 0x0018, 0x2067u);
	wr32(cc + 0x0010, 0x4805u);
	wr32(cc + 0x000c, 0x7u);
	mode = rd32(cc + 0x0000);
	wr32(cc + 0x0000, mode & ~1u);
	wr32(cc + 0x002c, 0x0u);
	gpu_delay_ms(1);
	wr32(cc + 0x0000, (mode & ~1u) | 0x4u);
	wr32(cc + 0x002c, 0x1u);
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
	do {
		if (rd32(cc + 0x0000) & 0x80000000u) { locked = 1; break; }
		__asm__ volatile("mrs %0, cntpct_el0" : "=r"(t));
	} while ((t - t0) * 1000 < f * 5);
	uctl = rd32(cc + 0x000c);
	wr32(cc + 0x000c, uctl | 0x7u);
	mode = rd32(cc + 0x0000);
	wr32(cc + 0x0000, mode | 0x1u);
	con_hex2("GPU PLL", "lock=", (uint32_t)locked, "mode=", rd32(cc + 0x0000));
	wr32(cc + 0x1078, 0x0u);
	wr32(cc + 0x107c, 0xffffffffu);
	wr32(cc + 0x1080, 0xffffffffu);
	wr32(cc + 0x1074, 0x101u);
	wr32(cc + 0x1070, 0x0u);
	gpu_delay_ms(5);
	wr32(cc + 0x1098, 1u);
	gpu_delay_ms(5);
	con_hex2("GPU RCG", "cmd=", rd32(cc + 0x1070), "cfg=", rd32(cc + 0x1074));
}

void shim_gpu_probe(const void *payload, uintptr_t adt_pa)
{
	const struct payload_hdr *p = payload;
	uint64_t mdt_pa, mdt_len, seg_pa, seg_len;
	struct scm_ret r;

	if (!str_has(p->cmdline, "shimgpu=1")) {
		return;
	}
	con_str("GPU PAS", "probe enter");

	if (!adt_mmap_entry(adt_pa, "GPU-ZAP-MDT", &mdt_pa, &mdt_len) ||
	    !adt_mmap_entry(adt_pa, "GPU-ZAP-SEG", &seg_pa, &seg_len)) {
		con_str("GPU PAS", "blobs missing");
		return;
	}
	con_hex2("GPU MDT", "pa=", (uint32_t)mdt_pa, "len=", (uint32_t)mdt_len);
	con_hex2("GPU SEG", "pa=", (uint32_t)seg_pa, "len=", (uint32_t)seg_len);

	if (mdt_len < 0x34 || mdt_len > 16384) {
		con_str("GPU PAS", "mdt size bad");
		return;
	}
	{
		const uint8_t *m = (const uint8_t *)mdt_pa;
		for (uint32_t i = 0; i < (uint32_t)mdt_len; i++)
			((uint8_t *)zap_meta)[i] = m[i];
	}
	dc_clean((uintptr_t)zap_meta, (uintptr_t)zap_meta + 16384);

	{
		const uint8_t *m = (const uint8_t *)zap_meta;
		uint32_t phoff = rd32((uintptr_t)m + 0x1c);
		uint16_t phnum = *(const uint16_t *)(m + 0x2c);
		uint32_t lo = 0, hi = 0, filesz = 0, memsz = 0, nload = 0;
		for (uint32_t i = 0; i < phnum && phoff + (i + 1) * 32 <= mdt_len; i++) {
			const struct zap_phdr *ph =
			    (const struct zap_phdr *)(m + phoff + i * 32);
			if (ph->type != 1 || ((ph->flags >> 24) & 7) == 2 ||
			    ph->memsz == 0)
				continue;
			nload++;
			lo = ph->paddr;
			hi = (ph->paddr + ph->memsz + 0xfff) & ~0xfffu;
			filesz = ph->filesz;
			memsz = ph->memsz;
		}
		con_hex2("GPU PHDR", "n=", nload,
			 "filesz=", filesz);
		if (nload != 1 || filesz != (uint32_t)seg_len || memsz > ZAP_MAX) {
			con_str("GPU PAS", "layout bad");
			return;
		}

		/* 1. init_image: metadata buffer (PA), image id 13 */
		r = scm_call(2, 1, 2u | (2u << 6), GPU_PAS_ID,
			     (uint64_t)zap_meta, 0);
		con_hex2("PAS init", "st=", (uint32_t)r.status,
			 "v=", (uint32_t)r.value);
		if (r.status != 0 || r.value != 0)
			return;

		/* 2. mem_setup: carveout region */
		r = scm_call(2, 2, 3, GPU_PAS_ID, ZAP_PA, hi - lo);
		con_hex2("PAS setup", "st=", (uint32_t)r.status,
			 "v=", (uint32_t)r.value);
		if (r.status != 0 || r.value != 0)
			return;

		/* copy the signed segment into the carveout (Device memory) */
		{
			const uint32_t *src = (const uint32_t *)seg_pa;
			uint32_t words = (memsz + 3) & ~3u;
			for (uint32_t off = 0; off < words; off += 4)
				wr32(ZAP_PA + off,
				     off < seg_len ? src[off / 4] : 0);
		}
		__asm__ volatile("dsb sy" ::: "memory");

		/* 3. auth_and_reset */
		r = scm_call(2, 5, 1, GPU_PAS_ID, 0, 0);
		con_hex3("PAS auth", "st=", (uint32_t)r.status,
			 "v=", (uint32_t)r.value, "lo=", lo);
		if (r.status == 0 && r.value == 0)
			con_str("GPU PAS", "OK at shim stage");
		else
			con_str("GPU PAS", "auth failed");

		/* AHB reads of the GPU need its clocks: power GX/CX and GPLL0
		 * first (same sequence proven by the kernel probe). The shim
		 * console has a fixed row count, so clear and start a fresh page
		 * for this section. */
		con_clear(BLUE);
		con_str("GPU2", "power + TZ state after PAS");
		shim_gpu_power();
		/* SECVID reads stall until the full GPMU/zap power cycle has run
		 * (kernel-level logs confirm they are readable later). Skip them
		 * here so the shim can hand over cleanly; the CP producer stage
		 * will revisit them after its own GPMU bring-up. */
		if (str_has(p->cmdline, "shimcp=1")) {
			(void)shim_gpu_cp(payload, adt_pa);
		}
	}
}

/* ---- Shim-stage CP producer (sagit bridge model, identity SMMU) ---- */

#define GPU_BASE	0x05000000u
#define SMMU_BASE	0x05040000u
#define R_CP_ME_CNTL	0x0832u
#define R_CP_PFP_ME	0x0832u
#define R_RB_BASE	0x0800u
#define R_RB_CNTL	0x0802u
#define R_RB_RPTR_ADDR	0x0804u
#define R_RB_RPTR	0x0806u
#define R_RB_WPTR	0x0807u
#define R_ME_BASE	0x0838u
#define R_PFP_BASE	0x0835u
#define R_CTX_SWITCH	0x083bu
#define R_INT_MASK	0x0038u
#define R_INT_CLEAR	0x0037u

static uint32_t cp_ring[4096] __attribute__((aligned(4096)));
static uint32_t cp_scratch[1024] __attribute__((aligned(4096)));
static uint8_t cp_pm4[0x8000] __attribute__((aligned(4096)));
static uint8_t cp_pfp[0x8000] __attribute__((aligned(4096)));
static uint64_t cp_l1[512] __attribute__((aligned(4096)));
static uint64_t cp_l2[512] __attribute__((aligned(4096)));

static void dc_clean_range(uintptr_t s, uintptr_t e)
{
	uint64_t line = 4UL << ((rsr_ctr() >> 16) & 0xf);
	for (uintptr_t a = s & ~(line - 1); a < e; a += line)
		__asm__ volatile("dc cvac, %0" : : "r"(a) : "memory");
	__asm__ volatile("dsb sy" ::: "memory");
}

static void dc_inval_range(uintptr_t s, uintptr_t e)
{
	uint64_t line = 4UL << ((rsr_ctr() >> 16) & 0xf);
	for (uintptr_t a = s & ~(line - 1); a < e; a += line)
		__asm__ volatile("dc ivac, %0" : : "r"(a) : "memory");
	__asm__ volatile("dsb sy" ::: "memory");
}

static void gpu_wr(uint32_t reg, uint32_t v) { wr32(GPU_BASE + reg * 4u, v); }
static uint32_t gpu_rd(uint32_t reg) { return rd32(GPU_BASE + reg * 4u); }

static uint32_t cp_pkt7(uint32_t op, uint32_t cnt)
{
	/* A5xx type-7 header (odd parity on count and opcode). */
	uint32_t p;
	p = cnt; p ^= p >> 4; p ^= p >> 8; p ^= p >> 16; p = (~p) & 1u;
	uint32_t q = op; q ^= q >> 4; q ^= q >> 8; q ^= q >> 16; q = (~q) & 1u;
	return (7u << 28) | cnt | (p << 15) | (op << 16) | (q << 23);
}

static int shim_gpu_smmu_identity(void)
{
	const uintptr_t sm = SMMU_BASE;
	uintptr_t cb = sm + 0x8000u;
	uintptr_t gr1 = sm + 0x1000u;
	uint64_t root_pa = (uint64_t)cp_l1;
	uint32_t i;

	/* 1 GiB of 2 MiB identity blocks: 0x80000000..0xC0000000. */
	for (i = 0; i < 512; i++)
		cp_l2[i] = (0x80000000ull + (uint64_t)i * 0x200000ull) |
		    3ull | (2ull << 8) | (1ull << 10);
	for (i = 0; i < 512; i++)
		cp_l1[i] = 0;
	cp_l1[2] = (uint64_t)cp_l2 | 3ull;	/* GB2 = 0x80000000 */
	dc_clean_range((uintptr_t)cp_l1, (uintptr_t)cp_l1 + 4096);
	dc_clean_range((uintptr_t)cp_l2, (uintptr_t)cp_l2 + 4096);

	wr32(sm + 0x0, 0);			/* keep global CNTL */
	wr32(gr1 + 0x0, 1u << 16);		/* CBAR0: s1 tr, s2 bypass */
	wr32(gr1 + 0x800, 1u);			/* CBA2R0 */
	wr32(cb + 0x10, (7u << 15) | 1u);	/* TTBCR2 */
	wr32(cb + 0x30, 28u | (1u << 23));	/* TCR: 36-bit */
	wr32(cb + 0x38, 0x44u);
	wr32(cb + 0x20, (uint32_t)root_pa);
	wr32(cb + 0x24, (uint32_t)(root_pa >> 32));
	wr32(cb + 0x28, 0);
	wr32(cb + 0x2c, 0);
	wr32(cb + 0x58, 0xffffffffu);
	wr32(cb + 0x0, (1u << 5) | (1u << 2) | (1u << 1) | 1u);
	wr32(sm + 0x800, 1u << 31);		/* SMR0 valid, SID 0 */
	wr32(sm + 0xc00, 0u);			/* S2CR0 translate */
	wr32(sm + 0x68, 0);
	wr32(sm + 0x70, 0);
	for (i = 0; i < 2000; i++)
		if (!(rd32(sm + 0x74) & 1u))
			break;
	return (rd32(cb + 0x0) & 1u) && (rd32(sm + 0x800) == 0x80000000u);
}

/* Vendor startup.reset(): only legal with CP halted and VBIF drained, which
 * the caller proves first. Clears the CP core state TZ left behind and
 * resets GPMU CM3, then validates an empty ring and trust<=1. */
static int shim_gpu_reset(void)
{
	uint32_t i, pre;
	if ((gpu_rd(R_CP_ME_CNTL) & 0x303u) != 0x303u ||
	    (gpu_rd(0x3081u) & 15u) != 15u)
		return 0;
	gpu_wr(0x0043u, 1u);		/* RBBM_SW_RESET_CMD */
	(void)gpu_rd(0x0043u);
	gpu_wr(0x0043u, 0u);
	gpu_wr(R_INT_MASK, 0u);
	gpu_wr(R_CP_ME_CNTL, 0x303u);
	gpu_wr(0x3080u, 15u);
	for (i = 0; i < 2000; i++) {
		if ((gpu_rd(R_CP_ME_CNTL) & 0x303u) == 0x303u &&
		    (gpu_rd(0x3081u) & 15u) == 15u)
			break;
		gpu_delay_ms(1);
	}
	pre = gpu_rd(0x0043u);
	if (pre || gpu_rd(R_RB_RPTR) || gpu_rd(R_RB_WPTR) ||
	    gpu_rd(R_CTX_SWITCH) || gpu_rd(0xf400u) > 1u)
		return 0;
	gpu_wr(0xa8d8u, 1u);		/* GPMU_CM3_SYSRESET */
	gpu_wr(0xa8e0u, 0u);
	gpu_wr(0xa8e1u, 0u);
	return gpu_rd(0xa8d8u) == 1u && !gpu_rd(0xa8e0u) && !gpu_rd(0xa8e1u);
}

static int shim_gpu_gpmu(uintptr_t adt_pa)
{
	uint64_t pa, len;
	const uint32_t *g;
	uint32_t remaining, metadata, first, instr, i, msg, payload;
	if (!adt_mmap_entry(adt_pa, "GPU-GPMU", &pa, &len) || len < 64)
		return 0;
	g = (const uint32_t *)pa;
	remaining = g[0]; metadata = g[2];
	if (g[1] != 2 || remaining < 3 || remaining >= len / 4 ||
	    metadata > remaining - 3)
		return 0;
	first = metadata + 3;
	instr = remaining - metadata - 2;
	gpu_wr(0xa8d8u, 1u);
	gpu_wr(0xa8e0u, 0u);
	gpu_wr(0xa8e1u, 0u);
	for (i = 0; i < instr; i++)
		gpu_wr(0x8800u + i, g[first + i]);
	con_hex2("GPU GPMU", "instr=", instr, "w0=", gpu_rd(0x8800u));

	msg = 0x9807u; payload = msg + 5u;
	gpu_wr(0xac60u, 0x80000000u);
	gpu_wr(0xac80u, 0x80000000u | 6000u);
	for (i = 0; i < 36; i++)
		gpu_wr(payload + i, 0u);
	gpu_wr(msg, 0x80000001u);
	gpu_wr(msg + 1u, 1u);
	gpu_wr(payload, 5448u);
	gpu_wr(payload + 1u, 1u);
	gpu_wr(payload + 2u, 800u);
	gpu_wr(payload + 3u, 180u);
	gpu_wr(payload + 34u, (1u << 24) | (1u << 17) | (2u << 8));
	gpu_wr(payload + 35u, ~0x303u);
	gpu_wr(msg + 3u, 36u * 4u);
	gpu_wr(0x9805u, 0xbabefaceu);
	gpu_wr(0xa892u, 0x7fu);
	gpu_wr(0xa894u, 0u);
	gpu_wr(0xa893u, 0xa0080u);
	gpu_wr(0xa891u, 0x600040u);
	gpu_wr(0xa8d8u, 0u);
	for (i = 0; i < 1000; i++) {
		if (gpu_rd(0xa8e0u) == 0xbabefaceu)
			return 1;
		gpu_delay_ms(1);
	}
	return 0;
}

static int shim_gpu_cp(const void *payload, uintptr_t adt_pa)
{
	uint64_t mdt_pa, mdt_len, seg_pa, seg_len;
	const struct payload_hdr *p = payload;
	const uint8_t *m;
	uint32_t phoff, pm4_bytes, pfp_bytes, i;
	uint64_t ring_pa = (uint64_t)cp_ring;
	uint64_t scr_pa = (uint64_t)cp_scratch;
	int mi = 0, stop = 0;

	if (!adt_mmap_entry(adt_pa, "GPU-PM4", &mdt_pa, &mdt_len) ||
	    !adt_mmap_entry(adt_pa, "GPU-PFP", &seg_pa, &seg_len)) {
		con_str("GPU CP", "fw blobs missing");
		return 0;
	}
	(void)p;
	/* Header word 0 is not CP microcode (adreno_fw_create_bo). */
	pm4_bytes = (uint32_t)mdt_len - 4u;
	pfp_bytes = (uint32_t)seg_len - 4u;
	if (pm4_bytes > sizeof(cp_pm4) || pfp_bytes > sizeof(cp_pfp)) {
		con_str("GPU CP", "fw too large");
		return 0;
	}
	for (i = 0; i < pm4_bytes; i++)
		cp_pm4[i] = ((const uint8_t *)mdt_pa)[4 + i];
	for (i = 0; i < pfp_bytes; i++)
		cp_pfp[i] = ((const uint8_t *)seg_pa)[4 + i];

	con_hex2("GPU CP", "pm4=", (uint32_t)(uintptr_t)cp_pm4,
		 "pfp=", (uint32_t)(uintptr_t)cp_pfp);
	con_hex2("GPU CP", "pm4b=", pm4_bytes, "pfpb=", pfp_bytes);

	if (str_has(p->cmdline, "shimtsb=1")) {
		/* Vendor hw_init zeroes the trusted stream buffer. */
		gpu_wr(0xf803u, 0u);
		gpu_wr(0xf800u, 0u);
		gpu_wr(0xf801u, 0u);
		gpu_wr(0xf802u, 0u);
		gpu_wr(0xf810u, 1u);
		con_hex2("GPU TSB", "base=", gpu_rd(0xf800u),
			 "size=", gpu_rd(0xf802u));
	}
	if (str_has(p->cmdline, "shimcfg=1")) {
		/* Startup::configure subset that gates CP decode. */
		gpu_wr(0x0860u, 1u);		/* CP_ADDR_MODE_CNTL */
		gpu_wr(0x08a0u, 7u);		/* CP_PROTECT_CNTL */
		gpu_wr(0x0825u, 0x40u);		/* MEQ thresholds */
		gpu_wr(0x0826u, 0x400u);	/* MERCIU */
		gpu_wr(0x0820u, 0x80000060u);
		gpu_wr(0x081fu, 0x40201b16u);
		gpu_wr(0x0833u, 0x02000000u);	/* CP_CHICKEN_DBG */
		gpu_wr(0x0093u, 1u);		/* RBBM_AHB_CNTL0 */
		gpu_wr(0x0094u, 0xa6ffffffu);
		con_hex2("GPU CFG", "amb=", gpu_rd(0x0860u),
			 "prot=", gpu_rd(0x08a0u));
	}
	if (str_has(p->cmdline, "shimgpmu=1")) {
		int gr = shim_gpu_gpmu(adt_pa);
		con_hex2("GPU GPMU", "ready=", (uint32_t)gr,
			 "rdy=", gpu_rd(0xa8e0u));
	}
	if (!shim_gpu_smmu_identity()) {
		con_str("GPU CP", "smmu identity failed");
		return 0;
	}
	if (str_has(p->cmdline, "shimrst=1")) {
		int rr;
		con_str("GPU CP", "vendor reset requested");
		rr = shim_gpu_reset();
		con_hex2("GPU RST", "ok=", (uint32_t)rr,
			 "me=", gpu_rd(R_CP_ME_CNTL));
		if (!rr)
			return 0;
	}
	con_str("GPU CP", "smmu identity ok");

	/* Halt CP + drain VBIF before touching its registers. */
	gpu_wr(R_INT_MASK, 0);
	gpu_wr(R_CP_ME_CNTL, 0x303u);
	gpu_wr(0x3080u, 15u);
	for (i = 0; i < 2000; i++) {
		if ((gpu_rd(R_CP_ME_CNTL) & 0x303u) == 0x303u &&
		    (gpu_rd(0x3081u) & 15u) == 15u)
			break;
		gpu_delay_ms(1);
	}
	con_hex2("GPU CP", "halt_me=", gpu_rd(R_CP_ME_CNTL),
		 "vbif=", gpu_rd(0x3081u));
	if ((gpu_rd(R_CP_ME_CNTL) & 0x303u) != 0x303u) {
		con_str("GPU CP", "halt failed");
		return 0;
	}

	/* Microcode through the AHB RAM ports (kgsl a5xx path). */
	for (i = 0; i < pm4_bytes / 4u; i++) {
		gpu_wr(0x082fu, i);
		gpu_wr(0x0830u, ((const uint32_t *)cp_pm4)[i]);
	}
	for (i = 0; i < pfp_bytes / 4u; i++) {
		gpu_wr(0x082au, i);
		gpu_wr(0x082bu, ((const uint32_t *)cp_pfp)[i]);
	}
	gpu_wr(0x082fu, 0);
	gpu_wr(0x082au, 0);
	con_hex2("GPU CP", "uc_me=", gpu_rd(0x0830u), "uc_pfp=", gpu_rd(0x082bu));

	/* Program instruction bases (identity PA), ring and RPTR shadow. */
	wr32(GPU_BASE + R_PFP_BASE * 4u, (uint32_t)(uintptr_t)cp_pfp);
	wr32(GPU_BASE + (R_PFP_BASE + 1u) * 4u, 0);
	wr32(GPU_BASE + R_ME_BASE * 4u, (uint32_t)(uintptr_t)cp_pm4);
	wr32(GPU_BASE + (R_ME_BASE + 1u) * 4u, 0);
	wr32(GPU_BASE + R_RB_BASE * 4u, (uint32_t)ring_pa);
	wr32(GPU_BASE + (R_RB_BASE + 1u) * 4u, 0);
	wr32(GPU_BASE + R_RB_RPTR_ADDR * 4u, (uint32_t)(scr_pa + 0x10u));
	wr32(GPU_BASE + (R_RB_RPTR_ADDR + 1u) * 4u, 0);
	wr32(GPU_BASE + R_CTX_SWITCH * 4u, 0);
	wr32(GPU_BASE + R_RB_CNTL * 4u, 11u | (2u << 8) | (1u << 27));
	wr32(GPU_BASE + R_RB_WPTR * 4u, 0);
	gpu_wr(R_INT_MASK, 0);
	gpu_wr(R_INT_CLEAR, 0xffffffffu);

	/* One ME_INIT packet, exactly the payload sagit/AdrenoStartup uses. */
	for (i = 0; i < 4096; i++)
		cp_ring[i] = 0;
	cp_ring[0] = cp_pkt7(0x48u, 8u);
	cp_ring[1] = 0x2fu;
	cp_ring[2] = 3u;
	cp_ring[3] = 0x20000000u;
	for (i = 4; i < 9; i++)
		cp_ring[i] = 0;
	dc_clean_range((uintptr_t)cp_ring, (uintptr_t)cp_ring + 4096);
	dc_clean_range((uintptr_t)cp_pm4, (uintptr_t)cp_pm4 + sizeof(cp_pm4));
	dc_clean_range((uintptr_t)cp_pfp, (uintptr_t)cp_pfp + sizeof(cp_pfp));

	wr32(GPU_BASE + R_RB_WPTR * 4u, 9u);
	wr32(GPU_BASE + 0x3080u * 4u, 0u);	/* VBIF unhalt */
	wr32(GPU_BASE + R_CP_ME_CNTL * 4u, 0u);	/* release ME */
	for (i = 0; i < 2000; i++) {
		if (gpu_rd(R_RB_RPTR) == 9u) { mi = 1; break; }
		gpu_delay_ms(1);
	}
	dc_inval_range((uintptr_t)cp_scratch, (uintptr_t)cp_scratch + 256);
	con_hex2("GPU CP", "rptr=", gpu_rd(R_RB_RPTR),
		 "mi=", (uint32_t)mi);
	con_hex2("GPU CP", "me=", gpu_rd(R_CP_ME_CNTL),
		 "scr=", cp_scratch[0]);

	/* Leave the CP halted and drained for XNU. */
	gpu_wr(R_INT_MASK, 0);
	gpu_wr(R_CP_ME_CNTL, 0x303u);
	gpu_wr(0x3080u, 15u);
	for (i = 0; i < 2000; i++) {
		if ((gpu_rd(R_CP_ME_CNTL) & 0x303u) == 0x303u &&
		    (gpu_rd(0x3081u) & 15u) == 15u) { stop = 1; break; }
		gpu_delay_ms(1);
	}
	con_hex2("GPU CP", "stopped=", (uint32_t)stop,
		 "final_me=", gpu_rd(R_CP_ME_CNTL));
	return mi;
}
