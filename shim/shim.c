/*
 * Milestone 2: one boot answers the questions that decide the XNU port.
 *   - CPU feature registers (16K granule, GIC sysreg interface)
 *   - PSCI/SMCCC version and which PSCI calls the Qualcomm TZ offers
 *   - SMP: PSCI CPU_ON for the 7 secondaries, each reports its MPIDR
 *   - GICv3 Group 1 IRQ from the virtual timer (INTID 19 on Qualcomm)
 * Any unexpected exception paints a blue panic screen.
 */
#include <stdint.h>
#include "console.h"
#include "mmu.h"
#include "loader.h"
#include "ufs.h"
#include "wdt.h"

#define IMEM_RESTART_REASON	0x146bf65cUL	/* qcom,msm-imem restart_reason@65c */
#define RESTART_BOOTLOADER	0x77665500U
#define PSHOLD			0x10ac000UL
#define APPSLOG_LBA		208260U	/* sda58: 512B sector 1666080 -> 4K block */

#define GICD_BASE		0x17a00000UL
#define GICR_BASE		0x17b00000UL
#define GICR_SIZE		0x100000UL
#define GICR_PE_SIZE		0x20000UL
#define GICR_TYPER		0x08
#define GICR_WAKER		0x14
#define GICR_SGI		0x10000
#define GICR_IGROUPR0		(GICR_SGI + 0x080)
#define GICR_ISENABLER0		(GICR_SGI + 0x100)
#define GICR_ICENABLER0		(GICR_SGI + 0x180)
#define GICR_IPRIORITYR		(GICR_SGI + 0x400)
#define GICR_ISPENDR0		(GICR_SGI + 0x200)
#define GICR_ISACTIVER0		(GICR_SGI + 0x300)
#define GICR_ICFGR1		(GICR_SGI + 0xc04)
#define VTIMER_INTID		19	/* DT: <GIC_PPI 3 ...>, not the usual 27 */
#define PTIMER_INTID		18	/* DT: <GIC_PPI 2 ...> */
#define SGI_INTID		1

#define PSCI_VERSION		0x84000000UL
#define PSCI_CPU_SUSPEND	0xc4000001UL
#define PSCI_CPU_OFF		0x84000002UL
#define PSCI_CPU_ON		0xc4000003UL
#define PSCI_MIGRATE_INFO_TYPE	0x84000006UL
#define PSCI_SYSTEM_RESET	0x84000009UL
#define PSCI_FEATURES		0x8400000aUL
#define PSCI_SYSTEM_SUSPEND	0xc400000eUL
#define PSCI_SYSTEM_RESET2	0xc4000012UL
#define SMCCC_VERSION		0x80000000UL

#define NR_CPUS			8

#define rsr(r) ({ uint64_t _v; __asm__ volatile("mrs %0, " r : "=r"(_v)); _v; })
#define wsr(r, v) __asm__ volatile("msr " r ", %0" : : "r"((uint64_t)(v)))
#define isb() __asm__ volatile("isb" ::: "memory")

static inline uint32_t rd32(uintptr_t a) { return *(volatile uint32_t *)a; }
static inline uint64_t rd64(uintptr_t a) { return *(volatile uint64_t *)a; }
static inline void wr32(uintptr_t a, uint32_t v) { *(volatile uint32_t *)a = v; }
static inline void wr8(uintptr_t a, uint8_t v) { *(volatile uint8_t *)a = v; }

extern char secondary_entry[];

static const char *stage;	/* set at runtime: no absolute pointers in data */
static volatile uint64_t sec_mpidr[NR_CPUS];
static volatile uint64_t sec_probe[4];	/* CPU1 redistributor write test */
static volatile uint32_t irq_count, irq_other, irq_last;
static volatile int irq_src;

/* ICC_SGI1R_EL1 value targeting the calling core only. */
static uint64_t sgi_self(unsigned id)
{
	uint64_t m = rsr("mpidr_el1");
	return ((m >> 16) & 0xff) << 32 |	/* Aff2 */
	       ((m >> 8) & 0xff) << 16 |	/* Aff1 */
	       (uint64_t)id << 24 |
	       (1U << (m & 0xf));		/* target list: Aff0 */
}

static uint64_t smc(uint64_t fn, uint64_t a1, uint64_t a2, uint64_t a3)
{
	register uint64_t x0 __asm__("x0") = fn;
	register uint64_t x1 __asm__("x1") = a1;
	register uint64_t x2 __asm__("x2") = a2;
	register uint64_t x3 __asm__("x3") = a3;
	__asm__ volatile("smc #0"
			 : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3)
			 :
			 : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
			   "x12", "x13", "x14", "x15", "x16", "x17", "memory");
	return x0;
}

static uint64_t now(void)
{
	isb();
	return rsr("cntpct_el0");
}

static void delay_ms(unsigned ms)
{
	uint64_t f = rsr("cntfrq_el0");
	if (!f)
		f = 19200000;	/* ABL may leave CNTFRQ unset — QC timer is always 19.2 MHz */
	uint64_t end = now() + f / 1000 * ms;
	while (now() < end)
		;
}

static void __attribute__((noreturn)) reset(unsigned after_s)
{
	con_skip();
	con_str("RESET", after_s > 20 ? "in 40s" : "in 20s");
	delay_ms(after_s * 1000);
	wr32(IMEM_RESTART_REASON, RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
}

/* ---- exceptions ---- */

static void irq_handler(void)
{
	uint64_t iar = rsr("icc_iar1_el1");
	uint32_t id = iar & 0xffffff;

	if (id >= 1020 && id <= 1023)
		return;	/* spurious */
	irq_last = id;
	if (id == (uint32_t)irq_src)
		irq_count++;
	else
		irq_other++;
	if (id == VTIMER_INTID)
		wsr("cntv_tval_el0", rsr("cntfrq_el0") / 100);	/* 10 ms */
	else if (id == PTIMER_INTID)
		wsr("cntp_tval_el0", rsr("cntfrq_el0") / 100);
	wsr("icc_eoir1_el1", iar);
	isb();
}

void exc_handler(uint64_t slot, uint64_t *regs)
{
	if (slot == 5) {	/* IRQ from current EL, SPx */
		irq_handler();
		return;
	}
	con_clear(BLUE);
	switch (slot & 3) {
	case 0: con_str("PANIC", "SYNC"); break;
	case 1: con_str("PANIC", "IRQ"); break;
	case 2: con_str("PANIC", "FIQ"); break;
	default: con_str("PANIC", "SERROR"); break;
	}
	con_hex2("SLOT", "n=", slot, "cpu=", rsr("mpidr_el1"));
	con_str("STAGE", stage);
	con_hex("ESR", rsr("esr_el1"));
	con_hex("ELR", rsr("elr_el1"));
	con_hex("FAR", rsr("far_el1"));
	con_hex("SPSR", rsr("spsr_el1"));
	con_hex("X0", regs[0]);
	con_hex("X1", regs[1]);
	con_hex("X30", regs[29]);	/* stashed lr is just above the frame */
	reset(40);
}

/* ---- SMP ---- */

static uintptr_t find_gicr(void);

void secondary_main(uint64_t idx)
{
	sec_mpidr[idx] = rsr("mpidr_el1") | (1ULL << 63);
	if (idx == 1) {
		uintptr_t r = find_gicr();
		if (r) {
			sec_probe[0] = rd32(r + GICR_IGROUPR0);
			wr32(r + GICR_IGROUPR0, 0xffffffff);
			sec_probe[1] = rd32(r + GICR_IGROUPR0);
			wr8(r + GICR_IPRIORITYR + 19, 0xa0);
			sec_probe[2] = rd32(r + GICR_IPRIORITYR + 16);
		}
		sec_probe[3] = r ? 1 : 0xdead;
	}
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_CPU_OFF, 0, 0, 0);
}

static void __attribute__((unused)) test_smp(void)
{
	stage = "smp";
	uint64_t self = rsr("mpidr_el1") & 0xffffff;
	int64_t ret[NR_CPUS];

	for (int i = 1; i < NR_CPUS; i++)
		ret[i] = smc(PSCI_CPU_ON, (uint64_t)i << 8, (uintptr_t)secondary_entry, i);
	delay_ms(500);
	for (int i = 1; i < NR_CPUS; i++) {
		char label[] = "CPU0";
		label[3] = '0' + i;
		if (((uint64_t)i << 8) == self)
			con_str(label, "is boot cpu");
		else
			con_hex2(label, "ret=", (uint32_t)ret[i], "mpidr=",
				 (uint32_t)sec_mpidr[i] | (sec_mpidr[i] ? 0 : 0xdead0000));
	}
}

/* ---- GIC + timer ---- */

static uintptr_t find_gicr(void)
{
	uint64_t aff = rsr("mpidr_el1");
	uint32_t want = (aff & 0xffffff) | ((aff >> 8) & 0xff000000);

	for (uintptr_t off = 0; off < GICR_SIZE; off += GICR_PE_SIZE) {
		uint64_t typer = rd64(GICR_BASE + off + GICR_TYPER);
		if ((uint32_t)(typer >> 32) == want)
			return GICR_BASE + off;
		if (typer & (1 << 4))	/* Last */
			break;
	}
	return 0;
}

static void gicd_wait_rwp(void)
{
	for (int i = 0; i < 1000000 && (rd32(GICD_BASE) & (1U << 31)); i++)
		;
}

/* Run one interrupt source for ~300 ms and report what the GIC saw. */
static void irq_window(const char *label, uintptr_t r, int src)
{
	uint64_t f = rsr("cntfrq_el0");
	uint32_t pend = 0, act = 0, isr = 0, hppi = 0x3ff, ist = 0;

	irq_count = irq_other = 0;
	irq_last = 0x3ff;
	irq_src = src;
	if (src == VTIMER_INTID) {
		wsr("cntv_tval_el0", f / 100);
		wsr("cntv_ctl_el0", 1);
	} else if (src == PTIMER_INTID) {
		wsr("cntp_tval_el0", f / 100);
		wsr("cntp_ctl_el0", 1);
	}
	isb();
	/* sample state with IRQs still masked, after the first period */
	uint64_t t = now() + f / 50;
	while (now() < t)
		;
	if (src == SGI_INTID)
		wsr("icc_sgi1r_el1", sgi_self(SGI_INTID));
	isb();
	ist = src == PTIMER_INTID ? rsr("cntp_ctl_el0") : rsr("cntv_ctl_el0");
	pend = rd32(r + GICR_ISPENDR0);
	act = rd32(r + GICR_ISACTIVER0);
	isr = rsr("isr_el1");
	hppi = rsr("icc_hppir1_el1");

	__asm__ volatile("msr daifclr, #2" ::: "memory");
	t = now() + f * 3 / 10;
	/* SGI: one already sent above, send 9 more 10 ms apart -> expect 10 */
	uint64_t next = now() + f / 100;
	for (int sent = 1; now() < t;)
		if (src == SGI_INTID && sent < 10 && now() >= next) {
			wsr("icc_sgi1r_el1", sgi_self(SGI_INTID));
			isb();
			sent++;
			next += f / 100;
		}
	__asm__ volatile("msr daifset, #2" ::: "memory");
	wsr("cntv_ctl_el0", 0);
	wsr("cntp_ctl_el0", 0);
	isb();

	con_hex2(label, "irqs=", irq_count, "other=", irq_other);
	con_hex2(" pre", "pend=", pend, "act=", act);
	con_hex2(" pre", "isr=", isr, "hppi=", hppi);
	con_hex2(" pre", "tctl=", ist, "last=", irq_last);
}

static void test_gic(void)
{
	stage = "gic sre";
	uint64_t sre = rsr("icc_sre_el1");
	if (!(sre & 1)) {
		wsr("icc_sre_el1", sre | 1);
		isb();
	}
	if (!(rsr("icc_sre_el1") & 1)) {
		con_str("GIC", "no sysreg iface, skip");
		return;
	}

	stage = "gic dist";
	uint32_t dctlr = rd32(GICD_BASE);
	/* Non-secure view (DS=0): bit0 EnableGrp1, bit1 EnableGrp1A, bit4 ARE_NS; Linux writes 0x13 */
	wr32(GICD_BASE, dctlr | 0x13);
	gicd_wait_rwp();
	con_hex2("GICD", "was=", dctlr, "now=", rd32(GICD_BASE));

	stage = "gic redist";
	uintptr_t r = find_gicr();
	if (!r) {
		con_str("GICR", "not found");
		return;
	}
	uint32_t waker = rd32(r + GICR_WAKER);
	if (waker & 2) {
		wr32(r + GICR_WAKER, waker & ~2U);
		for (int i = 0; i < 100000 && (rd32(r + GICR_WAKER) & 4); i++)
			;
	}
	con_hex2("GICR", "base=", r, "waker=", rd32(r + GICR_WAKER));

	stage = "gic config";
	uint32_t grp = rd32(r + GICR_IGROUPR0);
	wr32(r + GICR_IGROUPR0, 0xffffffff);	/* NS view: secure bits are WI */
	uint32_t en_before = rd32(r + GICR_ISENABLER0);
	uint32_t pri_before = rd32(r + GICR_IPRIORITYR + 16);	/* INTIDs 16..19 */
	for (int id = 0; id < 32; id++)
		wr8(r + GICR_IPRIORITYR + id, 0xa0);
	wr32(r + GICR_ISENABLER0, (1U << VTIMER_INTID) | (1U << PTIMER_INTID) | (1U << SGI_INTID));
	con_hex2("ENABLE", "was=", en_before, "now=", rd32(r + GICR_ISENABLER0));
	(void)grp;
	(void)pri_before;

	uint64_t ctlr = rsr("icc_ctlr_el1");
	wsr("icc_ctlr_el1", ctlr & ~2ULL);	/* EOImode 0: EOIR also deactivates */
	wsr("icc_pmr_el1", 0xf0);
	wsr("icc_bpr1_el1", 0);
	wsr("icc_igrpen1_el1", 1);
	isb();
	con_hex2("ICC", "ctlr=", ctlr, "grpen1=", rsr("icc_igrpen1_el1"));

	stage = "gic vtimer";
	irq_window("VTIMER", r, VTIMER_INTID);
	stage = "gic sgi";
	irq_window("SGI1", r, SGI_INTID);
	wr32(r + GICR_ICENABLER0, 0xffffffff);
	isb();
}

/* ---- main ---- */

static void __attribute__((unused)) psci_line(const char *label, uint64_t fn)
{
	con_hex2(label, "feat=", smc(PSCI_FEATURES, fn, 0, 0), 0, 0);
}

static uint64_t scratch[8192];	/* 64KB, lives in our (later cacheable) image */

static uint64_t ticks_fill(uint32_t c)
{
	uint64_t t = now();
	con_clear(c);
	return now() - t;
}

static uint64_t ticks_mem(void)
{
	uint64_t t = now();
	for (int pass = 0; pass < 16; pass++)
		for (int i = 0; i < 8192; i++)
			scratch[i] = scratch[i] * 3 + i;
	return now() - t;
}

static uint32_t to_us(uint64_t ticks)
{
	return ticks * 10 / 192;	/* 19.2 MHz */
}

static volatile uint32_t *cp_ps = (volatile uint32_t *)0xffcc0000UL;

/* Diag stash: tail of the 36 MB ABL splash region (0x9d400000+32M).
   Plain DDR — survives warm reset, nothing in Android scribbles it,
   readable+writable via /dev/fmem. (XBL IMEM 0x146bf000 reads back
   scrambled — TZ-secured; kernel's own restart_reason page at
   0xffbc7000 is exclusive-reserved so memremap writes bounce.) */
#define IMEM_DIAG	0x9f400000UL
void imem_diag(const char *tag, uint32_t a, uint32_t b)
{
	static volatile uint32_t *d = (volatile uint32_t *)IMEM_DIAG;
	d[0] = 0x44494147;	/* 'DIAG' */
	d[1] = a;
	d[2] = b;
	for (int i = 0; tag[i] && i < 20; i++)
		((volatile char *)d)[12 + i] = tag[i];
	d += 8;			/* 32 bytes per record */
}

static void cp_mark(char c0, char c1)
{
	volatile char *d = (volatile char *)(0xffcc0000UL + 0xc);
	static int pos;
	cp_ps[0] = 0x43474244;
	d[pos++] = c0; d[pos++] = c1; d[pos++] = 0x0a;
	cp_ps[1] = 0; cp_ps[2] = pos;
	/* Mirror the checkpoint into IMEM restart_reason: after any reset the
	   next Android cmdline shows warmboot=0x7766<c0><c1> — the last
	   checkpoint reached even if the shim dies before finishing. */
	wr32(IMEM_RESTART_REASON, 0x77660000u | ((uint32_t)c0 << 8) | (uint32_t)c1);
}

void shim_main(uint64_t dtb)
{
	imem_diag("entry", 0x5149, (uint32_t)dtb);
	cp_mark('C','0');
	/* Dead-man switch: any hang bites within ~2 min and lands back in
	   fastboot (observed: everything resets to fastboot after fastboot boot). */
	wdt_arm(120, 125);
	wdt_pet();
#ifdef SHIM_PING
	/* Proof-of-life with a delay so timing distinguishes "ran to end"
	   from "never jumped in": 20 s delay then reset to fastboot. */
	imem_diag("ping", 0xdead, 0xbeef);
	for (int i = 0; i < 20; i++) { delay_ms(1000); wdt_pet(); }
	wr32(IMEM_RESTART_REASON, 0x77665501u);
	wr32(PSHOLD, 0);
	for (;;)
		;
#endif
	struct mmu_info mi;

	/* An xnu payload in the ramdisk means: boot it (MMU stays off). */
	stage = "loader";
	cp_mark('C','1');
	con_clear(RED);
	con_str("SHIM", "sweet milestone 4: xnu loader");
	con_skip();
	cp_mark('C','2');
	const char *why = loader_boot(dtb);
	cp_mark('C','3');
	con_str("LOADER", why);
	con_skip();

	/* No payload: probe the internal UFS while the MMU is still off. */
	con_hex("MMFR0", rsr("id_aa64mmfr0_el1"));
	con_hex("MIDR", rsr("midr_el1"));
	con_hex("MPIDR", rsr("mpidr_el1"));
	con_skip();
	stage = "ufs";
	cp_mark('C','4');
	int ustage = test_ufs();
	cp_mark('C','5');
	con_skip();
	/* Keep the probe results on screen (later tests clear it); reset after 3 min. */
	con_str("LOG", "flush to appslog");
	uint32_t wrc = 0x1f, src = 0xf, mApre = 0, wrc2 = 0xf;
	{
		/* Pre-write baseline: does the shim see Android's 'AAAA'+marker
		   at APPSLOG_LBA BEFORE writing? If not, our LBA view differs
		   from Android's entirely. */
		static uint8_t pre[4096] __attribute__((aligned(4096)));
		uint32_t prc = ufs_read_blocks(APPSLOG_LBA, 1, pre);
		mApre = !prc && pre[0] == 'A' && pre[2048] == 'X';
		uint32_t ln;
		char *lb = con_log(&ln);
		uint32_t nblk = (ln + 4095) / 4096;
		wrc = ufs_write_blocks(APPSLOG_LBA, nblk, lb);
		/* Control write at +1 MiB — same spot where an Android dd write
		   survived a hard PSHOLD. Separates "write path broken" from
		   "offset-0 region special". */
		wrc2 = ufs_write_blocks(APPSLOG_LBA + 256, 1, "SHIMDEEPMARKER77");
		src = ufs_sync_cache();
		con_hex2("LOG WR", "rc=", wrc, "sync=", src);
	}
	/* Telemetry: encode stage+rc into restart_reason, read as warmboot= next boot */
	cp_mark('C','6');
	con_hex2("TELE", "ustage=", ustage, "wrc=", wrc);
	/* ABL-independent diag: IMEM survives, /dev/fmem reads it back. */
	imem_diag("ufs-stage", ustage, wrc);
	wdt_arm(300, 305);	/* timing run can legitimately take minutes */
	/* Read-back forensics: does OUR write land at APPSLOG_LBA, at a
	   shifted LBA, or nowhere? And does the shim see Android's marker?
	   cand[] probes: exact, +1, -8, +4 blocks and the 512B-scaled LBA. */
	uint32_t found = 7, mA = 0, rrc = 9;
	{
		static uint8_t rbuf[4096] __attribute__((aligned(4096)));
		static const uint32_t cand[5] = { APPSLOG_LBA, APPSLOG_LBA + 1,
			APPSLOG_LBA - 8, APPSLOG_LBA + 4, 1666080 };
		for (int i = 0; i < 5; i++) {
			uint32_t rc = ufs_read_blocks(cand[i], 1, rbuf);
			if (!rc && rbuf[0] == 'S' && rbuf[1] == 'H') { found = i; break; }
		}
		rrc = ufs_read_blocks(APPSLOG_LBA, 1, rbuf);
		mA = !rrc && rbuf[0] == 'A' && rbuf[2048] == 'X';
		imem_diag("readback", rrc << 8 | mA,
			  rbuf[0] << 24 | rbuf[1] << 16 | rbuf[2] << 8 | rbuf[3]);
	}
	/* Long powered settle — the device commits its cache in background;
	   the surviving Android marker had ~20 s idle before its PSHOLD.
	   No hibernate: keep the link alive so the device can drain. */
	for (int i = 0; i < 20; i++) { delay_ms(3000); wdt_pet(); }
	/* pstore console record at 0xffcc0000: sig/start/size/data */
	{
		volatile uint32_t *prz = (volatile uint32_t *)0xffcc0000UL;
		uint32_t ln;
		char *lb = con_log(&ln);
		volatile char *dst = (volatile char *)(prz + 3);
		for (uint32_t i = 0; i < ln; i++)
			dst[i] = lb[i];
		prz[2] = ln;			/* size */
		prz[1] = 0;			/* start */
		prz[0] = 0x43474244;		/* DBGC */
	}
	/* Timing telemetry: DDR doesn't survive ANY reset on this device and
	   ABL swallows arbitrary restart_reason values, so encode the result
	   in wall time — delay = (code+2) * 2 s, host measures time-to-
	   fastboot and decodes. Layout:
	     ustage!=0      -> code = 256 + ustage
	     ustage==0      -> code = mApre*64 + wrc2*8 + wrc*4 + fbit*2 + mA
	   fbit: 0 = 'SH' seen at APPSLOG_LBA post-write.
	   mApre: 'AAAA'+marker seen at APPSLOG_LBA PRE-write. */
	{
		uint32_t fbit = (found != 0);
		uint32_t code = ustage ? 256 + ustage
			: mApre * 64 + (wrc2 & 0xf) * 8 + (wrc & 0xf) * 4
				+ fbit * 2 + mA;
		for (uint32_t i = 0; i < code + 2; i++) {
			delay_ms(2000);
			wdt_pet();
		}
	}
	wr32(IMEM_RESTART_REASON, 0x77665501u);
	wr32(PSHOLD, 0);
	for (;;)
		;

	stage = "fill off";
	uint64_t fill_off = ticks_fill(RED);
	uint64_t mem_off = ticks_mem();

	stage = "mmu enable";
	int rc = mmu_enable(&mi);

	stage = "fill on";
	uint64_t fill_on = rc ? 0 : ticks_fill(RED);
	uint64_t mem_on = rc ? 0 : ticks_mem();
	if (rc)
		con_clear(RED);

	con_str("SHIM", "sweet milestone 3");
	con_skip();
	con_hex2("DTB", "at=", dtb, "magic=", dtb ? __builtin_bswap32(rd32(dtb)) : 0);
	con_hex2("IMAGE", "lo=", mi.img_lo, "hi=", mi.img_hi);
	con_hex2("MMU", "rc=", rc, "l3=", mi.n_l3);
	con_hex2("TTBR0", "pa=", mi.ttbr, "pa_hi=", mi.ttbr >> 32);
	con_hex("SCTLR0", mi.sctlr_before);
	con_hex("SCTLR1", mi.sctlr_after);
	con_hex("TCR", rsr("tcr_el1"));
	con_hex2("PARANGE", "mmfr0=", rsr("id_aa64mmfr0_el1") & 0xf, "tg16=", (rsr("id_aa64mmfr0_el1") >> 20) & 0xf);
	con_skip();
	con_hex("PAR img", mmu_probe((uintptr_t)shim_main));
	con_hex("PAR fb", mmu_probe(FB_PHYS + 0x1000));
	con_hex("PAR gic", mmu_probe(GICD_BASE));
	con_hex("PAR 12G", mmu_probe(0x300000000UL));
	con_skip();
	con_hex2("FILL us", "off=", to_us(fill_off), "on=", to_us(fill_on));
	con_hex2("MEM us", "off=", to_us(mem_off), "on=", to_us(mem_on));
	con_skip();

	stage = "psci";
	con_hex2("PSCI", "ver=", smc(PSCI_VERSION, 0, 0, 0), "smccc=", smc(SMCCC_VERSION, 0, 0, 0));
	con_skip();
	test_gic();

	stage = "done";
	reset(40);
}
