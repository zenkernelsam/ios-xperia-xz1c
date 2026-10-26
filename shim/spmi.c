/*
 * Minimal SPMI PMIC-arbiter v2 access for lilac (msm8998).
 * Register map from CAF drivers/spmi/spmi-pmic-arb.c:
 *   core   @0x800f000  (per-apid map entries at +0x800+4*n)
 *   cnfg   @0x800a000  (ownership table at +0x700+4*n)
 *   obsrvr @0x9400000  (read/obs channel space)
 *   chnls  @0x8400000  (read/write channel space)
 * Per-channel regs: CMD=0x00 CFG=0x04 STATUS=0x08 WDATA0=0x10 RDATA0=0x18.
 * v2 channel offset = 0x1000*ee + 0x8000*apid  (ee = 0 for APPS).
 * cmd word v2 = (opc<<27) | ((addr&0xff)<<4) | (bc&0x7).
 */
#include <stdint.h>

#define ARB_CORE	0x800f000UL
#define ARB_CNFG	0x800a000UL
#define ARB_OBS		0x9400000UL
#define ARB_WR		0x8400000UL

/* msm8998 DT sets qcom,enable-ahb-bus-workaround; the kernel reads this
   cnfg register before every write cmd or the transaction is corrupted. */
#define ARB_PROTOCOL_IRQ_STATUS	0x6000

#define CH_CMD		0x00
#define CH_STATUS	0x08
#define CH_WDATA0	0x10
#define CH_WDATA1	0x14
#define CH_RDATA0	0x18
#define CH_RDATA1	0x1c

/* PON registers live at 0x8xx (>0xFF): the kernel's SPMI regmap uses
   EXT_WRITEL / EXT_READL for them (SPMI_CMD 0x30 / 0x38), which map to
   arbiter opcodes 0 / 1. The short WRITE/READ (14/15) address the wrong
   register space and silently no-op. */
#define OP_READ		1
#define OP_WRITE	0

#define ST_DONE		1u
#define ST_FAILURE	2u
#define ST_DENIED	4u
#define ST_DROPPED	8u

static inline uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)a; }
static inline void wr32(uint64_t a, uint32_t v)
{
	*(volatile uint32_t *)a = v;
	__asm__ volatile("dsb sy" ::: "memory");
}

/* Scan the per-apid map for `ppid`; return apid or -1.
   core+0x800+4*apid holds entries; (v>>8)&0xFFF == ppid. */
int spmi_find_apid(uint16_t ppid)
{
	for (int apid = 0; apid < 256; apid++) {
		uint32_t v = rd32(ARB_CORE + 0x800 + 4 * apid);
		if (!v)
			continue;
		if (((v >> 8) & 0xfff) == ppid)
			return apid;
	}
	return -1;
}

static int ch_wait(uint64_t base, uint32_t off)
{
	for (int i = 0; i < 100000; i++) {
		uint32_t s = rd32(base + off + CH_STATUS);
		if (s & ST_DONE)
			return (s & (ST_FAILURE | ST_DENIED | ST_DROPPED)) ? -(int)s : 0;
	}
	return -1;
}

/* Write one byte to a known APID, skipping the map scan. Lets callers
   act even if the APID table is unreadable. */
int spmi_write_apid(int apid, uint16_t addr, uint8_t val)
{
	if (apid < 0 || apid > 511)
		return -1;
	uint32_t off = 0x8000 * (uint32_t)apid;
	wr32(ARB_WR + off + CH_WDATA0, val);
	wr32(ARB_WR + off + CH_CMD, (OP_WRITE << 27) | ((addr & 0xff) << 4));
	return ch_wait(ARB_WR, off);
}

/* Read one byte from (sid,addr). Returns byte or <0 error. */
int spmi_read(uint16_t ppid, uint8_t sid, uint16_t addr)
{
	int apid = spmi_find_apid(ppid);
	if (apid < 0)
		return -100;
	uint32_t off = 0x8000 * (uint32_t)apid;
	uint32_t cmd = (OP_READ << 27) | ((addr & 0xff) << 4);
	wr32(ARB_OBS + off + CH_CMD, cmd);
	int r = ch_wait(ARB_OBS, off);
	if (r < 0)
		return r;
	return rd32(ARB_OBS + off + CH_RDATA0) & 0xff;
}

/* Write one byte to (sid,addr). Returns 0 or <0 error. */
int spmi_write(uint16_t ppid, uint8_t sid, uint16_t addr, uint8_t val)
{
	int apid = spmi_find_apid(ppid);
	if (apid < 0)
		return -100;
	uint32_t off = 0x8000 * (uint32_t)apid;
	wr32(ARB_WR + off + CH_WDATA0, val);
	uint32_t cmd = (OP_WRITE << 27) | ((addr & 0xff) << 4);
	wr32(ARB_WR + off + CH_CMD, cmd);
	return ch_wait(ARB_WR, off);
}
/* Read one byte via an already-known apid (mirror of spmi_write_apid). */
int spmi_read_apid(int apid, uint16_t addr)
{
	uint32_t off = 0x8000 * (uint32_t)apid;
	wr32(ARB_OBS + off + CH_CMD, (OP_READ << 27) | ((addr & 0xff) << 4));
	int r = ch_wait(ARB_OBS, off);
	if (r < 0)
		return r;
	return rd32(ARB_OBS + off + CH_RDATA0) & 0xff;
}
