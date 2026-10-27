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
int spmi_find_apid(uint16_t ppid);
int spmi_read(uint16_t ppid, uint8_t sid, uint16_t addr);
int spmi_write(uint16_t ppid, uint8_t sid, uint16_t addr, uint8_t val);
int spmi_write_apid(int apid, uint16_t addr, uint8_t val);

/* Real XBL reset-reason channel (from P_114/O82 XResetReasonDxe disasm):
 * reason word @0x146bf764 is only honored while the cookie
 * @0x146bfb14 == 0xDEADD00D; otherwise XBL falls back to the GCC
 * reset register (the 0x52d0-family values). The old @0x146bf65c
 * cell is kernel-side only and never reaches warmboot=. */
#define IMEM_RESTART_REASON	0x146bf764UL
#define IMEM_RR_COOKIE		0x146bfb14UL
#define RR_COOKIE_MAGIC		0xdeadd00dU
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
/* Write the XBL reset reason: field first, then the validity cookie.
 * Honored by XBL next boot → routed + echoed as warmboot= in cmdline. */
static inline void rr_route(uint32_t v) {
	wr32(IMEM_RESTART_REASON, v);
	wr32(IMEM_RR_COOKIE, RR_COOKIE_MAGIC);
	__asm__ volatile("dsb sy" ::: "memory");
}
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

static uint64_t smc5(uint64_t fn, uint64_t a1, uint64_t a2, uint64_t a3,
		     uint64_t a4, uint64_t a5)
{
	register uint64_t x0 __asm__("x0") = fn;
	register uint64_t x1 __asm__("x1") = a1;
	register uint64_t x2 __asm__("x2") = a2;
	register uint64_t x3 __asm__("x3") = a3;
	register uint64_t x4 __asm__("x4") = a4;
	register uint64_t x5 __asm__("x5") = a5;
	register uint64_t x6 __asm__("x6") = 0;
	__asm__ volatile("smc #0"
			 : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
			   "+r"(x4), "+r"(x5), "+r"(x6)
			 :
			 : "x7", "x8", "x9", "x10", "x11",
			   "x12", "x13", "x14", "x15", "x16", "x17", "memory");
	return x0;
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
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	/* SYSTEM_RESET returned: TZ didn't take it. NEVER write PSHOLD
	   here — on lilac PSHOLD=0 powers the phone OFF (charge icon)
	   and wipes IMEM+DRAM evidence. Fall back to the hardware WDT
	   bite: guaranteed warm reset, no TZ cooperation needed. */
	wdt_arm(2, 3);
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
	rr_route(0x77660000u | ((uint32_t)c0 << 8) | (uint32_t)c1);
}

void shim_main(uint64_t dtb)
{
#ifdef SHIM_BEACON
	/* Durable channel: PMIC PON spare bytes. They live on the PMIC, which
	   stays powered as long as the battery is connected, so they survive
	   the PMIC-level power cycle that wipes IMEM/DDR. Two PMICs, two
	   independent bytes: SID2 = PMI8998 PON (ppid 0x0208), SID0 = PM8998
	   (ppid 0x0008). PON base register 0x800 -> spare bytes 0x88e/0x88f.
	   SID0 0x88e[6:0] is a status/last-stage byte; SID2 0x88e is magic,
	   SID2 0x88f is a 1 Hz heartbeat counter. */
	(void)dtb;
#if SHIM_BEACON == 56
	/* Wire the IMEM restart_reason field into a 16-bit telemetry channel:
	   ABL forwards any value to the next cmdline as warmboot= and logs
	   "[UNKNOWN TO XBOOT (0x...)]"; only 0x77665500 routes to fastboot.
	   Payload nibbles: [15:12] PON rev read, [11:8] arbiter version high
	   nibble, [7:4] find_apid(0x0008), [3:0] 5 = marker. Failure = 0xF. */
	{
		struct mmu_info mi;
		int apid, rev;
		uint32_t ver;
		(void)mmu_enable(&mi);
		ver = rd32(0x800f000UL);
		apid = spmi_find_apid(0x0008);
		rev = spmi_read(0x0008, 0, 0x01);
		rr_route(0x77660000u |
		     ((uint32_t)(rev < 0 ? 0xf : rev & 0xf) << 12) |
		     (((ver >> 28) & 0xf) << 8) |
		     ((uint32_t)(apid < 0 ? 0xf : apid & 0xf) << 4) | 5u);
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 78
	/* Cover the entire 36 MB splash reservation with RGB565 red and hold
	   for two minutes. Removes every base-offset doubt: if the panel is
	   driven from this region at all it must turn solid red. */
	{
		volatile uint32_t *p = (volatile uint32_t *)FB_BASE;
		for (uint32_t i = 0; i < 0x2400000u / 4; i++)
			p[i] = 0xf800f800u;	/* two RGB565 red pixels */
		delay_ms(120000);
		spmi_write_apid(4, 0x8f, 0x06);
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 77
	/* Which window does the panel actually scan? Fill four 3.6 MB
	   windows starting at 0x9d400000 + n*0x380000 with red, green, blue,
	   white (6 s each), then auto-return to fastboot. */
	{
		static const uint32_t cols[4] = { 0xffff0000u, 0xff00ff00u,
						  0xff0000ffu, 0xffffffffu };
		for (int p = 0; p < 4; p++) {
			volatile uint32_t *fb = (volatile uint32_t *)(FB_BASE + (uint32_t)p * 0x380000u);
			for (uint32_t i = 0; i < 0x380000u / 4; i++)
				fb[i] = cols[p];
			delay_ms(6000);
		}
		spmi_write_apid(4, 0x8f, 0x06);
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 76
	/* Same diagnostics as 74, but with opaque colours and a 25 s hold so
	   the screen can be read directly, then auto-return to fastboot. */
	{
		int us;
		con_clear(0xff101010);
		con_str("SHIM", "beacon76 diag");
		con_hex2("CPU", "EL=", (uint32_t)(rsr("CurrentEL") >> 2) & 3,
			 "SCTLR=", (uint32_t)rsr("sctlr_el1"));
		con_hex("SPMI", "arb_ver=", rd32(0x800f000UL));
		con_hex2("SPMI", "apid0=", (uint32_t)spmi_find_apid(0x0008),
			 "apid2=", (uint32_t)spmi_find_apid(0x0208));
		con_hex2("SPMI", "pon_rev=", (uint32_t)spmi_read(0x0008, 0, 0x01),
			 "pon_88e=", (uint32_t)spmi_read(0x0008, 0, 0x8e));
		con_hex2("WDT", "en=", rd32(0x17817000UL + 0x08),
			 "sts=", rd32(0x17817000UL + 0x0c));
		us = test_ufs();
		con_hex("UFS", "probe=", (uint32_t)us);
		con_str("HOLD", "25s then fastboot");
		delay_ms(25000);
		spmi_write_apid(4, 0x8f, 0x06);
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 75
	/* Framebuffer geometry calibration. Four 10 s phases, each fills the
	   panel white with a different candidate layout. The host watches
	   which phase turns the panel uniformly white:
	     P1 32bpp stride 2880 (720 px)      P2 32bpp stride 5760
	     P3 24bpp stride 2160               P4 RGB565 stride 1440   */
	{
		for (int y = 0; y < 1280; y++) {
			volatile uint32_t *r = (volatile uint32_t *)(FB_BASE + (uint32_t)y * 2880);
			for (int x = 0; x < 720; x++)
				r[x] = 0xffffffff;
		}
		delay_ms(10000);
		for (int y = 0; y < 1280; y++) {
			volatile uint32_t *r = (volatile uint32_t *)(FB_BASE + (uint32_t)y * 5760);
			for (int x = 0; x < 720; x++)
				r[x] = 0xffffffff;
		}
		delay_ms(10000);
		for (int y = 0; y < 1280; y++) {
			volatile uint8_t *r = (volatile uint8_t *)(FB_BASE + (uint32_t)y * 2160);
			for (int x = 0; x < 2160; x++)
				r[x] = 0xff;
		}
		delay_ms(10000);
		for (int y = 0; y < 1280; y++) {
			volatile uint16_t *r = (volatile uint16_t *)(FB_BASE + (uint32_t)y * 1440);
			for (int x = 0; x < 720; x++)
				r[x] = 0xffff;
		}
		delay_ms(10000);
		spmi_write_apid(4, 0x8f, 0x06);
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 74
	/* Diagnostics on a shim proven to run: CPU state, SPMI arbiter,
	   PON reads, APSS WDT, UFS probe. Flushes the console log to appslog
	   through UFS, then routes the next boot to fastboot via the PON
	   reason. Host reads appslog from TWRP/Android. */
	{
		int us;
		uint32_t ln;
		char *lb;
		con_clear(0x00101010);
		con_str("SHIM", "beacon74 diag");
		con_hex2("CPU", "EL=", (uint32_t)(rsr("CurrentEL") >> 2) & 3,
			 "SCTLR=", (uint32_t)rsr("sctlr_el1"));
		con_hex("SPMI", "arb_ver=", rd32(0x800f000UL));
		con_hex2("SPMI", "apid0=", (uint32_t)spmi_find_apid(0x0008),
			 "apid2=", (uint32_t)spmi_find_apid(0x0208));
		con_hex2("SPMI", "pon_rev=", (uint32_t)spmi_read(0x0008, 0, 0x01),
			 "pon_88f=", (uint32_t)spmi_read(0x0008, 0, 0x8f));
		con_hex2("WDT", "en=", rd32(0x17817000UL + 0x08),
			 "sts=", rd32(0x17817000UL + 0x0c));
		us = test_ufs();
		con_hex("UFS", "probe=", (uint32_t)us);
		lb = con_log(&ln);
		if (ufs_write_blocks(APPSLOG_LBA, (ln + 4095) / 4096, lb) == 0) {
			ufs_sync_cache();
			ufs_hibernate();
		}
		spmi_write_apid(4, 0x8f, 0x06);	/* next boot -> fastboot */
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 73
	/* Unattended fastboot loop: same PON write as 72 but with reason
	   BOOTLOADER (3) -> 0x88F = 6. XBL should route the next boot to
	   fastboot, so every later test can run without key presses. */
	{
		int w = spmi_write_apid(4, 0x8f, 0x06);
		(void)w;
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 72
	/* Does the shim's SPMI write reach the PMIC? Write reboot reason
	   RECOVERY (2) to PM8998 PON soft-reboot spare 0x88F bits[7:1]
	   (value = 2<<1 = 4) through apid 4, then reset. XBL maps that to
	   warmboot=0x77665502 in the next Android cmdline; 0x77665501 or
	   0 means the write did not take. */
	{
		int w = spmi_write_apid(4, 0x8f, 0x04);
		(void)w;
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(200);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 70
	/* First diagnostics on a shim that actually executes. Prints to the
	   panel console (720x1280) and then idles so the screen can be read.
	   Order matters: a hang shows the last line it reached. */
	{
		struct mmu_info mi;
		int apid0, apid2, rev, us;
		uint32_t ver, el, sctlr;
		con_clear(0x00101010);
		con_str("SHIM", "beacon70 diagnostics");
		el = (uint32_t)(rsr("CurrentEL") >> 2) & 3;
		sctlr = (uint32_t)rsr("sctlr_el1");
		con_hex2("CPU", "EL=", el, "SCTLR=", sctlr);
		ver = rd32(0x800f000UL);
		con_hex("SPMI", "arb_ver=", ver);
		apid0 = spmi_find_apid(0x0008);
		apid2 = spmi_find_apid(0x0208);
		con_hex2("SPMI", "apid_sid0=", (uint32_t)apid0, "apid_sid2=", (uint32_t)apid2);
		rev = spmi_read(0x0008, 0, 0x01);
		con_hex("SPMI", "pon_rev=", (uint32_t)rev);
		con_hex2("WDT", "en=", rd32(0x17817000UL + 0x08),
			 "sts=", rd32(0x17817000UL + 0x0c));
		us = test_ufs();
		con_hex("UFS", "probe=", (uint32_t)us);
		con_str("HOLD", "alive; power-cycle to exit");
		for (;;)
			delay_ms(1000);
	}
#endif
#if SHIM_BEACON == 66
	/* Execution + survival test, no display, no reset: if the shim runs
	   and the APSS WDT is the killer, petting it keeps the phone alive
	   indefinitely (no charger UI). If it crashes or the killer is not
	   the APSS WDT, the phone power-cycles on the usual schedule. */
	for (;;) {
		wdt_pet();
		delay_ms(10);
	}
#endif
#if SHIM_BEACON == 80
	/* Display-geometry probe v3: the panel consumes RGB888 = 2160 B per
	   line (qcom,mdss-dsi-bpp=24). Top half = 16-row RED/GREEN bands,
	   bottom half = 16-col BLUE/WHITE bars, written byte-by-byte. */
	{
		for (int y = 0; y < 640; y++) {
			volatile uint8_t *ln = con_fb + y * FB_STRIDE;
			uint8_t a = (y / 16) & 1, r = a ? 0 : 0xff, g = a ? 0xff : 0;
			for (int x = 0; x < FB_W; x++) {
				ln[x * 3] = r; ln[x * 3 + 1] = g; ln[x * 3 + 2] = 0;
			}
		}
		for (int y = 640; y < FB_H; y++) {
			volatile uint8_t *ln = con_fb + y * FB_STRIDE;
			for (int x = 0; x < FB_W; x++) {
				uint8_t a = (x / 16) & 1;
				ln[x * 3] = a ? 0 : 0xff; ln[x * 3 + 1] = a ? 0 : 0xff;
				ln[x * 3 + 2] = a ? 0xff : 0xff;
			}
		}
	}
	__asm__ volatile("dsb sy" ::: "memory");
	con_str("GRID", "RGB888/2160: top=rg bands bottom=bw bars");
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 65
	/* Unmistakable visual liveness test: cycle red/green/blue forever,
	   1.5 s each, MMU off, no reset. If the shim runs, the panel flashes
	   colours; when the battery UI replaces it, the shim is dead. */
	for (;;) {
		con_clear(0x00ff0000);
		con_str("SHIM", "beacon65 RED");
		delay_ms(1500);
		con_clear(0x0000ff00);
		con_str("SHIM", "beacon65 GREEN");
		delay_ms(1500);
		con_clear(0x000000ff);
		con_str("SHIM", "beacon65 BLUE");
		delay_ms(1500);
	}
#endif
#if SHIM_BEACON == 64
	/* Visual execution proof with the MMU off: red screen + text for
	   20 s, then green for 20 s, then spin. If the panel shows it, the
	   shim runs, delay_ms works and we have a human-readable channel. */
	con_clear(0x00ff0000);
	con_str("SHIM", "beacon64 running");
	con_str("STAGE", "red 20s -> green");
	delay_ms(20000);
	con_clear(0x0000ff00);
	con_str("SHIM", "beacon64 green");
	delay_ms(20000);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 63
	/* Route the next boot to fastboot through the PMIC itself:
	   PM8998 PON (SID0) soft-reboot spare 0x88F bits[7:1] hold the
	   7-bit restart reason (qpnp_pon_set_restart_reason, gen2 = <<1).
	   BOOTLOADER = 3 -> write 0x06. apid 4 = ppid 0x008.
	   If the phone comes back in FASTBOOT, the shim's SPMI write worked
	   and future tests can loop without key presses. */
	{
		int w = spmi_write_apid(4, 0x8f, 0x06);
		(void)w;
		rr_route(RESTART_BOOTLOADER);
		__asm__ volatile("dsb sy" ::: "memory");
		delay_ms(2000);
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wdt_arm(2, 3);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 62
	/* Execution proof that cannot be confused with an ABL crash: after
	   5 s (below any ~10 s dog) request SYSTEM_RESET with the bootloader
	   restart reason. Shim ran -> phone comes back in FASTBOOT; shim did
	   not run -> charger UI then Android. No MMU, no SPMI. */
	delay_ms(5000);
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wdt_arm(2, 3);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 61
	/* PMI8998 SMB2 charger watchdog: XBL leaves it armed when the phone
	   comes up on USB; it bites after ~10 s with a PMIC reset (the
	   charger UI). Linux disables it in qpnp-smb2 probe
	   (WD_CFG_REG=0). ppid 0x216 (SID2 addr 0x1600) -> apid 39.
	   MMU off. Disable WD_CFG (0x1651) and SNARL/BARK/BITE cfg
	   (0x1653), pet the APSS WDT through a 60 s wait, then PSHOLD. */
	{
		int w1 = spmi_write_apid(39, 0x1651, 0x00);
		for (int i = 0; i < 120; i++) {
			wdt_pet();
			delay_ms(500);
		}
		(void)w1;
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 60
	/* MMU off: pet the APSS WDT every 500 ms for 60 s, then PSHOLD.
	   Charger UI at ~T0+75 s => the dog is the ~10 s killer and petting
	   works; at ~T0+25 s => the killer is not the APSS WDT. */
	for (int i = 0; i < 120; i++) {
		wdt_pet();
		delay_ms(500);
	}
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 59
	/* A/B control for 58: identical 30 s delay + PSHOLD, but MMU left
	   off. If this reaches ~T0+50 s while 58 dies at ~T0+25 s, then
	   mmu_enable() is what kills the shim (and every earlier MMU-on
	   "SPMI result" was really a watchdog bite). */
	delay_ms(30000);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 58
	/* Survival test with only proven primitives: 30 s of arch-timer
	   delay, then PSHOLD. Host timestamps when the charger UI (E1F4)
	   appears: ~T0+45 s = no external killer, ~T0+25 s = something
	   resets the AP around 10 s. */
	{
		struct mmu_info mi;
		(void)mmu_enable(&mi);
		delay_ms(30000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 57
	/* Same 16-bit payload as 56, but reset via the APSS WDT bite only:
	   a warm reset keeps IMEM, a PSCI SYSTEM_RESET appears to power-cycle
	   the PMIC (charger UI, warmboot=0). */
	{
		struct mmu_info mi;
		int apid, rev;
		uint32_t ver;
		(void)mmu_enable(&mi);
		ver = rd32(0x800f000UL);
		apid = spmi_find_apid(0x0008);
		rev = spmi_read(0x0008, 0, 0x01);
		rr_route(0x77660000u |
		     ((uint32_t)(rev < 0 ? 0xf : rev & 0xf) << 12) |
		     (((ver >> 28) & 0xf) << 8) |
		     ((uint32_t)(apid < 0 ? 0xf : apid & 0xf) << 4) | 6u);
		wdt_arm(2, 4);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 54
	/* UFS appslog channel test (MMU off, no SPMI): write a magic block and
	   power off. If it survives, appslog is a precise telemetry channel. */
	{
		volatile uint8_t *lb = (volatile uint8_t *)0x9f500000UL;
		int us;
		for (unsigned i = 0; i < 4096; i++)
			lb[i] = 0;
		lb[0] = 'S'; lb[1] = 'H'; lb[2] = 'U'; lb[3] = 'F';
		lb[4] = 'S'; lb[5] = '5'; lb[6] = '4';
		us = test_ufs();
		lb[8] = (uint8_t)us;
		ufs_write_blocks(APPSLOG_LBA, 1, (const void *)lb);
		ufs_write_blocks(APPSLOG_LBA + 256, 1, (const void *)lb);
		ufs_sync_cache();
		ufs_hibernate();
		delay_ms(300);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 52
	/* PSHOLD-timing: code = (CurrentEL != EL1) | ((SCTLR_EL1.M == 0) << 1);
	   delay 2/4/6/8 s. */
	{
		struct mmu_info mi;
		uint32_t el, m, code;
		(void)mmu_enable(&mi);
		el = (uint32_t)(rsr("CurrentEL") >> 2) & 3;
		m = (uint32_t)(rsr("sctlr_el1") & 1);
		code = (el != 1) | ((m == 0) << 1);
		delay_ms((2 + 2 * code) * 1000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 51
	/* PSHOLD-timing: does spmi_find_apid(0x0008) return 4? 2 s yes / 8 s no. */
	{
		struct mmu_info mi;
		int a;
		(void)mmu_enable(&mi);
		a = spmi_find_apid(0x0008);
		delay_ms(a == 4 ? 2000 : 8000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 49
	/* PSHOLD-timing: does spmi_write return 0? 2 s if yes, 8 s if not. */
	{
		struct mmu_info mi;
		int w;
		(void)mmu_enable(&mi);
		w = spmi_write(0x0008, 0, 0x8e, 0x5a);
		delay_ms(w == 0 ? 2000 : 8000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 50
	/* Baseline: fixed 2 s then PSHOLD. */
	delay_ms(2000);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 48
	/* MMU-on residency + WDT pet + PMIC markers. Survive >20 s => the
	   nGnRE mapping fixed both the WDT and the SPMI access. */
	{
		struct mmu_info mi;
		(void)mmu_enable(&mi);
		spmi_write(0x0208, 0, 0x8e, 0xb5);
		spmi_write(0x0208, 0, 0x8f, 0x00);
		spmi_write(0x0008, 0, 0x8e, 0x5e);
		for (;;) {
			wdt_pet();
			delay_ms(300);
		}
	}
#endif
#if SHIM_BEACON == 45
	/* MMU-on nGnRE test: map MMIO as Device-nGnRE (like ioremap), then
	   pet the WDT and write PMIC markers. If the markers land, the
	   MMU-off/Device-nGnRnE attribute was the "10 s killer" root cause. */
	{
		struct mmu_info mi;
		(void)mmu_enable(&mi);
		spmi_write(0x0208, 0, 0x8e, 0xb5);
		spmi_write(0x0208, 0, 0x8f, 0x00);
		spmi_write(0x0008, 0, 0x8e, 0x5e);
		wdt_pet();
		delay_ms(300);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 10
	/* Durable execution proof + SPMI forensics: dump the arbiter APID
	   map, SPMI return codes and WDT state into one appslog block, push
	   it through the UFS host (FUA+sync), then power off. */
	{
		/* Fixed scratch DDR (just past the ABL splash) so the 4K log
		   buffer needs no static storage / relocations. */
		volatile uint8_t *lb = (volatile uint8_t *)0x9f500000UL;
		for (unsigned i = 0; i < 4096; i++)
			lb[i] = 0;
		lb[0] = 'S'; lb[1] = 'H'; lb[2] = 'B'; lb[3] = 'E';
		lb[4] = 'A'; lb[5] = 'C'; lb[6] = 'O'; lb[7] = 'N';
		lb[8] = '1'; lb[9] = '0';
		lb[16] = (uint8_t)(-(int)spmi_write(0x0208, 0, 0x8e, 0xb5));
		lb[17] = (uint8_t)(-(int)spmi_write(0x0208, 0, 0x8f, 0x00));
		lb[18] = (uint8_t)(-(int)spmi_write(0x0008, 0, 0x8e, 0x5e));
		lb[20] = (uint8_t)spmi_read(0x0008, 0, 0x01);	/* 0x801 rev, want 1 */
		lb[21] = (uint8_t)spmi_read(0x0208, 0, 0x01);
		lb[22] = (uint8_t)spmi_read(0x0008, 0, 0x0c);	/* poff reason */
		lb[23] = (uint8_t)spmi_read(0x0008, 0, 0x8e);	/* readback */
		((uint32_t *)lb)[6] = rd32(0x17817000UL + 0x08);	/* EN */
		((uint32_t *)lb)[7] = rd32(0x17817000UL + 0x0c);	/* STS */
		((uint32_t *)lb)[8] = rd32(0x17817000UL + 0x10);	/* BARK */
		((uint32_t *)lb)[9] = rd32(0x17817000UL + 0x14);	/* BITE */
		((uint32_t *)lb)[10] = (uint32_t)rsr("CurrentEL");
		((uint32_t *)lb)[11] = (uint32_t)rsr("mpidr_el1");
		((uint32_t *)lb)[12] = (uint32_t)rsr("cntfrq_el0");
		for (int i = 0; i < 512; i++)	/* APID map, 0x800..0xfff */
			((uint32_t *)lb)[16 + i] = rd32(0x800f000UL + 0x800 + 4 * i);
		int us = test_ufs();
		lb[13] = (uint8_t)us;	/* UFS init stage */
		uint32_t w1 = ufs_write_blocks(APPSLOG_LBA, 1, lb);
		uint32_t w2 = ufs_write_blocks(APPSLOG_LBA + 256, 1, lb);
		uint32_t sy = ufs_sync_cache();
		ufs_hibernate();
		(void)w2;
		(void)sy;
		delay_ms(500);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 8
	/* Pure-MMIO execution proof (no SPMI): arm the APSS WDT at 3s/6s
	   and park. If this payload runs, the dog bites at ~6s and the
	   phone resets instead of booting TWRP's Linux. */
	wdt_arm(3, 6);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 43
	/* PSHOLD-timing probe: can the shim read the SPMI APID map at all?
	   apid==4 -> 2 s, otherwise 6 s, then power off. Host times the
	   appearance of the charger UI. */
	{
		int a = spmi_find_apid(0x0008);
		delay_ms((a == 4) ? 2000 : 6000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 44
	/* PSHOLD-timing probe: does spmi_read of PON rev (0x801) return 1? */
	{
		int r = spmi_read(0x0008, 0, 0x01);
		delay_ms((r == 1) ? 2000 : 6000);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 40
	/* Timing telemetry: encode 2 bits of SPMI lookup state into the
	   delay before self-reset. code = (no SID0 apid) | (no SID2 apid)<<1
	   delay = 2 + 2*code s. Host measures time until fastboot returns. */
	{
		int a0 = spmi_find_apid(0x0008);
		int a2 = spmi_find_apid(0x0208);
		uint32_t code = (a0 < 0) | ((a2 < 0) << 1);
		for (uint32_t i = 0; i < 2 + 2 * code; i++)
			delay_ms(1000);
		rr_route(RESTART_BOOTLOADER);
		__asm__ volatile("dsb sy" ::: "memory");
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 41
	/* code = (SID0 rev read != 1) | (write SID2 88e != 0)<<1 */
	{
		int r0 = spmi_read(0x0008, 0, 0x01);
		int w0 = spmi_write(0x0208, 0, 0x8e, 0xb5);
		uint32_t code = (r0 != 1) | ((w0 != 0) << 1);
		for (uint32_t i = 0; i < 2 + 2 * code; i++)
			delay_ms(1000);
		rr_route(RESTART_BOOTLOADER);
		__asm__ volatile("dsb sy" ::: "memory");
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#if SHIM_BEACON == 32
	/* delay_ms validation: 20 s of nothing but the arch timer, then
	   self-reset to fastboot. Host measures the return time. */
	delay_ms(20000);
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 33
	/* WDT MMIO probe: one read of WDT0_EN, then self-reset. If the read
	   hangs the bus, the phone never returns to fastboot. */
	{
		volatile uint32_t w = rd32(0x17817000UL + 0x08);
		(void)w;
	}
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 30
	/* Self-return test: ask TZ for SYSTEM_RESET with restart_reason =
	   fastboot. If this lands in fastboot, future tests can loop
	   without physical key presses. */
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 20
	/* Positive execution proof: delay, then hand control back to the
	   real Linux entry point of the image we are sewn into. PAYLOAD_OFF
	   / KENTRY_OFF are the payload's and the original entry's offsets
	   in the decompressed kernel Image (supplied by the builder). */
	{
		extern char _head[];
		uint64_t self;
		__asm__ volatile("adr %0, _head" : "=r"(self));
		uint64_t tgt = self - (uint64_t)PAYLOAD_OFF + (uint64_t)KENTRY_OFF;
		delay_ms(15000);
		register uint64_t x0 __asm__("x0") = dtb;
		register uint64_t x1 __asm__("x1") = 0;
		register uint64_t x2 __asm__("x2") = 0;
		register uint64_t x3 __asm__("x3") = 0;
		register uint64_t t __asm__("x4") = tgt;
		__asm__ volatile("br x4" : : "r"(x0), "r"(x1), "r"(x2),
				 "r"(x3), "r"(t) : "memory");
	}
#endif
#if SHIM_BEACON == 13
	/* Survival test: 60 s with a 1 Hz WDT pet, then reset. If the
	   phone reboots only every ~64 s there is no real killer; a ~12-
	   15 s period means an armed dog that our pet/disarm can't touch. */
	wdt_disarm();
	for (int i = 0; i < 60; i++) {
		wdt_pet();
		delay_ms(1000);
	}
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
#if SHIM_BEACON == 12
	/* Timing proof with self-return: 8 s of visible delay, then ask
	   TZ to reset and route ABL to fastboot. Landing in fastboot well
	   after the ~3-4 s boot proves the payload ran; PSHOLD is only a
	   fallback if TZ refuses SYSTEM_RESET. */
	wdt_disarm();
	delay_ms(8000);
	rr_route(RESTART_BOOTLOADER);
	__asm__ volatile("dsb sy" ::: "memory");
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wr32(PSHOLD, 0);
	for (;;)
		__asm__ volatile("wfe");
#endif
	{
		uint32_t en0 = rd32(0x17817000UL + 0x08);
		uint32_t bark = rd32(0x17817000UL + 0x10);
		uint32_t bite = rd32(0x17817000UL + 0x14);
		int w1 = spmi_write(0x0208, 0, 0x8e, 0xb5);
		int w2 = spmi_write(0x0208, 0, 0x8f, 0x00);
		uint32_t st = 0x20 | (en0 & 1) << 4 |
			      (bark >= 60 * 32765) << 3 |
			      (bite >= 63 * 32765) << 2 |
			      (SHIM_BEACON & 7);
#if SHIM_BEACON == 1
		/* Channel validation only: mark, then PS_HOLD power-off. */
		spmi_write(0x0008, 0, 0x8e, 0x5e);
		delay_ms(300);
		wr32(PSHOLD, 0);
		for (;;)
			__asm__ volatile("wfe");
#else
		if (w1 < 0 || w2 < 0)
			st = 0x7f;
		spmi_write(0x0008, 0, 0x8e, st);
#if SHIM_BEACON == 4
		/* Re-arm with long timers, then prove the writes stuck by
		   reading EN and BARK back before the heartbeat starts. */
		wdt_arm(60, 63);
		en0 = rd32(0x17817000UL + 0x08);
		bark = rd32(0x17817000UL + 0x10);
		bite = rd32(0x17817000UL + 0x14);
		spmi_write(0x0008, 0, 0x8e,
			   0x20 | (en0 & 1) << 4 |
			   (bark >= 60 * 32765) << 3 |
			   (bite >= 63 * 32765) << 2 | (SHIM_BEACON & 7));
#endif
#if SHIM_BEACON == 5
		wdt_disarm();
		en0 = rd32(0x17817000UL + 0x08);
		spmi_write(0x0008, 0, 0x8e,
			   0x20 | (en0 & 1) << 4 | (SHIM_BEACON & 7));
#endif
		for (uint32_t n = 1;; n++) {
			delay_ms(1000);
			spmi_write(0x0208, 0, 0x8f, n & 0xff);
#if SHIM_BEACON == 3 || SHIM_BEACON == 4
			wdt_pet();
#endif
		}
#endif
	}
#endif
#ifdef SHIM_PURE
	/* Zero-MMIO bisect probe: touch ONLY the framebuffer, then park.
	   If this survives, the ~10 s killer is one of our own MMIO pokes
	   (IMEM / WDT / pstore / UFS), not the ABL. SHIM_PURE=2 pets the
	   APSS WDT in the park loop: if that variant survives while the
	   plain one dies, an ABL-armed watchdog is the killer (EN locked
	   by TZ, but RST pet channel open to NS). */
	(void)dtb;
	con_clear(0x003030);
	con_str("PURE", "no mmio — parked");
	con_hex("CPU", rsr("CurrentEL"));
#if SHIM_PURE == 2
	con_str("MODE", "wdt petting");
	for (;;) {
		wdt_pet();
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 3
	con_str("MODE", "smc+pet keepalive");
	for (;;) {
		smc(0x84000000, 0, 0, 0);   /* PSCI_VERSION */
		smc(0x80000000, 0, 0, 0);   /* SMCCC_VERSION */
		wdt_pet();
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 4
	/* scm_call2(SCM_SIP_FNID(BOOT=1, SEC_WDOG_DIS=7), arg=1):
	   x0=fnid|SMC64, x1=arginfo, x2..x5=args — CAF scm_call2 ABI. */
	con_str("MODE", "scm sec-wdt disarm");
	smc5(0x42000107, 1, 1, 0, 0, 0);
	smc5(0xC2000107, 1, 1, 0, 0, 0);   /* atomic variant, belt&braces */
	con_str("WDOG", "disarm sent");
	for (;;) {
		wdt_pet();
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 6
	con_str("MODE", "all cores checkin");
	for (int i = 1; i < 8; i++) {
		uint64_t r = smc(PSCI_CPU_ON, (uint64_t)i << 8,
				 (uintptr_t)secondary_entry, i);
		con_hex2("cpu", "id=", i, "on=", (uint32_t)r);
	}
	for (;;) {
		wdt_pet();
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 7
	/* Fuse-length measurement: mark a checkpoint every ~500 ms on
	   screen + IMEM diag cell; at ~5 s, warm-reset to fastboot. If we
	   land safely, the killer fires >5 s — then bisect upward. */
	con_str("MODE", "5s self-reset");
	{
		volatile uint32_t *cell = (volatile uint32_t *)0x9f400000UL;
		for (int i = 0; i < 10; i++) {
			cell[i] = 0xD1E50000 | i;
			con_hex("tick", i);
			for (volatile int j = 0; j < 1000000; j++)
				__asm__ volatile("nop");
		}
		con_str("RST", "to fastboot now");
		rr_route(0x77665500u);
		__asm__ volatile("dsb sy" ::: "memory");
		smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	}
#elif SHIM_PURE == 8
	/* Run cached+paged like a real kernel: if the fabric/NOC hang
	   detector kills long uncached spins, enabling MMU should save us.
	   Symbols are linked at 0 — runtime phys base via adr. */
	con_str("MODE", "mmu+cache then park");
	{
		uint64_t base, top;
		__asm__ volatile("adr %0, _head" : "=r"(base));
		__asm__ volatile("adr %0, _end" : "=r"(top));
		struct mmu_info mi;
		int r = mmu_enable(&mi,
				 base & ~0xfffUL, (top + 0xfff) & ~0xfffUL);
		con_hex("MMU", r ? 0xbad : mi.sctlr_after);
	}
	for (;;) {
		wdt_pet();
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 9
	/* WDT forensics: dump the ABL-handoff register state, attempt
	   disarm, read back to detect TZ/XPU write-locking, then heartbeat
	   while petting. Reveals (a) whether the dog was even running at
	   handoff, (b) whether our writes land, (c) which beat the killer
	   fires on (~1s per beat). */
	con_str("MODE", "wdt forensics");
	con_hex("WD00", rd32(0x17817000UL + 0x00));
	con_hex("EN0",  rd32(0x17817000UL + 0x08));
	con_hex("STS",  rd32(0x17817000UL + 0x0c));
	con_hex("BARK", rd32(0x17817000UL + 0x10));
	con_hex("BITE", rd32(0x17817000UL + 0x14));
	wdt_disarm();
	con_hex("EN1",  rd32(0x17817000UL + 0x08));
	{
		uint32_t beat = 0;
		for (;;) {
			con_hex("beat", beat++);
			wdt_pet();
			for (volatile int i = 0; i < 2000000; i++)
				__asm__ volatile("nop");
		}
	}
#elif SHIM_PURE == 10
	/* SILENT HEARTBEAT — the DSI cmd-mode panel never displays our FB
	   writes, so screen output is a dead channel. Instead write the
	   whole forensic record into the IMEM safe zone (<0x720, tz-log is
	   at 0x720+). Layout:
	     0x146bf000  alive magic 0x5149ACE1
	     0x146bf004  heartbeat counter (each ~200 ms)
	     0x146bf008  WDT handoff snapshot: EN|STS|BARK|BITE packed
	     0x146bf00c  EN readback after disarm attempt
	     0x146bf010  live WDT STS (re-read each beat)
	     0x146bf014  incoming restart_reason (ABL handoff)
	     0x146bf018  CurrentEL at entry
	     0x146bf01c  beat of last successful pet write
	   Read back via /dev/fmem after next boot. If cells are all zero →
	   PMIC-level power loss wiped IMEM, or we never ran. */
	volatile uint32_t *im = (volatile uint32_t *)0x146bf000UL;
	uint32_t en0  = rd32(0x17817000UL + 0x08);
	uint32_t sts  = rd32(0x17817000UL + 0x0c);
	uint32_t bark = rd32(0x17817000UL + 0x10);
	uint32_t bite = rd32(0x17817000UL + 0x14);
	im[0] = 0x5149ACE1u;
	im[1] = 0;
	im[2] = en0 ^ (sts << 8) ^ (bark << 16) ^ (bite << 24); /* lossy pack, also store raw below */
	im[3] = 0;
	im[4] = 0;
	im[5] = rd32(IMEM_RESTART_REASON);
	im[6] = rsr("CurrentEL");
	im[7] = 0;
	/* raw copies of the four regs in cells 8..11 for clean decode */
	im[8] = en0; im[9] = sts; im[10] = bark; im[11] = bite;
	wdt_disarm();
	im[3] = rd32(0x17817000UL + 0x08);	/* did EN write stick? */
	for (;;) {
		im[1]++;
		wdt_pet();
		im[4] = rd32(0x17817000UL + 0x0c);	/* live STS */
		im[7] = im[1];
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 11
	/* PMIC PON_WD hunt — the one channel never tried. PM8998 PON periph
	   lives on SPMI usid0 ppid 0x08; WD disarm bit = reg 0x857 bit7.
	   If ABL/fastboot arms a PMIC-level watchdog whose S2 action is
	   shutdown, only SPMI writes can stop it — every MMIO pet was
	   shooting at the wrong dog. Dump before/after into IMEM cells. */
	volatile uint32_t *im = (volatile uint32_t *)0x146bf000UL;
	im[0] = 0x53504D49u;	/* "SPMI" */
	int apid = spmi_find_apid(0x08);
	im[1] = (uint32_t)apid;
	int r0 = spmi_read(0x08, 0, 0x857);
	im[2] = (uint32_t)r0;
	int w = spmi_write(0x08, 0, 0x857, (uint8_t)(r0 & ~0x80));
	im[3] = (uint32_t)w;
	int r1 = spmi_read(0x08, 0, 0x857);
	im[4] = (uint32_t)r1;
	/* also read S1/S2 timer cfg + S2 type for forensics */
	im[5] = (uint32_t)spmi_read(0x08, 0, 0x854);	/* WD S1 timer */
	im[6] = (uint32_t)spmi_read(0x08, 0, 0x855);	/* WD S2 timer */
	im[7] = (uint32_t)spmi_read(0x08, 0, 0x856);	/* WD S2 ctl  */
	uint32_t beat = 0;
	for (;;) {
		im[8] = beat++;
		for (volatile int i = 0; i < 400000; i++)
			__asm__ volatile("nop");
	}
#elif SHIM_PURE == 12
	/* EXECUTION PROOF: PSHOLD write-0 is a guaranteed instant PMIC
	   poweroff (verified 10-21). If the device dies in <2s, our code
	   really runs and the ~10s killer is a genuine timer. If it still
	   takes ~10s, WE NEVER RAN AT ALL — ABL jumps elsewhere and every
	   conclusion tonight is void. */
	wr32(PSHOLD, 0);
	for (;;) ;
#else
	for (;;)
		__asm__ volatile("wfe");
#endif
#endif
	/* The incoming restart_reason is evidence — cp_mark clobbers it
	   below, so capture it first (SHIM_PROBE prints this, not the live cell). */
	uint32_t rr_orig = rd32(IMEM_RESTART_REASON);
	(void)rr_orig;
	imem_diag("entry", 0x5149, (uint32_t)dtb);
	cp_mark('C','0');
	/* Dead-man switch: any hang bites within ~2 min and lands back in
	   fastboot (observed: everything resets to fastboot after fastboot boot). */
	wdt_arm(120, 125);
	wdt_pet();
#ifdef SHIM_PROBE
	/* Post-mortem probe — fired via `fastboot boot` right after an XNU
	 * attempt lands back in fastboot (warm reset keeps IMEM). Dumps the
	 * IMEM telemetry page to the framebuffer AND the pstore console prz,
	 * mirrors it to appslog on UFS (survives even power-off), then arms
	 * a short WDT to warm-reset into Android. Never jumps to a kernel. */
	{
		con_clear(0x003030);
		con_str("PROBE", "watchdog + imem harvest");
		con_hex("RR-ORIG", rr_orig);      /* captured before cp_mark clobbered it */
		con_skip();
		/* Suspect: ABL hands us a running (secure) watchdog that stock
		   kernels take over but we never pet -> ~10s power-off. Dump
		   every WDT reg BEFORE arming so the state is on screen even
		   if we die right after. */
		con_hex("WDT_EN ", rd32(0x17817000UL + 0x08));
		con_hex("WDT_STS", rd32(0x17817000UL + 0x0c));
		con_hex("WDT_BRK", rd32(0x17817000UL + 0x10));
		con_hex("WDT_BIT", rd32(0x17817000UL + 0x14));
		con_hex("EL    ", rsr("CurrentEL"));
		/* Kill the APSS WDT outright and park: if an ABL-left watchdog
		   was the killer, we now stay alive with this screen up —
		   the theory's smoking gun either way. */
		wdt_disarm();
		con_str("WDT   ", "DISARMED — holding");
		con_skip();
		static const uint16_t cells[] = {
			0x200, 0x300, 0x400, 0x500,      /* mirror cells (65c was clobbered by cp_mark; see RR-ORIG) */
			0x700, 0x708, 0x710, 0x718,           /* pre-jump state */
			0x720, 0x728, 0x730, 0x738, 0x740, 0x748, 0x750, 0x758, 0x760, 0x768, 0x770, 0x778,
			0x780, 0x788, 0x790, 0x798,           /* vectors2 dump */
			0x7a0, 0x7a8, 0x7b0, 0x7b8, 0x7c0, 0x7c8, 0x7d0, 0x7d8, 0x7e0, 0x7e8, 0x7f0, 0x7f8,
			0x800, 0x808, 0x810, 0x818, 0x820, 0x828, 0x830, 0x838, 0x840, /* hios ret */
		};
		for (unsigned i = 0; i < sizeof(cells)/sizeof(cells[0]); i++) {
			char tag[8];
			tag[0] = 'I'; tag[1] = 'M';
			const char *hx = "0123456789abcdef";
			tag[2] = hx[(cells[i] >> 8) & 0xf];
			tag[3] = hx[(cells[i] >> 4) & 0xf];
			tag[4] = hx[cells[i] & 0xf];
			tag[5] = 0;
			con_hex(tag, rd64(0x146bf000UL + cells[i]));
		}
		con_skip();
		con_str("PSTORE", "writing prz console + imem page");
		/* "IMEMDUMP-2026-10-22 " as immediates — a string source here
		   makes GCC emit an ABS64 rodata constant (src-dst fold). */
		wr32(0xffcc000cUL, 0x4d454d49);	/* "IMEM" */
		wr32(0xffcc0010UL, 0x504d5544);	/* "DUMP" */
		wr32(0xffcc0014UL, 0x3230322d);	/* "-202" */
		wr32(0xffcc0018UL, 0x30312d36);	/* "6-10" */
		wr32(0xffcc001cUL, 0x2032322d);	/* "-22 " */
		for (uint32_t i = 0; i < 1024; i++)
			wr32(0xffcc0000UL + 32 + i * 4, rd32(0x146bf000UL + i * 4));
		wr32(0xffcc0008UL, 20 + 4096);
		wr32(0xffcc0004UL, 0);
		wr32(0xffcc0000UL, 0x43474244);
		con_str("HOLD ", "parked — read screen, then power-key");
		for (;;)
			__asm__ volatile("wfe");
	}
#endif
#ifdef SHIM_PING
	/* Proof-of-life with a delay so timing distinguishes "ran to end"
	   from "never jumped in": 20 s delay then reset to fastboot. */
	imem_diag("ping", 0xdead, 0xbeef);
	for (int i = 0; i < 20; i++) { delay_ms(1000); wdt_pet(); }
	rr_route(0x77665501u);
	/* PSHOLD=0 powers OFF on lilac — use a WDT bite for the warm reset. */
	wdt_arm(2, 3);
	for (;;)
		__asm__ volatile("wfe");
#endif
	struct mmu_info mi;

	/* An xnu payload in the ramdisk means: boot it (MMU stays off). */
	stage = "loader";
	cp_mark('C','1');
	con_clear(RED);
	con_str("SHIM", "sweet milestone 4: xnu loader");
	con_skip();
	cp_mark('C','2');
#ifdef SHIM_RET_FASTBOOT
	/* Unattended bring-up: pre-arm PMIC PON SOFT_RB_SPARE(0x88f) = BOOTLOADER
	   so ANY subsequent reset (XNU panic PSCI, WDT, hang-feeds) routes back
	   to fastboot instead of Android/charge. Verified: beacon73 wrote 0x06
	   here and XBL landed in fastboot. XNU can overwrite it later. */
	spmi_write_apid(4, 0x88f, 0x06);
	con_str("PON", "reason=BOOTLOADER armed (auto fastboot on reset)");
#endif
	/* Read back LAST run doorbells before this run overwrites them:
	   every fire self-reports the previous run marks on screen. */
	{
		extern int spmi_read_apid(int, uint16_t);
		int pe = spmi_read_apid(4, 0x88e), pf = spmi_read_apid(4, 0x88f);
		int p9 = spmi_read_apid(4, 0x889), pa = spmi_read_apid(4, 0x88a);
		int pb = spmi_read_apid(4, 0x88b), pc = spmi_read_apid(4, 0x88c);
		int pd = spmi_read_apid(4, 0x88d);
		con_hex2("PONe/f", "88e=", (uint32_t)pe, " 88f=", (uint32_t)pf);
		con_hex2("PON9/a", "889=", (uint32_t)p9, " 88a=", (uint32_t)pa);
		con_hex2("PONb/c", "88b=", (uint32_t)pb, " 88c=", (uint32_t)pc);
		con_hex("PON88d", (uint32_t)pd);
		/* SHIM_STEP_DELAY: camera-free telemetry. The previous XNU run's
		 * last A540 step is in PON 0x88e (0xb1..0xb8). Delay the jump by
		 * (step-0xb0)*4s so the host can decode it from the time between
		 * the boot command and the next fastboot enumeration. */
		if (pe >= 0xb0 && pe <= 0xbf)
			delay_ms((uint32_t)(pe - 0xb0) * 4000);

		/* Post-MMU probes left by the previous XNU run: 'PST1' sits
		   somewhere in the boot-args area (scan, the PA drifts per
		   run) and 'POST' is at the fixed durable cell 0xffbff000. */
		{
			uint32_t pv = *(volatile uint32_t *)0xffbff000UL;
			uint32_t pst_at = 0;
			for (volatile uint32_t *q = (volatile uint32_t *)0xa1d00000UL;
			     q < (volatile uint32_t *)0xa1e00000UL; q++) {
				if (*q == 0x31545350u) { pst_at = (uint32_t)(uintptr_t)q; break; }
			}
			con_hex("POST", pv);
			con_hex("PST1@", pst_at);
		}
		con_skip();
	}
	const char *why = loader_boot(dtb);
	cp_mark('C','3');
	con_str("LOADER", why);
	con_skip();
	/* SHIM_ERR_FASTBOOT: never fall back to Android when the payload is
	 * missing or rejected; park the next boot in fastboot instead of
	 * running the long UFS probe and rr_route(normal boot). */
	spmi_write_apid(4, 0x8f, 0x06);
	__asm__ volatile("dsb sy" ::: "memory");
	delay_ms(200);
	smc(PSCI_SYSTEM_RESET, 0, 0, 0);
	wdt_arm(2, 3);
	for (;;)
		__asm__ volatile("wfe");

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
	rr_route(0x77665501u);
	/* PSHOLD=0 powers OFF on lilac — use a WDT bite for the warm reset. */
	wdt_arm(2, 3);
	for (;;)
		__asm__ volatile("wfe");

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
