/*
 * Sweet GPU bring-up.
 *   sweetgpu=1  read-only rail probe (PM8005 S1/S4, PM8998 S9)
 *   sweetgpu=2  also set both VDD_GFX phases to 900mV and enable them
 *   sweetgpupause=N  hold the console N seconds (camera read)
 *
 * PM8005 S1/S4 expose the common2 regulator layout on this board:
 *   0x40 voltage LSB, 0x41 voltage MSB (mV, 0x02F0 = 752mV),
 *   0x45 mode (6 = auto), 0x46 bit7 = enable.
 */
#if defined(ARM64_BOARD_CONFIG_SWEET)
#include <sys/param.h>
#include <sys/systm.h>
#include <kern/clock.h>
#include <machine/machine_routines.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <pexpert/pexpert.h>
#include <pexpert/device_tree.h>
#include <string.h>
extern void flush_dcache(vm_offset_t, unsigned, int);
extern ppnum_t pmap_find_phys(pmap_t, addr64_t);
extern addr64_t kvtophys(vm_offset_t);
extern vm_map_address_t phystokv(uint64_t pa);

extern int sweet_spmi_init(void);
extern int sweet_spmi_read_ppid(unsigned ppid, unsigned addr, uint8_t *out);
extern int sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value);
extern int sweet_spmi_describe(unsigned ppid);
extern int sweet_spmi_write_ppid(unsigned ppid, unsigned addr, uint8_t value);

#define PON_PPID 0x008u
#define PON_ROUTE 0x88fu /* PON_ROUTE_ARMED */
#define PON_SCRATCH 0x088eu

/* a5xx CP registers (dword offsets into GPU 0x5000000). */
#define R_CP_RB_BASE 0x0800u
#define R_CP_RB_BASE_HI 0x0801u
#define R_CP_RB_CNTL 0x0802u
#define R_CP_RB_RPTR 0x0806u
#define R_CP_RB_WPTR 0x0807u
#define R_CP_ROQ_THRESHOLDS_1 0x081fu
#define R_CP_ROQ_THRESHOLDS_2 0x0820u
#define R_CP_MEQ_THRESHOLDS 0x0825u
#define R_CP_MERCIU_SIZE 0x0826u
#define R_CP_PFP_ME_CNTL 0x0832u
#define R_CP_CHICKEN_DBG 0x0833u
#define R_CP_PFP_INSTR_BASE 0x0835u
#define R_CP_ME_INSTR_BASE 0x0838u
#define R_CP_HW_FAULT 0x0b1au
#define R_CP_INTERRUPT_STATUS 0x0b18u
#define R_RBBM_AHB_CNTL0 0x0093u
#define R_RBBM_AHB_CNTL1 0x0094u
#define R_RBBM_AHB_CNTL2 0x0095u
#define R_RBBM_PERFCTR_GPU_BUSY_MASKED 0x046fu
#define R_UCHE_CACHE_WAYS 0x0e96u
#define R_UCHE_TRAP_BASE 0x0e89u
#define R_UCHE_WRITE_THRU_BASE 0x0e87u
#define R_UCHE_GMEM_RANGE_MIN 0x0e8bu
#define R_UCHE_GMEM_RANGE_MAX 0x0e8du
#define R_UCHE_MODE_CNTL 0x0e81u
#define R_PC_DBG_ECO_CNTL 0x0d00u
#define R_TPL1_MODE_CNTL 0x0f02u
#define R_RB_MODE_CNTL 0x0cc6u
#define R_VPC_DBG_ECO_CNTL 0x0e60u
#define R_VBIF_RR_QOS_ARB 0x3049u
#define R_SECVID_TSB_CNTL 0xf803u
#define R_SECVID_TSB_BASE 0xf800u
#define R_SECVID_TSB_SIZE 0xf802u
#define R_SECVID_TSB_ADDR_MODE 0xf810u
#define R_GPMU_RBCCU_POWER_CNTL 0xa887u
#define R_GPMU_SP_POWER_CNTL 0xa881u
#define R_GPMU_RBCCU_PWR_CLK_STATUS 0xa88du
#define R_GPMU_SP_PWR_CLK_STATUS 0xa88bu
#define R_CP_RB_RPTR_ADDR_LO 0x0804u
#define R_CP_RB_RPTR_ADDR_HI 0x0805u

#define CP_ME_INIT_OP 0x48u
#define CP_MEM_WRITE_OP 0x3du
#define CP_NOP_OP 0x10u

static uint32_t cp_ring[4096] __attribute__((aligned(16384)));
static uint32_t cp_scratch[4096] __attribute__((aligned(16384)));
static uint32_t cp_rb_n;

static int sweet_gpu_blob(const char *name, uint64_t *pa, uint32_t *len);
extern vm_offset_t ml_io_map(vm_offset_t, vm_size_t);

#define GPU_PAS_ID 13u
#define ZAP_PA 0x95200000u
#define ZAP_SIZE 0x00100000u
#define CP_SET_SECURE_MODE_OP 0x66u

static uint32_t zap_meta[4096] __attribute__((aligned(16384)));
static int zap_ok;

struct zap_phdr {
	uint32_t type, offset, vaddr, paddr, filesz, memsz, flags, align;
};

/* Qualcomm SCM call, ARM SMCCC (standard, SMC64, SIP owner). */
static int64_t
sweet_scm_call(uint32_t svc, uint32_t cmd, uint32_t arginfo, uint64_t a,
    uint64_t b, uint64_t c, uint64_t *r1)
{
	uint64_t fn = (1ull << 30) | (2ull << 24) | ((uint64_t)svc << 8) | cmd;
	for (int busy = 0;; busy++) {
		uint64_t a0 = fn, a6 = 0;
		register uint64_t x0 __asm__("x0"), x1 __asm__("x1");
		register uint64_t x2 __asm__("x2"), x3 __asm__("x3");
		register uint64_t x4 __asm__("x4"), x5 __asm__("x5");
		register uint64_t x6 __asm__("x6"), x7 __asm__("x7");
		do {
			x0 = a0; x1 = arginfo; x2 = a; x3 = b; x4 = c; x5 = 0;
			x6 = a6; x7 = 0;
			__asm__ volatile("smc #0"
			    : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
			      "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
			    :
			    : "x8", "x9", "x10", "x11", "x12", "x13", "x14",
			      "x15", "x16", "x17", "memory");
			a0 = 1;
			a6 = x6;
		} while (x0 == 1);
		if ((int64_t)x0 != -12 || busy >= 20) {
			if (r1) {
				*r1 = x1;
			}
			return (int64_t)x0;
		}
		delay(30000);
	}
}


static uint32_t cp_pkt7(uint32_t op, uint32_t cnt);
static void cp_emit(uint32_t v);
static void sweet_gpu_step(uint8_t code);
static uint32_t cp_pkt4(uint32_t reg, uint32_t cnt);

static void gpu_wr(vm_offset_t base, unsigned off, uint32_t v);
static uint32_t gpu_rd(vm_offset_t base, unsigned off);
static uint64_t gpu_alloc(vm_offset_t *va, vm_size_t size);

/* A540 startup, ported from research/ios_sagit/drivers/gpu/QcomGPU/
 * AdrenoStartup.h (A540-only adaptation of Linux a5xx_gpu.c). */
static uint32_t a540_ring[8192] __attribute__((aligned(16384)));
static uint32_t a540_scratch[4096] __attribute__((aligned(16384)));
static int a540_want_gpmu;
static uint64_t smmu_l1[2048] __attribute__((aligned(16384)));
static uint64_t smmu_l2b[512] __attribute__((aligned(4096)));

#define A540_ME_HALT 0x303u
#define A540_RING_CNTL (12u | (2u << 8) | (1u << 27))
#define A540_COMPLETION_IRQ (1u << 20)
#define A540_ERROR_IRQS (0x7eu | (1u << 9) | (1u << 23) | (1u << 24) | \
    (1u << 25) | (1u << 28))
#define A540_OWNED_IRQS (A540_COMPLETION_IRQ | A540_ERROR_IRQS)

static uint32_t
a540_prot(uint32_t base, unsigned log2len)
{
	return 0x60000000u | (log2len << 24) | (base << 2);
}

static int
sweet_a540_start(int secure_cb)
{
	vm_offset_t gpu = ml_io_map(0x05000000u, 0x00040000u);
	if (!gpu) {
		printf("SWEET GPU: a540 no gpu map\n");
		return 0;
	}
	uint64_t pm4 = 0, pfp = 0;
	uint32_t pm4_len = 0, pfp_len = 0;
	if (!sweet_gpu_blob("GPU-PM4", &pm4, &pm4_len) ||
	    !sweet_gpu_blob("GPU-PFP", &pfp, &pfp_len) ||
	    pm4_len <= 4 || pfp_len <= 4) {
		printf("SWEET GPU: a540 no fw\n");
		return 0;
	}
	vm_offset_t pm4_va = 0, pfp_va = 0;
	uint64_t pm4_base = gpu_alloc(&pm4_va, 0x8000);
	uint64_t pfp_base = gpu_alloc(&pfp_va, 0x8000);
	if (!pm4_base || !pfp_base) {
		printf("SWEET GPU: a540 fw alloc failed\n");
		return 0;
	}
	memcpy((void *)pm4_va, (const void *)phystokv(pm4 + 4), pm4_len - 4);
	memcpy((void *)pfp_va, (const void *)phystokv(pfp + 4), pfp_len - 4);
	uint32_t pm4_bytes = pm4_len - 4, pfp_bytes = pfp_len - 4;
	printf("SWEET GPU: a540 aligned fw pm4=%#llx pfp=%#llx\n",
	    pm4_base, pfp_base);

	/* Pre-auth the SMMU is secure-owned: touching CB1/2 or sACR/sCTLR
	 * (0x68/0x70) trips a secure violation and resets the SoC. */
	vm_offset_t smmu = secure_cb ? ml_io_map(0x05040000u, 0x00010000u) : 0;
	uint64_t pt = smmu ? (uint64_t)kvtophys((vm_offset_t)smmu_l1) : 0;
	if (secure_cb && smmu && pt) {
		vm_offset_t gr1 = smmu + 0x1000u;
		for (uint32_t cb = 1; cb <= 2; cb++) {
			vm_offset_t cbb = smmu + 0x8000u + cb * 0x1000u;
			gpu_wr(cbb, 0x0, 0);
			gpu_wr(cbb, 0x10, (7u << 15) | 1u);
			gpu_wr(cbb, 0x30, 28u | (1u << 23));
			gpu_wr(cbb, 0x38, 0x44u);
			gpu_wr(cbb, 0x20, (uint32_t)pt);
			gpu_wr(cbb, 0x24, (uint32_t)(pt >> 32));
			gpu_wr(cbb, 0x58, 0xffffffffu);
			gpu_wr(cbb, 0x0, 7u);
			gpu_wr(gr1, 4u * cb, 1u << 16);
			gpu_wr(gr1, 0x800u + 4u * cb, 1u);
			gpu_wr(smmu, 0x800u + 4u * cb, (1u << 31) | cb);
			gpu_wr(smmu, 0xc00u + 4u * cb, cb);
		}
		gpu_wr(smmu, 0x68, 0);
		gpu_wr(smmu, 0x70, 0);
		printf("SWEET GPU: a540 secure cb SMR0=%#x SMR1=%#x SMR2=%#x S2CR0=%#x S2CR1=%#x S2CR2=%#x CB2=%#x CBAR2=%#x\n",
		    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0x804), gpu_rd(smmu, 0x808),
		    gpu_rd(smmu, 0xc00), gpu_rd(smmu, 0xc04), gpu_rd(smmu, 0xc08),
		    gpu_rd(smmu, 0xa000), gpu_rd(gr1, 0x8));
	}
	if (!secure_cb)
		printf("SWEET GPU: a540 secure cb skipped (pre-auth)\n");
	vm_offset_t ring_va = 0, scratch_va = 0;
	uint64_t ring_pa = gpu_alloc(&ring_va, 32768);
	uint64_t scratch_pa = gpu_alloc(&scratch_va, 16384);
	if (!ring_pa || !scratch_pa) {
		printf("SWEET GPU: a540 contig alloc failed\n");
		return 0;
	}
	uint32_t *ring = (uint32_t *)ring_va;
	uint32_t *scratch = (uint32_t *)scratch_va;
	memset(ring, 0, 32768);
	memset(scratch, 0, 16384);
	printf("SWEET GPU: a540 ring=%#llx scratch=%#llx pm4=%#llx pfp=%#llx\n",
	    ring_pa, scratch_pa, pm4_base, pfp_base);

	/* reset: halt ME/VBIF, RBBM soft reset, GPMU CM3 reset. */
	gpu_wr(gpu, 0x0038u * 4u, 0u);			/* InterruptMask */
	gpu_wr(gpu, 0x0832u * 4u, A540_ME_HALT);	/* MEControl halt */
	gpu_wr(gpu, 0x3080u * 4u, 15u);			/* VBIF halt */
	int halted = 0;
	for (int i = 0; i < 100; ++i) {
		if ((gpu_rd(gpu, 0x0832u * 4u) & A540_ME_HALT) == A540_ME_HALT &&
		    (gpu_rd(gpu, 0x3081u * 4u) & 15u) == 15u) {
			halted = 1; break;
		}
		delay(1000);
	}
	printf("SWEET GPU: a540 halt=%d ME=%#x VBIF=%#x trust=%#x\n",
	    halted, gpu_rd(gpu, 0x0832u * 4u), gpu_rd(gpu, 0x3081u * 4u),
	    gpu_rd(gpu, 0xf400u * 4u));
	sweet_gpu_step(0xb2);
	gpu_wr(gpu, 0x0043u * 4u, 1u);			/* RBBM_SW_RESET */
	(void)gpu_rd(gpu, 0x0043u * 4u);
	gpu_wr(gpu, 0x0043u * 4u, 0u);
	delay(100);
	sweet_gpu_step(0xb3);
	gpu_wr(gpu, 0xa8d8u * 4u, 1u);			/* GPMU CM3 reset */
	gpu_wr(gpu, 0xa8e0u * 4u, 0u);			/* GPMU ready */
	gpu_wr(gpu, 0xa8e1u * 4u, 0u);			/* GPMU error */
	printf("SWEET GPU: a540 reset ME=%#x VBIF=%#x RB=%u/%u GPMU=%#x\n",
	    gpu_rd(gpu, 0x0832u * 4u), gpu_rd(gpu, 0x3081u * 4u),
	    gpu_rd(gpu, 0x0806u * 4u), gpu_rd(gpu, 0x0807u * 4u),
	    gpu_rd(gpu, 0xa8d8u * 4u));

	/* A540 configure (AdrenoStartup::configure, HGCG table omitted). */
	gpu_wr(gpu, 0x3049u * 4u, 3u);			/* VBIF RR QoS */
	gpu_wr(gpu, 0x302au * 4u, 9u);			/* VBIF gate off wrreq */
	gpu_wr(gpu, 0x046fu * 4u, 0xffffffffu);
	gpu_wr(gpu, 0x0093u * 4u, 1u);
	gpu_wr(gpu, 0x002fu * 4u, 0x4000ffffu);		/* hang int */
	gpu_wr(gpu, 0x0464u * 4u, 1u);
	gpu_wr(gpu, 0x0bb0u * 4u, 0u);
	gpu_wr(gpu, 0x046bu * 4u, 6u);
	gpu_wr(gpu, 0x0e96u * 4u, 2u);
	gpu_wr(gpu, 0x0e89u * 4u, 0xffff0000u);
	gpu_wr(gpu, 0x0e8au * 4u, 0x1ffffu);
	gpu_wr(gpu, 0x0e87u * 4u, 0xffff0000u);
	gpu_wr(gpu, 0x0e88u * 4u, 0x1ffffu);
	gpu_wr(gpu, 0x0e8bu * 4u, 0x100000u);
	gpu_wr(gpu, 0x0e8cu * 4u, 0u);
	gpu_wr(gpu, 0x0e8du * 4u, 0x1fffffu);
	gpu_wr(gpu, 0x0e8eu * 4u, 0u);
	gpu_wr(gpu, 0x0825u * 4u, 0x40u);
	gpu_wr(gpu, 0x0826u * 4u, 0x400u);		/* A540 MERCIU */
	gpu_wr(gpu, 0x0820u * 4u, 0x80000060u);
	gpu_wr(gpu, 0x081fu * 4u, 0x40201b16u);
	gpu_wr(gpu, 0x0d00u * 4u, (0x400u << 11) | (0x300u << 22));
	gpu_wr(gpu, 0x0e81u * 4u, 1u << 29);
	gpu_wr(gpu, 0x0833u * 4u, 0x02000000u);
	gpu_wr(gpu, 0x0094u * 4u, 0xa6ffffffu);
	gpu_wr(gpu, 0x0095u * 4u, 0x3fu);
	gpu_wr(gpu, 0x0f02u * 4u, 2u << 7);
	gpu_wr(gpu, 0x0cc6u * 4u, 2u << 1);
	gpu_wr(gpu, 0x0e8fu * 4u, 2u);
	gpu_wr(gpu, 0x08a0u * 4u, 7u);			/* CP_PROTECT_CNTL */
	gpu_wr(gpu, 0x0e60u * 4u,
	    gpu_rd(gpu, 0x0e60u * 4u) | (1u << 10) | (1u << 23));
	gpu_wr(gpu, 0x0e04u * 4u,
	    gpu_rd(gpu, 0x0e04u * 4u) & ~(1u << 18));
	static const uint32_t prot[] = {
		0x0880u + 0, 0x0880u + 1, 0x0880u + 2, 0x0880u + 3,
		0x0880u + 4, 0x0880u + 5, 0x0880u + 6, 0x0880u + 7,
		0x0880u + 8, 0x0880u + 9, 0x0880u + 10, 0x0880u + 11,
		0x0880u + 12, 0x0880u + 13, 0x0880u + 14, 0x0880u + 15,
		0x0880u + 16, 0x0880u + 17,
	};
	const uint32_t protv[] = {
		a540_prot(0x4, 2), a540_prot(0x8, 3), a540_prot(0x10, 4),
		a540_prot(0x20, 5), a540_prot(0x40, 6), a540_prot(0x80, 6),
		a540_prot(0xf800, 4), a540_prot(0xf400, 1),
		a540_prot(0x800, 6), a540_prot(0x840, 3), a540_prot(0x880, 5),
		a540_prot(0xaa0, 0), a540_prot(0xcc0, 0), a540_prot(0xcf0, 1),
		a540_prot(0xe68, 3), a540_prot(0xe70, 4), a540_prot(0xe80, 4),
		a540_prot(0x10000, 15),
	};
	for (unsigned i = 0; i < sizeof(prot) / sizeof(prot[0]); i++) {
		gpu_wr(gpu, prot[i] * 4u, protv[i]);
	}
	gpu_wr(gpu, 0xf803u * 4u, 0u);
	gpu_wr(gpu, 0xf800u * 4u, 0u);
	gpu_wr(gpu, 0xf801u * 4u, 0u);
	gpu_wr(gpu, 0xf802u * 4u, 0u);
	static const uint16_t modes[] = {
		0x0860u, 0x0bc1u, 0x0c81u, 0x0cc5u, 0x0d01u, 0x0e05u,
		0x0e41u, 0x0e61u, 0x0e80u, 0x0ec1u, 0x0f01u, 0xf810u,
	};
	for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
		gpu_wr(gpu, modes[i] * 4u, 1u);
	}
	/* A540 hardware clock gating (AdrenoStartup::hardwareClockGating). */
	gpu_wr(gpu, 104u * 4u, 0x02222222u);
	gpu_wr(gpu, 105u * 4u, 0x02222222u);
	gpu_wr(gpu, 106u * 4u, 0x02222222u);
	gpu_wr(gpu, 107u * 4u, 0x02222222u);
	gpu_wr(gpu, 108u * 4u, 0x02222220u);
	gpu_wr(gpu, 109u * 4u, 0x02222220u);
	gpu_wr(gpu, 110u * 4u, 0x02222220u);
	gpu_wr(gpu, 111u * 4u, 0x02222220u);
	gpu_wr(gpu, 112u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 113u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 114u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 115u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 116u * 4u, 0x00000080u);
	gpu_wr(gpu, 117u * 4u, 0x00000080u);
	gpu_wr(gpu, 118u * 4u, 0x00000080u);
	gpu_wr(gpu, 119u * 4u, 0x00000080u);
	gpu_wr(gpu, 74u * 4u, 0x22222222u);
	gpu_wr(gpu, 75u * 4u, 0x22222222u);
	gpu_wr(gpu, 76u * 4u, 0x22222222u);
	gpu_wr(gpu, 77u * 4u, 0x22222222u);
	gpu_wr(gpu, 78u * 4u, 0x22222222u);
	gpu_wr(gpu, 79u * 4u, 0x22222222u);
	gpu_wr(gpu, 80u * 4u, 0x22222222u);
	gpu_wr(gpu, 81u * 4u, 0x22222222u);
	gpu_wr(gpu, 82u * 4u, 0x00002222u);
	gpu_wr(gpu, 83u * 4u, 0x00002222u);
	gpu_wr(gpu, 84u * 4u, 0x00002222u);
	gpu_wr(gpu, 85u * 4u, 0x00002222u);
	gpu_wr(gpu, 176u * 4u, 0x77777777u);
	gpu_wr(gpu, 177u * 4u, 0x77777777u);
	gpu_wr(gpu, 178u * 4u, 0x77777777u);
	gpu_wr(gpu, 179u * 4u, 0x77777777u);
	gpu_wr(gpu, 180u * 4u, 0x77777777u);
	gpu_wr(gpu, 181u * 4u, 0x77777777u);
	gpu_wr(gpu, 182u * 4u, 0x77777777u);
	gpu_wr(gpu, 183u * 4u, 0x77777777u);
	gpu_wr(gpu, 184u * 4u, 0x00007777u);
	gpu_wr(gpu, 185u * 4u, 0x00007777u);
	gpu_wr(gpu, 186u * 4u, 0x00007777u);
	gpu_wr(gpu, 187u * 4u, 0x00007777u);
	gpu_wr(gpu, 164u * 4u, 0x11111111u);
	gpu_wr(gpu, 165u * 4u, 0x11111111u);
	gpu_wr(gpu, 166u * 4u, 0x11111111u);
	gpu_wr(gpu, 167u * 4u, 0x11111111u);
	gpu_wr(gpu, 168u * 4u, 0x11111111u);
	gpu_wr(gpu, 169u * 4u, 0x11111111u);
	gpu_wr(gpu, 170u * 4u, 0x11111111u);
	gpu_wr(gpu, 171u * 4u, 0x11111111u);
	gpu_wr(gpu, 172u * 4u, 0x00001111u);
	gpu_wr(gpu, 173u * 4u, 0x00001111u);
	gpu_wr(gpu, 174u * 4u, 0x00001111u);
	gpu_wr(gpu, 175u * 4u, 0x00001111u);
	gpu_wr(gpu, 90u * 4u, 0x22222222u);
	gpu_wr(gpu, 91u * 4u, 0x22222222u);
	gpu_wr(gpu, 92u * 4u, 0x22222222u);
	gpu_wr(gpu, 93u * 4u, 0x00222222u);
	gpu_wr(gpu, 94u * 4u, 0x00444444u);
	gpu_wr(gpu, 95u * 4u, 0x00000002u);
	gpu_wr(gpu, 120u * 4u, 0x22222222u);
	gpu_wr(gpu, 121u * 4u, 0x22222222u);
	gpu_wr(gpu, 122u * 4u, 0x22222222u);
	gpu_wr(gpu, 123u * 4u, 0x22222222u);
	gpu_wr(gpu, 124u * 4u, 0x00222222u);
	gpu_wr(gpu, 125u * 4u, 0x00222222u);
	gpu_wr(gpu, 126u * 4u, 0x00222222u);
	gpu_wr(gpu, 127u * 4u, 0x00222222u);
	gpu_wr(gpu, 130u * 4u, 0x00022220u);
	gpu_wr(gpu, 131u * 4u, 0x00022220u);
	gpu_wr(gpu, 132u * 4u, 0x00022220u);
	gpu_wr(gpu, 133u * 4u, 0x00022220u);
	gpu_wr(gpu, 138u * 4u, 0x05522222u);
	gpu_wr(gpu, 139u * 4u, 0x00505555u);
	gpu_wr(gpu, 134u * 4u, 0x04040404u);
	gpu_wr(gpu, 135u * 4u, 0x04040404u);
	gpu_wr(gpu, 136u * 4u, 0x04040404u);
	gpu_wr(gpu, 137u * 4u, 0x04040404u);
	gpu_wr(gpu, 128u * 4u, 0x07444044u);
	gpu_wr(gpu, 140u * 4u, 0x00000002u);
	gpu_wr(gpu, 141u * 4u, 0x00000002u);
	gpu_wr(gpu, 142u * 4u, 0x00000002u);
	gpu_wr(gpu, 143u * 4u, 0x00000002u);
	gpu_wr(gpu, 129u * 4u, 0x00010011u);
	gpu_wr(gpu, 99u * 4u, 0x04222222u);
	gpu_wr(gpu, 96u * 4u, 0x02222222u);
	gpu_wr(gpu, 145u * 4u, 0x00002222u);
	gpu_wr(gpu, 100u * 4u, 0x00000000u);
	gpu_wr(gpu, 98u * 4u, 0x04104004u);
	gpu_wr(gpu, 144u * 4u, 0x00000000u);
	gpu_wr(gpu, 102u * 4u, 0x00000000u);
	gpu_wr(gpu, 101u * 4u, 0x00004000u);
	gpu_wr(gpu, 97u * 4u, 0x00000200u);
	gpu_wr(gpu, 146u * 4u, 0x00002222u);
	gpu_wr(gpu, 201u * 4u, 0x770u);
	gpu_wr(gpu, 202u * 4u, 4u);
	gpu_wr(gpu, 103u * 4u, 0xaaa8aa00u);
	gpu_wr(gpu, 1331u * 4u, 0x182u);

	sweet_gpu_step(0xb4);
	/* bases + ring state */
	gpu_wr(gpu, 0x0838u * 4u, (uint32_t)pm4_base);
	gpu_wr(gpu, 0x0839u * 4u, (uint32_t)(pm4_base >> 32));
	gpu_wr(gpu, 0x0835u * 4u, (uint32_t)pfp_base);
	gpu_wr(gpu, 0x0836u * 4u, (uint32_t)(pfp_base >> 32));
	gpu_wr(gpu, 0x0800u * 4u, (uint32_t)ring_pa);
	gpu_wr(gpu, 0x0801u * 4u, (uint32_t)(ring_pa >> 32));
	gpu_wr(gpu, 0x083bu * 4u, 0u);
	gpu_wr(gpu, 0x0802u * 4u, A540_RING_CNTL);
	gpu_wr(gpu, 0x0807u * 4u, 0u);
	sweet_gpu_step(0xb5);
	gpu_wr(gpu, 0x0038u * 4u, 0u);
	gpu_wr(gpu, 0x0037u * 4u, A540_OWNED_IRQS);
	printf("SWEET GPU: a540 cfg ME=%#x rctl=%#x rptr=%u wptr=%u cp_mode=%#x prot=%#x\n",
	    gpu_rd(gpu, 0x0832u * 4u), gpu_rd(gpu, 0x0802u * 4u),
	    gpu_rd(gpu, 0x0806u * 4u), gpu_rd(gpu, 0x0807u * 4u),
	    gpu_rd(gpu, 0x0860u * 4u), gpu_rd(gpu, 0x08a0u * 4u));

	/* start: clear VBIF/ME halt, then A540 ME_INIT (no A530 0xb). */
	flush_dcache(pm4_base, pm4_bytes, TRUE);
	flush_dcache(pfp_base, pfp_bytes, TRUE);
	gpu_wr(gpu, 0x3080u * 4u, 0u);
	gpu_wr(gpu, 0x0832u * 4u, 0u);
	uint32_t n = 0;
	ring[n++] = cp_pkt7(CP_ME_INIT_OP, 8);
	static const uint32_t init[] = { 0x2fu, 3u, 0x20000000u, 0u, 0u, 0u, 0u, 0u };
	for (unsigned i = 0; i < 8; i++) {
		ring[n++] = init[i];
	}
	ring[n++] = cp_pkt7(CP_MEM_WRITE_OP, 3);
	ring[n++] = (uint32_t)scratch_pa;
	ring[n++] = (uint32_t)(scratch_pa >> 32);
	ring[n++] = 0xc0ffee01u;
	ring[n++] = cp_pkt7(CP_NOP_OP, 0);
	flush_dcache(ring_pa, 32768, TRUE);
	gpu_wr(gpu, 0x0807u * 4u, n);
	sweet_gpu_step(0xb6);
	/* v200 gpmu-before-wait: release GPMU before checking CP fetch. */
	int gready = 0;
	if (a540_want_gpmu) {
		uint64_t gpmu_pa = 0;
		uint32_t gpmu_len = 0;
		if (!sweet_gpu_blob("GPU-GPMU", &gpmu_pa, &gpmu_len) ||
		    gpmu_len < 64) {
			printf("SWEET GPU: a540 gpmu blob missing\n");
			return 1;
		}
		const uint32_t *g = (const uint32_t *)phystokv(gpmu_pa);
		uint32_t remaining = g[0], metadata = g[2];
		if (g[1] != 2 || remaining < 3 ||
		    remaining >= gpmu_len / 4 || metadata > remaining - 3) {
			printf("SWEET GPU: a540 gpmu header %#x/%#x/%#x\n",
			    remaining, g[1], metadata);
			return 1;
		}
		uint32_t first = metadata + 3;
		uint32_t instructions = remaining - metadata - 2;
		uint32_t words = instructions + (instructions + 126) / 127;
		vm_offset_t cmd_va = 0;
		uint64_t cmd_pa = gpu_alloc(&cmd_va, 0x4000);
		if (!cmd_pa || words > 0x1000) {
			printf("SWEET GPU: a540 gpmu cmd alloc failed\n");
			return 1;
		}
		uint32_t *cmd = (uint32_t *)cmd_va;
		uint32_t off = 0, cnt = 0;
		while (off < instructions) {
			uint32_t chunk = instructions - off > 127 ? 127 :
			    instructions - off;
			uint32_t reg = 0x8800u + off;
			cmd[cnt++] = cp_pkt4(reg, chunk);
			for (uint32_t i = 0; i < chunk; i++) {
				cmd[cnt++] = g[first + off + i];
			}
			off += chunk;
		}
		flush_dcache(cmd_pa, 0x4000, TRUE);
		printf("SWEET GPU: a540 gpmu words=%u instr=%u cmd=%#llx\n",
		    words, instructions, cmd_pa);
		/* AGC shared-register protocol + GPMU power registers. */
		const uint32_t msg = 0x9807u, payload = msg + 5u;
		gpu_wr(gpu, 0xac60u * 4u, 0x80000000u);
		gpu_wr(gpu, 0xac80u * 4u, 0x80000000u | 6000u);
		for (uint32_t i = 0; i < 36; i++) {
			gpu_wr(gpu, (payload + i) * 4u, 0u);
		}
		gpu_wr(gpu, msg * 4u, 0x80000001u);
		gpu_wr(gpu, (msg + 1u) * 4u, 1u);
		gpu_wr(gpu, payload * 4u, 5448u);
		gpu_wr(gpu, (payload + 1u) * 4u, 1u);
		gpu_wr(gpu, (payload + 2u) * 4u, 800u);	/* 800 mV */
		gpu_wr(gpu, (payload + 3u) * 4u, 180u);	/* 180 MHz */
		gpu_wr(gpu, (payload + 34u) * 4u,
		    (1u << 24) | (1u << 17) | (2u << 8));
		gpu_wr(gpu, (payload + 35u) * 4u, ~0x303u);
		gpu_wr(gpu, (msg + 3u) * 4u, 36u * 4u);
		gpu_wr(gpu, 0x9805u * 4u, 0xbabefaceu);
		gpu_wr(gpu, 0xa892u * 4u, 0x7fu);
		gpu_wr(gpu, 0xa894u * 4u, 0u);
		gpu_wr(gpu, 0xa893u * 4u, 0xa0080u);
		gpu_wr(gpu, 0xa891u * 4u, 0x600040u);
		/* Append the type4 burst after ME_INIT at ring[n]; cp_ring and
		 * cp_rb_n belong to the other CP test path and must not be touched. */
		ring[n++] = cp_pkt7(0x5fu, 1);
		ring[n++] = 0u;
		ring[n++] = cp_pkt7(0x3fu, 3);
		ring[n++] = (uint32_t)cmd_pa;
		ring[n++] = (uint32_t)(cmd_pa >> 32);
		ring[n++] = cnt;
		ring[n++] = cp_pkt7(0x5fu, 1);
		ring[n++] = 1u;
		flush_dcache(ring_pa, 32768, TRUE);
		gpu_wr(gpu, 0x0807u * 4u, n);
		gpu_wr(gpu, 0xa8d8u * 4u, 0u);		/* release CM3 */
		for (int i = 0; i < 300; ++i) {
			if (gpu_rd(gpu, 0xa8e0u * 4u) == 0xbabefaceu) {
				gready = 1; break;
			}
			delay(1000);
		}
		printf("SWEET GPU: a540 gpmu ready=%d (%#x/%#x) cm3=%#x\n",
		    gready, gpu_rd(gpu, 0xa8e0u * 4u),
		    gpu_rd(gpu, 0xa8e1u * 4u), gpu_rd(gpu, 0xa8d8u * 4u));
	}
	sweet_gpu_step(0xb7);
	int consumed = 0;
	for (int i = 0; i < 200; ++i) {
		if (gpu_rd(gpu, 0x0806u * 4u) == n) {
			consumed = 1; break;
		}
		delay(1000);
	}
	delay(1000);
	flush_dcache(scratch_pa, PAGE_SIZE, TRUE);
	uint32_t got = scratch[0];
	printf("SWEET GPU: a540 consumed=%d RPTR=%u WPTR=%u scratch=%#x RBBM=%#x FAULT=%#x gpmu=%d\n",
	    consumed, gpu_rd(gpu, 0x0806u * 4u), n, got,
	    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, 0x0b1au * 4u), gready);
	if (a540_want_gpmu) {
		/* v204: prove the CP is alive after GPMU by writing a second
		 * value to the same scratch word. */
		ring[n++] = cp_pkt7(CP_MEM_WRITE_OP, 3);
		ring[n++] = (uint32_t)scratch_pa;
		ring[n++] = (uint32_t)(scratch_pa >> 32);
		ring[n++] = 0xc0ffee02u;
		ring[n++] = cp_pkt7(CP_NOP_OP, 0);
		flush_dcache(ring_pa, 32768, TRUE);
		gpu_wr(gpu, 0x0807u * 4u, n);
		int consumed2 = 0;
		for (int i = 0; i < 200; ++i) {
			if (gpu_rd(gpu, 0x0806u * 4u) == n) { consumed2 = 1; break; }
			delay(1000);
		}
		delay(1000);
		flush_dcache(scratch_pa, PAGE_SIZE, TRUE);
		printf("SWEET GPU: a540 post-gpmu consumed=%d RPTR=%u WPTR=%u scratch=%#x\n",
		    consumed2, gpu_rd(gpu, 0x0806u * 4u), n, scratch[0]);
	}
	printf("SWEET GPU: a540 diag ME_CNTL=%#x trust=%#x FSR=%#x\n",
	    gpu_rd(gpu, 0x0860u * 4u), gpu_rd(gpu, 0xf400u * 4u),
	    smmu ? gpu_rd(smmu, 0x58u) : 0u);
	printf("SWEET GPU: a540 rb rptr=%u wptr=%u cntl=%#x\n",
	    gpu_rd(gpu, 0x0806u * 4u), gpu_rd(gpu, 0x0807u * 4u),
	    gpu_rd(gpu, 0x0802u * 4u));
	printf("SWEET GPU: a540 bases pfp=%#x/%#x pm4=%#x/%#x prot=%#x mode=%#x\n",
	    gpu_rd(gpu, 0x0835u * 4u), gpu_rd(gpu, 0x0836u * 4u),
	    gpu_rd(gpu, 0x0838u * 4u), gpu_rd(gpu, 0x0839u * 4u),
	    gpu_rd(gpu, 0x08a0u * 4u), gpu_rd(gpu, 0x0860u * 4u));
	printf("SWEET GPU: a540 gpmu ready=%#x err=%#x cm3=%#x\n",
	    gpu_rd(gpu, 0xa8e0u * 4u), gpu_rd(gpu, 0xa8e1u * 4u),
	    gpu_rd(gpu, 0xa8d8u * 4u));
	sweet_gpu_step(0xb8);
	{
		int h8 = 0;
		if (PE_parse_boot_argn("sweetgpuhalt", &h8, (int)sizeof(h8)) &&
		    h8 == 1) {
			printf("SWEET GPU: halt after b8; WDT/auto-reset recovers\n");
			for (;;)
				__asm__ volatile("wfe" ::: "memory");
		}
	}
	return (consumed && got == 0xc0ffee01u) || gready;


}

static int
sweet_zap_load(void)
{
	uint64_t mdt = 0, seg = 0;
	uint32_t mdt_len = 0, seg_len = 0;
	uint64_t meta = (uint64_t)kvtophys((vm_offset_t)zap_meta);

	if (!sweet_gpu_blob("GPU-ZAP-MDT", &mdt, &mdt_len) ||
	    !sweet_gpu_blob("GPU-ZAP-SEG", &seg, &seg_len)) {
		printf("SWEET GPU: zap blobs missing\n");
		return 0;
	}
	if (mdt_len < 0x34 || mdt_len > PAGE_SIZE) {
		printf("SWEET GPU: zap mdt size %u\n", mdt_len);
		return 0;
	}
	memcpy((void *)zap_meta, (const void *)phystokv(mdt), mdt_len);
	const uint8_t *m = (const uint8_t *)zap_meta;
	uint32_t phoff = *(const uint32_t *)(m + 0x1c);
	uint16_t phnum = *(const uint16_t *)(m + 0x2c);
	uint32_t lo = ~0u, hi = 0, filesz = 0, memsz = 0, nload = 0;
	for (uint32_t i = 0; i < phnum && phoff + (i + 1) * 32 <= mdt_len; i++) {
		const struct zap_phdr *ph =
		    (const struct zap_phdr *)(m + phoff + i * 32);
		if (ph->type != 1 || ((ph->flags >> 24) & 7) == 2 ||
		    ph->memsz == 0) {
			continue;
		}
		nload++;
		lo = ph->paddr;
		hi = (ph->paddr + ph->memsz + 0xfff) & ~0xfffu;
		filesz = ph->filesz;
		memsz = ph->memsz;
	}
	if (nload != 1 || filesz != seg_len || memsz > ZAP_SIZE) {
		printf("SWEET GPU: zap layout n=%u filesz=%u seg=%u memsz=%u\n",
		    nload, filesz, seg_len, memsz);
		return 0;
	}
	flush_dcache((vm_offset_t)zap_meta, PAGE_SIZE, FALSE);
	uint64_t r1 = 0;
	int64_t r = sweet_scm_call(2, 1, 2u | (2u << 6), GPU_PAS_ID, meta, 0, &r1);
	printf("SWEET GPU: zap init_image -> %lld/%#llx\n", r, r1);
	if (r != 0 || r1 != 0) {
		return 0;
	}
	r = sweet_scm_call(2, 2, 3, GPU_PAS_ID, ZAP_PA, hi - lo, &r1);
	printf("SWEET GPU: zap mem_setup %#x+%#x -> %lld/%#llx\n",
	    ZAP_PA, hi - lo, r, r1);
	if (r != 0 || r1 != 0) {
		return 0;
	}
	vm_offset_t zap = ml_io_map(ZAP_PA, ZAP_SIZE);
	if (!zap) {
		printf("SWEET GPU: zap carveout map failed\n");
		return 0;
	}
	const uint32_t *src = (const uint32_t *)phystokv(seg);
	for (uint32_t off = 0; off < ((memsz + 3) & ~3u); off += 4) {
		*(volatile uint32_t *)(zap + off) =
		    off < seg_len ? src[off / 4] : 0;
	}
	__asm__ volatile("dsb sy" ::: "memory");
	r = sweet_scm_call(2, 5, 1, GPU_PAS_ID, 0, 0, &r1);
	printf("SWEET GPU: zap auth_and_reset -> %lld/%#llx\n", r, r1);
	return r == 0 && r1 == 0;
}

static int
sweet_gpu_blob(const char *name, uint64_t *pa, uint32_t *len)
{
	DTEntry node;
	uint64_t const *v;
	unsigned int size;

	if (SecureDTLookupEntry(NULL, "/chosen/memory-map", &node) != kSuccess ||
	    SecureDTGetProperty(node, name, (void const **)&v, &size) != kSuccess ||
	    size != 16) {
		return 0;
	}
	*pa = v[0];
	*len = (uint32_t)v[1];
	return 1;
}

static uint32_t
cp_parity(uint32_t v)
{
	return (0x9669u >> (0xfu & (v ^ (v >> 4) ^ (v >> 8) ^ (v >> 12) ^
	    (v >> 16) ^ (v >> 20) ^ (v >> 24) ^ (v >> 28)))) & 1u;
}

static uint32_t
cp_pkt7(uint32_t op, uint32_t cnt)
{
	return (7u << 28) | cnt | (cp_parity(cnt) << 15) |
	    ((op & 0x7fu) << 16) | (cp_parity(op) << 23);
}

static uint32_t
cp_pkt4(uint32_t reg, uint32_t cnt)
{
	return (4u << 28) | cnt | (cp_parity(cnt) << 7) |
	    ((reg & 0x3ffffu) << 8) | (cp_parity(reg) << 27);
}

static void
cp_emit(uint32_t v)
{
	cp_ring[cp_rb_n] = v;
	cp_rb_n = (cp_rb_n + 1) & 4095u;
}

static void
sweet_gpu_step(uint8_t code)
{
	int rc = sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, code);
	printf("SWEET GPU: step %#x rc=%d\n", code, rc);
}
extern vm_offset_t ml_io_map(vm_offset_t, vm_size_t);

#define GCC_PA 0x00100000u
#define GCC_SIZE 0x000b0000u
#define GCC_GPU_CFG_AHB_CBCR 0x00071004u
#define GPUCC_PA 0x05065000u
#define GPUCC_SIZE 0x00009000u

#define PM8998_S9_PPID 0x02cu
#define PM8005_S1_PPID 0x514u
#define PM8005_S4_PPID 0x51du


static int
sweet_gpu_reg(unsigned ppid, unsigned addr)
{
	uint8_t value = 0;

	if (sweet_spmi_read_ppid(ppid, addr, &value) != 0) {
		return -1;
	}
	return value;
}

static void
sweet_gpu_rail(const char *tag, unsigned ppid, unsigned base)
{
	int lsb = sweet_gpu_reg(ppid, base + 0x40);
	int msb = sweet_gpu_reg(ppid, base + 0x41);
	int mode = sweet_gpu_reg(ppid, base + 0x45);
	int en = sweet_gpu_reg(ppid, base + 0x46);

	printf("SWEET GPU: %s v=%dmV mode=%d en=%d\n", tag,
	    lsb >= 0 && msb >= 0 ? ((msb << 8) | lsb) : -1, mode, en);
}

static uint32_t
gpu_rd(vm_offset_t base, unsigned off)
{
	uint32_t v = *(volatile uint32_t *)(base + off);
	__asm__ volatile("dmb oshld" ::: "memory");
	return v;
}

static void
gpu_wr(vm_offset_t base, unsigned off, uint32_t v)
{
	*(volatile uint32_t *)(base + off) = v;
	__asm__ volatile("dsb osh" ::: "memory");
}

/* Physically contiguous, zeroed kernel memory; returns its PA (0 failure). */
static uint64_t
gpu_alloc(vm_offset_t *va, vm_size_t size)
{
	if (kmem_alloc_contig(kernel_map, va, size, PAGE_MASK, 0, 0,
	    KMA_DATA | KMA_ZERO | KMA_PERMANENT, VM_KERN_MEMORY_IOKIT) !=
	    KERN_SUCCESS) {
		return 0;
	}
	return ptoa_64(pmap_find_phys(kernel_pmap, (addr64_t)*va));
}

/* Host-side GPMU boot: write the firmware from the AHB side, run the
 * AGC handshake and release CM3. Returns 1 on BABEFACE. */
static int
sweet_gpmu_host_boot(vm_offset_t gpu)
{
	int wantg = 0;
	if (!PE_parse_boot_argn("sweetgpmu", &wantg, (int)sizeof(wantg)) ||
	    wantg != 1) {
		return 0;
	}
	if (PE_parse_boot_argn("sweetgpmu", &wantg,
	    (int)sizeof(wantg)) && wantg == 1) {
		uint64_t gpmu_pa = 0;
		uint32_t gpmu_len = 0;
		if (sweet_gpu_blob("GPU-GPMU", &gpmu_pa, &gpmu_len) &&
		    gpmu_len >= 64) {
			const uint32_t *g = (const uint32_t *)phystokv(gpmu_pa);
			uint32_t remaining = g[0], metadata = g[2];
			if (g[1] == 2 && remaining >= 3 &&
			    remaining < gpmu_len / 4 && metadata <= remaining - 3) {
				uint32_t first = metadata + 3;
				uint32_t instr = remaining - metadata - 2;
				gpu_wr(gpu, 0xa8d8u * 4u, 1u);
				gpu_wr(gpu, 0xa8e0u * 4u, 0u);
				gpu_wr(gpu, 0xa8e1u * 4u, 0u);
				for (uint32_t i = 0; i < instr; i++) {
					gpu_wr(gpu, (0x8800u + i) * 4u, g[first + i]);
				}
				printf("SWEET GPU: GPMU host load instr=%u rb=%#x/%#x\n",
				    instr, gpu_rd(gpu, 0x8800u * 4u),
				    gpu_rd(gpu, (0x8800u + instr - 1u) * 4u));
				const uint32_t msg = 0x9807u, payload = msg + 5u;
				gpu_wr(gpu, 0xac60u * 4u, 0x80000000u);
				gpu_wr(gpu, 0xac80u * 4u, 0x80000000u | 6000u);
				for (uint32_t i = 0; i < 36; i++) {
					gpu_wr(gpu, (payload + i) * 4u, 0u);
				}
				gpu_wr(gpu, msg * 4u, 0x80000001u);
				gpu_wr(gpu, (msg + 1u) * 4u, 1u);
				gpu_wr(gpu, payload * 4u, 5448u);
				gpu_wr(gpu, (payload + 1u) * 4u, 1u);
				gpu_wr(gpu, (payload + 2u) * 4u, 800u);
				gpu_wr(gpu, (payload + 3u) * 4u, 180u);
				gpu_wr(gpu, (payload + 34u) * 4u,
				    (1u << 24) | (1u << 17) | (2u << 8));
				gpu_wr(gpu, (payload + 35u) * 4u, ~0x303u);
				gpu_wr(gpu, (msg + 3u) * 4u, 36u * 4u);
				gpu_wr(gpu, 0x9805u * 4u, 0xbabefaceu);
				gpu_wr(gpu, 0xa892u * 4u, 0x7fu);
				gpu_wr(gpu, 0xa894u * 4u, 0u);
				gpu_wr(gpu, 0xa893u * 4u, 0xa0080u);
				gpu_wr(gpu, 0xa891u * 4u, 0x600040u);
				gpu_wr(gpu, 0xa8d8u * 4u, 0u);
				int gr = 0;
				for (int i = 0; i < 100; ++i) {
					if (gpu_rd(gpu, 0xa8e0u * 4u) == 0xbabefaceu) {
						gr = 1; break;
					}
					delay(1000);
				}
				printf("SWEET GPU: GPMU host ready=%d %#x/%#x cm3=%#x\n",
				    gr, gpu_rd(gpu, 0xa8e0u * 4u),
				    gpu_rd(gpu, 0xa8e1u * 4u),
				    gpu_rd(gpu, 0xa8d8u * 4u));
			} else {
				printf("SWEET GPU: GPMU header bad %#x/%#x/%#x\n",
				    remaining, g[1], metadata);
			}
		} else {
			printf("SWEET GPU: GPMU blob missing\n");
		}
	}
	return 0;
}

void
sweet_gpu_probe(void)
{
	int level = 0;
	int gpmu_ready = 0;

	if (!PE_parse_boot_argn("sweetgpu", &level, (int)sizeof(level)) ||
	    level < 1) {
		return;
	}
	printf("SWEET GPU: probe level %d\n", level);
	if (sweet_spmi_init() != 0) {
		printf("SWEET GPU: SPMI unavailable\n");
		return;
	}
	{
		/* Any later reset must land in fastboot, not Android. */
		int rr = sweet_spmi_write_ppid(PON_PPID, PON_ROUTE, 0x06);
		printf("SWEET GPU: PON route arm rc=%d\n", rr);
		uint8_t last = 0;
		if (sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &last) == 0) {
			printf("SWEET GPU: last step=%#x\n", last);
		}
		/* scratch write test */
		int wrc = sweet_spmi_write_ppid(PON_PPID, PON_SCRATCH, 0xa5);
		uint8_t rb = 0;
		int rrc = sweet_spmi_read_ppid(PON_PPID, PON_SCRATCH, &rb);
		printf("SWEET GPU: scratch write test rc=%d read=%d val=%#x\n",
		    wrc, rrc, rb);
		delay(15 * 1000000);
	}
	sweet_spmi_describe(PM8005_S1_PPID);
	sweet_spmi_describe(PM8005_S4_PPID);
	sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
	sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);

	if (level >= 2) {
		/* One variable: enable at the bootloader's existing 752mV. */
		int rc1 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1446, 0x80);
		int rc2 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d46, 0x80);
		printf("SWEET GPU: enable rc=%d/%d\n", rc1, rc2);
		sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
		sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);
	}

	if (level >= 3) {
		/* 900mV = 0x0384, LSB first while the rail is already up. */
		int rc1 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1440, 0x84);
		int rc2 = sweet_spmi_write_ppid(PM8005_S1_PPID, 0x1441, 0x03);
		printf("SWEET GPU: S1 set 900mV rc=%d/%d\n", rc1, rc2);
		rc1 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d40, 0x84);
		rc2 = sweet_spmi_write_ppid(PM8005_S4_PPID, 0x1d41, 0x03);
		printf("SWEET GPU: S4 set 900mV rc=%d/%d\n", rc1, rc2);
		sweet_gpu_rail("S1", PM8005_S1_PPID, 0x1400);
		sweet_gpu_rail("S4", PM8005_S4_PPID, 0x1d00);
	}

	if (level == 5) {
		vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
		if (gpuc) {
			printf("SWEET GPU: PLL mode=%#x opmode=%#x status=%#x\n",
			    gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x002c),
			    gpu_rd(gpuc, 0x0024));
			printf("SWEET GPU: PLL L=%#x cal=%#x frac=%#x uctl=%#x\n",
			    gpu_rd(gpuc, 0x0004), gpu_rd(gpuc, 0x0008),
			    gpu_rd(gpuc, 0x0038), gpu_rd(gpuc, 0x000c));
			printf("SWEET GPU: RCG cmd=%#x cfg=%#x m=%#x n=%#x d=%#x\n",
			    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1074),
			    gpu_rd(gpuc, 0x1078), gpu_rd(gpuc, 0x107c),
			    gpu_rd(gpuc, 0x1080));
			printf("SWEET GPU: rbbm_rcg=%#x br=%#x\n",
			    gpu_rd(gpuc, 0x10b0), gpu_rd(gpuc, 0x10d0));
		}
	}

	if (level == 4) {
		vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
		printf("SWEET GPU: gpuc=%#lx\n", gpuc);
		if (gpuc) {
			printf("SWEET GPU: CX_GDSCR %#x GX_GDSCR %#x\n",
			    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094));
			printf("SWEET GPU: GX_BCR %#x clamp %#x\n",
			    gpu_rd(gpuc, 0x1090), gpu_rd(gpuc, 0x0130));
			printf("SWEET GPU: gfx3d_rcg %#x branch %#x\n",
			    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1098));
		}
	}

	if (level >= 6) {
		vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
		vm_offset_t gpu = ml_io_map(0x05000000u, 0x00040000u);
		printf("SWEET GPU: l6 gpuc=%#lx gpu=%#lx\n", gpuc, gpu);
		if (gpuc) {
			gpu_wr(gpuc, 0x1004, 0x222000u);
			delay(10);
			gpu_wr(gpuc, 0x1090, 0x1u);
			delay(1);
			gpu_wr(gpuc, 0x1090, 0x0u);
			gpu_wr(gpuc, 0x0130, 0x11u);
			delay(1);
			gpu_wr(gpuc, 0x0130, 0x01u);
			delay(1);
			gpu_wr(gpuc, 0x0130, 0x00u);
			gpu_wr(gpuc, 0x1094, 0x222000u);
			delay(10);
			printf("SWEET GPU: GDSC CX=%#x GX=%#x xo_branch=%#x\n",
			    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094),
			    gpu_rd(gpuc, 0x1020));
			printf("SWEET GPU: PLL pre mode=%#x op=%#x l=%#x cal=%#x frac=%#x\n",
			    gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x002c),
			    gpu_rd(gpuc, 0x0004), gpu_rd(gpuc, 0x0008),
			    gpu_rd(gpuc, 0x0038));
			gpu_wr(gpuc, 0x0004, 0x12u);
			gpu_wr(gpuc, 0x0008, 0x3fu);
			gpu_wr(gpuc, 0x0038, 0xc00u);
			gpu_wr(gpuc, 0x0014, 0x20485699u);
			gpu_wr(gpuc, 0x0018, 0x2067u);
			gpu_wr(gpuc, 0x0010, 0x4805u);
			gpu_wr(gpuc, 0x000c, 0x7u);
			printf("SWEET GPU: PLL rb l=%#x cal=%#x frac=%#x cfg=%#x uctl=%#x\n",
			    gpu_rd(gpuc, 0x0004), gpu_rd(gpuc, 0x0008),
			    gpu_rd(gpuc, 0x0038), gpu_rd(gpuc, 0x0014),
			    gpu_rd(gpuc, 0x000c));
			uint32_t mode = gpu_rd(gpuc, 0x0000);
			gpu_wr(gpuc, 0x0000, mode & ~1u);
			gpu_wr(gpuc, 0x002c, 0x0u);
			delay(1);
			gpu_wr(gpuc, 0x0000, (mode & ~1u) | 0x4u);
			gpu_wr(gpuc, 0x002c, 0x1u);
			int locked = 0;
			for (int i = 0; i < 5000; ++i) {
				if (gpu_rd(gpuc, 0x0000) & 0x80000000u) {
					locked = 1;
					break;
				}
				delay(1);
			}
			uint32_t uctl = gpu_rd(gpuc, 0x000c);
			gpu_wr(gpuc, 0x000c, uctl | 0x7u);
			mode = gpu_rd(gpuc, 0x0000);
			gpu_wr(gpuc, 0x0000, mode | 0x1u);
			printf("SWEET GPU: PLL lock=%d mode=%#x op=%#x status=%#x\n",
			    locked, gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x002c),
			    gpu_rd(gpuc, 0x0024));
			if (locked) {
				gpu_wr(gpuc, 0x1078, 0x0u);
				gpu_wr(gpuc, 0x107c, 0xffffffffu);
				gpu_wr(gpuc, 0x1080, 0xffffffffu);
				gpu_wr(gpuc, 0x1074, 0x101u);
				gpu_wr(gpuc, 0x1070, 0x0u);
				delay(10);
				gpu_wr(gpuc, 0x1098, 0x1u);
				delay(10);
				printf("SWEET GPU: RCG cmd=%#x cfg=%#x branch=%#x\n",
				    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1074),
				    gpu_rd(gpuc, 0x1098));
			} else {
				/* Fallback: gfx3d from GPLL0 (src 5), divide by 2. */
				gpu_wr(gpuc, 0x1074, 0x501u);
				gpu_wr(gpuc, 0x1070, 0x0u);
				delay(10);
				gpu_wr(gpuc, 0x1098, 0x1u);
				delay(10);
				printf("SWEET GPU: fallback RCG cmd=%#x cfg=%#x branch=%#x\n",
				    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1074),
				    gpu_rd(gpuc, 0x1098));
			}
			if (gpu) {
				printf("SWEET GPU: reading RBBM\n");
				uint32_t rbbm = gpu_rd(gpu, 0x4f5u * 4u);
				printf("SWEET GPU: RBBM_STATUS %#x ERR %#x ST3 %#x\n",
				    rbbm, gpu_rd(gpu, 0x4f4u * 4u),
				    gpu_rd(gpu, 0x530u * 4u));
				    printf("SWEET GPU: CP rb=%#x me=%#x pfp=%#x pm4=%#x\n",
				        gpu_rd(gpu, 0x802u * 4u), gpu_rd(gpu, 0x832u * 4u),
				        gpu_rd(gpu, 0x835u * 4u), gpu_rd(gpu, 0x838u * 4u));
			}
		}
	}

	if (level >= 7) {
		vm_offset_t smmu = ml_io_map(0x05040000u, 0x00010000u);
		printf("SWEET GPU: smmu=%#lx\n", smmu);
		if (smmu) {
			printf("SWEET GPU: SMMU ID0=%#x ID1=%#x SCR0=%#x\n",
			    gpu_rd(smmu, 0x20), gpu_rd(smmu, 0x24),
			    gpu_rd(smmu, 0x0));
			printf("SWEET GPU: SMMU SMR0=%#x S2CR0=%#x micro=%#x\n",
			    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0xc00),
			    gpu_rd(smmu, 0x6000));
			/* CB0@0x8000 (standard ARM SMMU); 0x48000 is not readable here. */
			vm_offset_t cb = smmu + 0x8000u;
			printf("SWEET GPU: SMMU CB0 SCTLR=%#x TTBR0=%#x TCR=%#x FSR=%#x\n",
			    gpu_rd(cb, 0x0), gpu_rd(cb, 0x20), gpu_rd(cb, 0x30),
			    gpu_rd(cb, 0x58));
		}
	}

	if (level >= 8) {
		sweet_gpu_step(0x80);
		printf("SWEET GPU: l8 enter\n");
		vm_offset_t smmu = ml_io_map(0x05040000u, 0x00010000u);
		sweet_gpu_step(0x81);
		printf("SWEET GPU: l8 smmu=%#lx\n", smmu);
		if (smmu) {
			uint64_t pt = (uint64_t)kvtophys((vm_offset_t)smmu_l1);
			sweet_gpu_step(0x82);
			printf("SWEET GPU: l8 pt=%#llx\n", pt);
			if (pt) {
				int use2m = 0;
				if (PE_parse_boot_argn("sweetgpu2m", &use2m,
				    (int)sizeof(use2m)) && use2m == 1) {
					for (uint32_t i = 0; i < 512; i++) {
						smmu_l2b[i] = (0x80000000ull +
						    (uint64_t)i * 0x200000ull) | 1u | (1u << 6) |
						    (2u << 8) | (1u << 10);
					}
					flush_dcache((vm_offset_t)smmu_l2b, PAGE_SIZE, FALSE);
					smmu_l1[2] = (uint64_t)kvtophys((vm_offset_t)smmu_l2b) | 3u;
					printf("SWEET GPU: SMMU GB2 -> 2MB table %#llx\n",
					    (uint64_t)kvtophys((vm_offset_t)smmu_l2b));
					for (uint64_t gb = 3; gb < 8; gb++) {
						smmu_l1[gb] = (gb << 30) | 1u | (1u << 6) |
						    (2u << 8) | (1u << 10);
					}
				} else {
					for (uint64_t gb = 2; gb < 8; gb++) {
						smmu_l1[gb] = (gb << 30) | 1u | (1u << 6) |
						    (2u << 8) | (1u << 10);
					}
				}
				flush_dcache((vm_offset_t)smmu_l1, PAGE_SIZE, FALSE);
				vm_offset_t cb = smmu + 0x8000u;
				vm_offset_t gr1 = smmu + 0x1000u;
				sweet_gpu_step(0x83);
				gpu_wr(cb, 0x0, 0);
				gpu_wr(gr1, 0x0, 1u << 16);
				gpu_wr(gr1, 0x800, 1u);
				gpu_wr(cb, 0x10, (7u << 15) | 1u);
				gpu_wr(cb, 0x30, 28u | (1u << 23));
				gpu_wr(cb, 0x38, 0x44u);
				gpu_wr(cb, 0x20, (uint32_t)pt);
				gpu_wr(cb, 0x24, (uint32_t)(pt >> 32));
				gpu_wr(cb, 0x58, 0xffffffffu);
				gpu_wr(cb, 0x0, (1u << 5) | (1u << 2) | (1u << 1) | 1u);
				sweet_gpu_step(0x84);
				printf("SWEET GPU: l8 cb done\n");
				sweet_gpu_step(0x85);
				gpu_wr(smmu, 0x800, 1u << 31);
				sweet_gpu_step(0x86);
				/* sailfish 0123: SMR0 matches stream 0 only and S2CR0
				 * stays translate; fault/bypass probing perturbed the
				 * firmware state (non-zero SIDs reset this SoC). */
				gpu_wr(smmu, 0xc00, 0x0u);
				printf("SWEET GPU: SMMU SID0 S2CR0=%#x (translate)\n",
				    gpu_rd(smmu, 0xc00));
				gpu_wr(smmu, 0x68, 0u);
				gpu_wr(smmu, 0x70, 0u);
				int sync = 0;
				for (int i = 0; i < 100; ++i) {
					if (!(gpu_rd(smmu, 0x74) & 1u)) { sync = 1; break; }
					delay(1);
				}
				sweet_gpu_step(0x87);
				printf("SWEET GPU: SMMU identity SMR0=%#x S2CR0=%#x CBAR0=%#x CBA2R0=%#x\n",
				    gpu_rd(smmu, 0x800), gpu_rd(smmu, 0xc00),
				    gpu_rd(gr1, 0x0), gpu_rd(gr1, 0x800));
				printf("SWEET GPU: SMMU CB0 SCTLR=%#x TCR=%#x TTBR0=%#x FSR=%#x sync=%d\n",
				    gpu_rd(cb, 0x0), gpu_rd(cb, 0x30), gpu_rd(cb, 0x20),
				    gpu_rd(cb, 0x58), sync);
			}
		}
	}

	if (level >= 10 && level < 13) {
		/* mainline order: GPMU up before the zap shader runs. */
		if (level >= 10) {
			vm_offset_t gpre = ml_io_map(0x05000000u, 0x00040000u);
			if (gpre) {
				gpmu_ready = sweet_gpmu_host_boot(gpre);
				printf("SWEET GPU: pre-zap GPMU ready=%d %#x\n",
				    gpmu_ready, gpu_rd(gpre, 0xa8e0u * 4u));
			}
		}
		sweet_gpu_step(0x99);
		printf("SWEET GPU: zap load begin\n");
		zap_ok = sweet_zap_load();
		printf("SWEET GPU: zap %s\n", zap_ok ? "OK" : "FAIL");
		if (zap_ok) {
			/* re-power after zap: auth_and_reset may reset clocks. */
			vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
			if (gpuc) {
				gpu_wr(gpuc, 0x1004, 0x222000u);
				delay(10);
				gpu_wr(gpuc, 0x1090, 0x1u);
				delay(1);
				gpu_wr(gpuc, 0x1090, 0x0u);
				gpu_wr(gpuc, 0x0130, 0x11u);
				delay(1);
				gpu_wr(gpuc, 0x0130, 0x01u);
				delay(1);
				gpu_wr(gpuc, 0x0130, 0x00u);
				gpu_wr(gpuc, 0x1094, 0x222000u);
				delay(10);
				gpu_wr(gpuc, 0x0004, 0x12u);
				gpu_wr(gpuc, 0x0008, 0x3fu);
				gpu_wr(gpuc, 0x0038, 0xc00u);
				gpu_wr(gpuc, 0x0014, 0x20485699u);
				gpu_wr(gpuc, 0x0018, 0x2067u);
				gpu_wr(gpuc, 0x0010, 0x4805u);
				gpu_wr(gpuc, 0x000c, 0x7u);
				uint32_t m = gpu_rd(gpuc, 0x0000);
				gpu_wr(gpuc, 0x0000, m & ~1u);
				gpu_wr(gpuc, 0x002c, 0x0u);
				delay(1);
				gpu_wr(gpuc, 0x0000, (m & ~1u) | 0x4u);
				gpu_wr(gpuc, 0x002c, 0x1u);
				delay(100);
				gpu_wr(gpuc, 0x000c, gpu_rd(gpuc, 0x000c) | 0x7u);
				gpu_wr(gpuc, 0x0000, gpu_rd(gpuc, 0x0000) | 0x1u);
				gpu_wr(gpuc, 0x1074, 0x101u);
				gpu_wr(gpuc, 0x1070, 0x0u);
				delay(10);
				gpu_wr(gpuc, 0x1098, 0x1u);
				delay(10);
				printf("SWEET GPU: re-power GDSC=%#x/%#x PLL=%#x RCG=%#x br=%#x\n",
				    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094),
				    gpu_rd(gpuc, 0x0000), gpu_rd(gpuc, 0x1074),
				    gpu_rd(gpuc, 0x1098));
			}
			/* Opt-in secure-mode drop (Linux no-zap fallback). */
			{
				int tr = 0;
				if (PE_parse_boot_argn("sweetgputrust", &tr,
				    (int)sizeof(tr)) && tr == 1) {
					vm_offset_t gt = ml_io_map(0x05000000u, 0x00040000u);
					if (gt) {
						printf("SWEET GPU: SECVID_TRUST before=%#x\n",
						    gpu_rd(gt, 0xf400u * 4u));
						gpu_wr(gt, 0xf400u * 4u, 0u);
						delay(1000);
						printf("SWEET GPU: SECVID_TRUST after=%#x\n",
						    gpu_rd(gt, 0xf400u * 4u));
					}
				}
				/* KGSL a5xx_microcode_load: SCM SET_REMOTE_STATE(0, 13). */
				{
					int rsm = 0;
					if (PE_parse_boot_argn("sweetgpuresume", &rsm,
					    (int)sizeof(rsm)) && rsm == 1) {
						uint64_t rr1 = 0;
						int64_t rr = sweet_scm_call(1u, 0x0au, 0x2u, 0u,
						    13u, 0u, &rr1);
						printf("SWEET GPU: SCM remote_state(0,13) -> %lld/%#llx\n",
						    rr, rr1);
					}
				}
			}
		}
	}

	if (level >= 9) {
		sweet_gpu_step(0x90);
		printf("SWEET GPU: CP test enter\n");
		vm_offset_t gpu = ml_io_map(0x05000000u, 0x00040000u);
		uint64_t pm4 = 0, pfp = 0;
		uint32_t pm4_len = 0, pfp_len = 0;
		int have = sweet_gpu_blob("GPU-PM4", &pm4, &pm4_len) &&
		    sweet_gpu_blob("GPU-PFP", &pfp, &pfp_len);
		printf("SWEET GPU: CP blobs %d pm4=%#llx(%u) pfp=%#llx(%u)\n",
		    have, pm4, pm4_len, pfp, pfp_len);
		if (gpu && have) {
			/* GPMU: zap may have reset it; load it again if needed. */
			if (level >= 10) {
				gpmu_ready = sweet_gpmu_host_boot(gpu);
			}
			sweet_gpu_step(0x91);
			uint64_t ring_pa = (uint64_t)kvtophys((vm_offset_t)cp_ring);
			uint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)cp_scratch);
			/* Copy the header-stripped microcode into aligned pages
			 * (sweet_a540_start layout). */
			vm_offset_t pm4_va2 = 0, pfp_va2 = 0;
			uint32_t pm4_bytes = pm4_len - 4u, pfp_bytes = pfp_len - 4u;
			uint64_t pm4_base = gpu_alloc(&pm4_va2, 0x8000);
			uint64_t pfp_base = gpu_alloc(&pfp_va2, 0x8000);
			if (!pm4_base || !pfp_base) {
				printf("SWEET GPU: CP fw alloc failed, using blob+4\n");
				pm4_base = pm4 + 4u;
				pfp_base = pfp + 4u;
			} else {
				memcpy((void *)pm4_va2, (const void *)phystokv(pm4 + 4),
				    pm4_bytes);
				memcpy((void *)pfp_va2, (const void *)phystokv(pfp + 4),
				    pfp_bytes);
			}
			uint32_t *ringp = cp_ring, *scratchp = cp_scratch;
			vm_offset_t carve = 0;
			/* carveout approach removed: secure carveout only maps the
			 * authenticated 8KB segment; host writes abort the bus. */
			printf("SWEET GPU: CP ring=%#llx scratch=%#llx pm4=%#llx pfp=%#llx\n",
			    ring_pa, scratch_pa, pm4_base, pfp_base);
			/* zap auth_and_reset may reset the GPU SMMU: show its
			 * state and restore the identity map before CP fetch. */
			if (level >= 8) {
				vm_offset_t sm2 = ml_io_map(0x05040000u, 0x00010000u);
				if (sm2) {
					vm_offset_t cb2 = sm2 + 0x8000u;
					vm_offset_t gr12 = sm2 + 0x1000u;
					uint64_t pt2 = (uint64_t)kvtophys((vm_offset_t)smmu_l1);
					printf("SWEET GPU: pre-CP SMMU SMR0=%#x S2CR0=%#x SCTLR=%#x TTBR0=%#x FSR=%#x\n",
					    gpu_rd(sm2, 0x800), gpu_rd(sm2, 0xc00), gpu_rd(cb2, 0x0),
					    gpu_rd(cb2, 0x20), gpu_rd(cb2, 0x58));
					gpu_wr(cb2, 0x0, 0);
					gpu_wr(gr12, 0x0, 1u << 16);
					gpu_wr(gr12, 0x800, 1u);
					gpu_wr(cb2, 0x10, (7u << 15) | 1u);
					gpu_wr(cb2, 0x30, 28u | (1u << 23));
					gpu_wr(cb2, 0x38, 0x44u);
					gpu_wr(cb2, 0x20, (uint32_t)pt2);
					gpu_wr(cb2, 0x24, (uint32_t)(pt2 >> 32));
					gpu_wr(cb2, 0x58, 0xffffffffu);
					gpu_wr(cb2, 0x0, (1u << 5) | (1u << 2) | (1u << 1) | 1u);
					gpu_wr(sm2, 0x800, 1u << 31);
					gpu_wr(sm2, 0xc00, 0x0u);
					gpu_wr(sm2, 0x68, 0u);
					gpu_wr(sm2, 0x70, 0u);
					int syn2 = 0;
					for (int i = 0; i < 100; ++i) {
						if (!(gpu_rd(sm2, 0x74) & 1u)) { syn2 = 1; break; }
						delay(1);
					}
					printf("SWEET GPU: re-CP SMMU SMR0=%#x S2CR0=%#x SCTLR=%#x TTBR0=%#x FSR=%#x sync=%d\n",
					    gpu_rd(sm2, 0x800), gpu_rd(sm2, 0xc00), gpu_rd(cb2, 0x0),
					    gpu_rd(cb2, 0x20), gpu_rd(cb2, 0x58), syn2);
				}
			}
			sweet_gpu_step(0x92);
			/* A540: halt CP and drain VBIF before reprogramming
			 * (sagit AdrenoStartup/Submit start() preconditions). */
			printf("SWEET GPU: CP pre-halt ME=%#x micro=%#x/%#x/%#x/%#x\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u),
			    gpu_rd(gpu, 0x01f7u * 4u), gpu_rd(gpu, 0x01f8u * 4u),
			    gpu_rd(gpu, 0x01f9u * 4u), gpu_rd(gpu, 0x01fau * 4u));
			gpu_wr(gpu, 0x0038u * 4u, 0u);
			gpu_wr(gpu, R_CP_PFP_ME_CNTL * 4u, 0x303u);
			gpu_wr(gpu, 0x3080u * 4u, 15u);
			int cph = 0;
			for (int i = 0; i < 100; ++i) {
				if ((gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u) & 0x303u) == 0x303u &&
				    (gpu_rd(gpu, 0x3081u * 4u) & 15u) == 15u) { cph = 1; break; }
				delay(1000);
			}
			printf("SWEET GPU: CP halt=%d ME=%#x VBIF=%#x trust=%#x gpmu=%#x/%#x cm3=%#x\n",
			    cph, gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u),
			    gpu_rd(gpu, 0x3081u * 4u), gpu_rd(gpu, 0xf400u * 4u),
			    gpu_rd(gpu, 0xa8e0u * 4u), gpu_rd(gpu, 0xa8e1u * 4u),
			    gpu_rd(gpu, 0xa8d8u * 4u));
			/* a5xx_pm_resume: power the RB/CCU and SP islands. */
			gpu_wr(gpu, R_GPMU_RBCCU_POWER_CNTL * 4u, 0x778000u);
			int rbccu = 0;
			for (int i = 0; i < 200; ++i) {
				if (gpu_rd(gpu, R_GPMU_RBCCU_PWR_CLK_STATUS * 4u) & (1u << 20)) {
					rbccu = 1; break;
				}
				delay(100);
			}
			gpu_wr(gpu, R_GPMU_SP_POWER_CNTL * 4u, 0x778000u);
			int sp = 0;
			for (int i = 0; i < 200; ++i) {
				if (gpu_rd(gpu, R_GPMU_SP_PWR_CLK_STATUS * 4u) & (1u << 20)) {
					sp = 1; break;
				}
				delay(100);
			}
			printf("SWEET GPU: GPMU rbccu=%d sp=%d\n", rbccu, sp);
			gpu_wr(gpu, R_VBIF_RR_QOS_ARB * 4u, 3u);
			gpu_wr(gpu, R_RBBM_PERFCTR_GPU_BUSY_MASKED * 4u, 0xffffffffu);
			gpu_wr(gpu, R_RBBM_AHB_CNTL0 * 4u, 1u);
			gpu_wr(gpu, R_UCHE_CACHE_WAYS * 4u, 2u);
			gpu_wr(gpu, R_UCHE_TRAP_BASE * 4u, 0x0001ffffu);
			gpu_wr(gpu, (R_UCHE_TRAP_BASE + 1u) * 4u, 0x0001ffffu);
			gpu_wr(gpu, R_UCHE_WRITE_THRU_BASE * 4u, 0x0001ffffu);
			gpu_wr(gpu, (R_UCHE_WRITE_THRU_BASE + 1u) * 4u, 0x0001ffffu);
			gpu_wr(gpu, R_UCHE_GMEM_RANGE_MIN * 4u, 0x00100000u);
			gpu_wr(gpu, (R_UCHE_GMEM_RANGE_MIN + 1u) * 4u, 0u);
			gpu_wr(gpu, R_UCHE_GMEM_RANGE_MAX * 4u, 0x001fffffu);
			gpu_wr(gpu, (R_UCHE_GMEM_RANGE_MAX + 1u) * 4u, 0u);
			gpu_wr(gpu, R_CP_MEQ_THRESHOLDS * 4u, 0x40u);
			gpu_wr(gpu, R_CP_MERCIU_SIZE * 4u, 0x400u);
			gpu_wr(gpu, R_CP_ROQ_THRESHOLDS_2 * 4u, 0x80000060u);
			gpu_wr(gpu, R_CP_ROQ_THRESHOLDS_1 * 4u, 0x40201b16u);
			gpu_wr(gpu, R_PC_DBG_ECO_CNTL * 4u, (0x400u << 11) | (0x300u << 22));
			gpu_wr(gpu, R_UCHE_MODE_CNTL * 4u, 1u << 29);
			gpu_wr(gpu, R_CP_CHICKEN_DBG * 4u, 0x02000000u);
			gpu_wr(gpu, R_RBBM_AHB_CNTL1 * 4u, 0xa6ffffffu);
			gpu_wr(gpu, R_RBBM_AHB_CNTL2 * 4u, 0x3fu);
			gpu_wr(gpu, R_TPL1_MODE_CNTL * 4u, 2u << 7);
			gpu_wr(gpu, R_RB_MODE_CNTL * 4u, 2u << 1);
			gpu_wr(gpu, R_VPC_DBG_ECO_CNTL * 4u,
			    gpu_rd(gpu, R_VPC_DBG_ECO_CNTL * 4u) | (1u << 10));
			gpu_wr(gpu, R_SECVID_TSB_CNTL * 4u, 0u);
			gpu_wr(gpu, R_SECVID_TSB_BASE * 4u, 0u);
			gpu_wr(gpu, (R_SECVID_TSB_BASE + 1u) * 4u, 0u);
			gpu_wr(gpu, R_SECVID_TSB_SIZE * 4u, 0u);
			static const uint16_t addr_mode[] = {
			    0x0860u, 0x0bc1u, 0x0c81u, 0x0cc5u, 0x0d01u, 0x0e05u,
			    0x0e41u, 0x0e61u, 0x0e80u, 0x0ec1u, 0x0f01u, 0xf810u };
			for (unsigned i = 0; i < sizeof(addr_mode) / sizeof(addr_mode[0]); i++) {
				gpu_wr(gpu, addr_mode[i] * 4u, 1u);
			}
	/* A540 configure (AdrenoStartup::configure, HGCG table omitted). */
	gpu_wr(gpu, 0x3049u * 4u, 3u);			/* VBIF RR QoS */
	gpu_wr(gpu, 0x302au * 4u, 9u);			/* VBIF gate off wrreq */
	gpu_wr(gpu, 0x046fu * 4u, 0xffffffffu);
	gpu_wr(gpu, 0x0093u * 4u, 1u);
	gpu_wr(gpu, 0x002fu * 4u, 0x4000ffffu);		/* hang int */
	gpu_wr(gpu, 0x0464u * 4u, 1u);
	gpu_wr(gpu, 0x0bb0u * 4u, 0u);
	gpu_wr(gpu, 0x046bu * 4u, 6u);
	gpu_wr(gpu, 0x0e96u * 4u, 2u);
	gpu_wr(gpu, 0x0e89u * 4u, 0xffff0000u);
	gpu_wr(gpu, 0x0e8au * 4u, 0x1ffffu);
	gpu_wr(gpu, 0x0e87u * 4u, 0xffff0000u);
	gpu_wr(gpu, 0x0e88u * 4u, 0x1ffffu);
	gpu_wr(gpu, 0x0e8bu * 4u, 0x100000u);
	gpu_wr(gpu, 0x0e8cu * 4u, 0u);
	gpu_wr(gpu, 0x0e8du * 4u, 0x1fffffu);
	gpu_wr(gpu, 0x0e8eu * 4u, 0u);
	gpu_wr(gpu, 0x0825u * 4u, 0x40u);
	gpu_wr(gpu, 0x0826u * 4u, 0x400u);		/* A540 MERCIU */
	gpu_wr(gpu, 0x0820u * 4u, 0x80000060u);
	gpu_wr(gpu, 0x081fu * 4u, 0x40201b16u);
	gpu_wr(gpu, 0x0d00u * 4u, (0x400u << 11) | (0x300u << 22));
	gpu_wr(gpu, 0x0e81u * 4u, 1u << 29);
	gpu_wr(gpu, 0x0833u * 4u, 0x02000000u);
	gpu_wr(gpu, 0x0094u * 4u, 0xa6ffffffu);
	gpu_wr(gpu, 0x0095u * 4u, 0x3fu);
	gpu_wr(gpu, 0x0f02u * 4u, 2u << 7);
	gpu_wr(gpu, 0x0cc6u * 4u, 2u << 1);
	gpu_wr(gpu, 0x0e8fu * 4u, 2u);
	gpu_wr(gpu, 0x08a0u * 4u, 7u);			/* CP_PROTECT_CNTL */
	gpu_wr(gpu, 0x0e60u * 4u,
	    gpu_rd(gpu, 0x0e60u * 4u) | (1u << 10) | (1u << 23));
	gpu_wr(gpu, 0x0e04u * 4u,
	    gpu_rd(gpu, 0x0e04u * 4u) & ~(1u << 18));
	static const uint32_t prot[] = {
		0x0880u + 0, 0x0880u + 1, 0x0880u + 2, 0x0880u + 3,
		0x0880u + 4, 0x0880u + 5, 0x0880u + 6, 0x0880u + 7,
		0x0880u + 8, 0x0880u + 9, 0x0880u + 10, 0x0880u + 11,
		0x0880u + 12, 0x0880u + 13, 0x0880u + 14, 0x0880u + 15,
		0x0880u + 16, 0x0880u + 17,
	};
	const uint32_t protv[] = {
		a540_prot(0x4, 2), a540_prot(0x8, 3), a540_prot(0x10, 4),
		a540_prot(0x20, 5), a540_prot(0x40, 6), a540_prot(0x80, 6),
		a540_prot(0xf800, 4), a540_prot(0xf400, 1),
		a540_prot(0x800, 6), a540_prot(0x840, 3), a540_prot(0x880, 5),
		a540_prot(0xaa0, 0), a540_prot(0xcc0, 0), a540_prot(0xcf0, 1),
		a540_prot(0xe68, 3), a540_prot(0xe70, 4), a540_prot(0xe80, 4),
		a540_prot(0x10000, 15),
	};
	for (unsigned i = 0; i < sizeof(prot) / sizeof(prot[0]); i++) {
		gpu_wr(gpu, prot[i] * 4u, protv[i]);
	}
	gpu_wr(gpu, 0xf803u * 4u, 0u);
	gpu_wr(gpu, 0xf800u * 4u, 0u);
	gpu_wr(gpu, 0xf801u * 4u, 0u);
	gpu_wr(gpu, 0xf802u * 4u, 0u);
	static const uint16_t modes[] = {
		0x0860u, 0x0bc1u, 0x0c81u, 0x0cc5u, 0x0d01u, 0x0e05u,
		0x0e41u, 0x0e61u, 0x0e80u, 0x0ec1u, 0x0f01u, 0xf810u,
	};
	for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
		gpu_wr(gpu, modes[i] * 4u, 1u);
	}
	/* A540 hardware clock gating (AdrenoStartup::hardwareClockGating). */
	gpu_wr(gpu, 104u * 4u, 0x02222222u);
	gpu_wr(gpu, 105u * 4u, 0x02222222u);
	gpu_wr(gpu, 106u * 4u, 0x02222222u);
	gpu_wr(gpu, 107u * 4u, 0x02222222u);
	gpu_wr(gpu, 108u * 4u, 0x02222220u);
	gpu_wr(gpu, 109u * 4u, 0x02222220u);
	gpu_wr(gpu, 110u * 4u, 0x02222220u);
	gpu_wr(gpu, 111u * 4u, 0x02222220u);
	gpu_wr(gpu, 112u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 113u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 114u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 115u * 4u, 0x0000F3CFu);
	gpu_wr(gpu, 116u * 4u, 0x00000080u);
	gpu_wr(gpu, 117u * 4u, 0x00000080u);
	gpu_wr(gpu, 118u * 4u, 0x00000080u);
	gpu_wr(gpu, 119u * 4u, 0x00000080u);
	gpu_wr(gpu, 74u * 4u, 0x22222222u);
	gpu_wr(gpu, 75u * 4u, 0x22222222u);
	gpu_wr(gpu, 76u * 4u, 0x22222222u);
	gpu_wr(gpu, 77u * 4u, 0x22222222u);
	gpu_wr(gpu, 78u * 4u, 0x22222222u);
	gpu_wr(gpu, 79u * 4u, 0x22222222u);
	gpu_wr(gpu, 80u * 4u, 0x22222222u);
	gpu_wr(gpu, 81u * 4u, 0x22222222u);
	gpu_wr(gpu, 82u * 4u, 0x00002222u);
	gpu_wr(gpu, 83u * 4u, 0x00002222u);
	gpu_wr(gpu, 84u * 4u, 0x00002222u);
	gpu_wr(gpu, 85u * 4u, 0x00002222u);
	gpu_wr(gpu, 176u * 4u, 0x77777777u);
	gpu_wr(gpu, 177u * 4u, 0x77777777u);
	gpu_wr(gpu, 178u * 4u, 0x77777777u);
	gpu_wr(gpu, 179u * 4u, 0x77777777u);
	gpu_wr(gpu, 180u * 4u, 0x77777777u);
	gpu_wr(gpu, 181u * 4u, 0x77777777u);
	gpu_wr(gpu, 182u * 4u, 0x77777777u);
	gpu_wr(gpu, 183u * 4u, 0x77777777u);
	gpu_wr(gpu, 184u * 4u, 0x00007777u);
	gpu_wr(gpu, 185u * 4u, 0x00007777u);
	gpu_wr(gpu, 186u * 4u, 0x00007777u);
	gpu_wr(gpu, 187u * 4u, 0x00007777u);
	gpu_wr(gpu, 164u * 4u, 0x11111111u);
	gpu_wr(gpu, 165u * 4u, 0x11111111u);
	gpu_wr(gpu, 166u * 4u, 0x11111111u);
	gpu_wr(gpu, 167u * 4u, 0x11111111u);
	gpu_wr(gpu, 168u * 4u, 0x11111111u);
	gpu_wr(gpu, 169u * 4u, 0x11111111u);
	gpu_wr(gpu, 170u * 4u, 0x11111111u);
	gpu_wr(gpu, 171u * 4u, 0x11111111u);
	gpu_wr(gpu, 172u * 4u, 0x00001111u);
	gpu_wr(gpu, 173u * 4u, 0x00001111u);
	gpu_wr(gpu, 174u * 4u, 0x00001111u);
	gpu_wr(gpu, 175u * 4u, 0x00001111u);
	gpu_wr(gpu, 90u * 4u, 0x22222222u);
	gpu_wr(gpu, 91u * 4u, 0x22222222u);
	gpu_wr(gpu, 92u * 4u, 0x22222222u);
	gpu_wr(gpu, 93u * 4u, 0x00222222u);
	gpu_wr(gpu, 94u * 4u, 0x00444444u);
	gpu_wr(gpu, 95u * 4u, 0x00000002u);
	gpu_wr(gpu, 120u * 4u, 0x22222222u);
	gpu_wr(gpu, 121u * 4u, 0x22222222u);
	gpu_wr(gpu, 122u * 4u, 0x22222222u);
	gpu_wr(gpu, 123u * 4u, 0x22222222u);
	gpu_wr(gpu, 124u * 4u, 0x00222222u);
	gpu_wr(gpu, 125u * 4u, 0x00222222u);
	gpu_wr(gpu, 126u * 4u, 0x00222222u);
	gpu_wr(gpu, 127u * 4u, 0x00222222u);
	gpu_wr(gpu, 130u * 4u, 0x00022220u);
	gpu_wr(gpu, 131u * 4u, 0x00022220u);
	gpu_wr(gpu, 132u * 4u, 0x00022220u);
	gpu_wr(gpu, 133u * 4u, 0x00022220u);
	gpu_wr(gpu, 138u * 4u, 0x05522222u);
	gpu_wr(gpu, 139u * 4u, 0x00505555u);
	gpu_wr(gpu, 134u * 4u, 0x04040404u);
	gpu_wr(gpu, 135u * 4u, 0x04040404u);
	gpu_wr(gpu, 136u * 4u, 0x04040404u);
	gpu_wr(gpu, 137u * 4u, 0x04040404u);
	gpu_wr(gpu, 128u * 4u, 0x07444044u);
	gpu_wr(gpu, 140u * 4u, 0x00000002u);
	gpu_wr(gpu, 141u * 4u, 0x00000002u);
	gpu_wr(gpu, 142u * 4u, 0x00000002u);
	gpu_wr(gpu, 143u * 4u, 0x00000002u);
	gpu_wr(gpu, 129u * 4u, 0x00010011u);
	gpu_wr(gpu, 99u * 4u, 0x04222222u);
	gpu_wr(gpu, 96u * 4u, 0x02222222u);
	gpu_wr(gpu, 145u * 4u, 0x00002222u);
	gpu_wr(gpu, 100u * 4u, 0x00000000u);
	gpu_wr(gpu, 98u * 4u, 0x04104004u);
	gpu_wr(gpu, 144u * 4u, 0x00000000u);
	gpu_wr(gpu, 102u * 4u, 0x00000000u);
	gpu_wr(gpu, 101u * 4u, 0x00004000u);
	gpu_wr(gpu, 97u * 4u, 0x00000200u);
	gpu_wr(gpu, 146u * 4u, 0x00002222u);
	gpu_wr(gpu, 201u * 4u, 0x770u);
	gpu_wr(gpu, 202u * 4u, 4u);
	gpu_wr(gpu, 103u * 4u, 0xaaa8aa00u);
	gpu_wr(gpu, 1331u * 4u, 0x182u);

			sweet_gpu_step(0x93);
			gpu_wr(gpu, R_CP_PFP_INSTR_BASE * 4u, (uint32_t)pfp_base);
			gpu_wr(gpu, (R_CP_PFP_INSTR_BASE + 1u) * 4u, (uint32_t)(pfp_base >> 32));
			gpu_wr(gpu, R_CP_ME_INSTR_BASE * 4u, (uint32_t)pm4_base);
			gpu_wr(gpu, (R_CP_ME_INSTR_BASE + 1u) * 4u, (uint32_t)(pm4_base >> 32));
			gpu_wr(gpu, R_CP_RB_RPTR_ADDR_LO * 4u, (uint32_t)(scratch_pa + 0x10));
			gpu_wr(gpu, R_CP_RB_RPTR_ADDR_HI * 4u,
			    (uint32_t)((scratch_pa + 0x10) >> 32));
			gpu_wr(gpu, R_CP_RB_BASE * 4u, (uint32_t)ring_pa);
			gpu_wr(gpu, R_CP_RB_BASE_HI * 4u, (uint32_t)(ring_pa >> 32));
			gpu_wr(gpu, 0x083bu * 4u, 0u);
			gpu_wr(gpu, R_CP_RB_CNTL * 4u, 11u | (2u << 8) | (1u << 27));
			sweet_gpu_step(0x94);
			cp_rb_n = 0;
			cp_emit(cp_pkt7(CP_ME_INIT_OP, 8));
			cp_emit(0x2fu);
			cp_emit(0x3u);
			cp_emit(0x20000000u);
			cp_emit(0u);
			cp_emit(0u);
			cp_emit(0u);
			cp_emit(0u);
			cp_emit(0u);
			cp_emit(cp_pkt7(CP_MEM_WRITE_OP, 3));
			cp_emit((uint32_t)scratch_pa);
			cp_emit((uint32_t)(scratch_pa >> 32));
			cp_emit(0xc0ffee01u);
			/* Leave secure mode (zap already authenticated). */
			cp_emit(cp_pkt7(0x66u, 1));
			cp_emit(0u);
			cp_emit(cp_pkt7(0x26u, 0));
			cp_emit(cp_pkt7(CP_MEM_WRITE_OP, 3));
			cp_emit((uint32_t)scratch_pa);
			cp_emit((uint32_t)(scratch_pa >> 32));
			cp_emit(0xc0ffee02u);
			cp_emit(cp_pkt7(CP_NOP_OP, 0));
			if (ringp != cp_ring) {
				memcpy(ringp, cp_ring, cp_rb_n * 4u);
			}
			flush_dcache(ring_pa, PAGE_SIZE, TRUE);
			flush_dcache(pm4_base, pm4_bytes, TRUE);
			flush_dcache(pfp_base, pfp_bytes, TRUE);
			sweet_gpu_step(0x95);
			printf("SWEET GPU: CP cfg rb=%#x/%#x cntl=%#x rptr=%u wptr=%u ctx=%#x ib=%#x/%#x pfp=%#x/%#x me=%#x stat=%#x wfi=%#x\n",
			    gpu_rd(gpu, R_CP_RB_BASE * 4u), gpu_rd(gpu, (R_CP_RB_BASE + 1u) * 4u),
			    gpu_rd(gpu, R_CP_RB_CNTL * 4u), gpu_rd(gpu, R_CP_RB_RPTR * 4u),
			    gpu_rd(gpu, R_CP_RB_WPTR * 4u), gpu_rd(gpu, 0x083bu * 4u),
			    gpu_rd(gpu, R_CP_ME_INSTR_BASE * 4u),
			    gpu_rd(gpu, (R_CP_ME_INSTR_BASE + 1u) * 4u),
			    gpu_rd(gpu, R_CP_PFP_INSTR_BASE * 4u),
			    gpu_rd(gpu, (R_CP_PFP_INSTR_BASE + 1u) * 4u),
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u), gpu_rd(gpu, 0x0b14u * 4u),
			    gpu_rd(gpu, 0x0b15u * 4u));
			gpu_wr(gpu, 0x3080u * 4u, 0u);
			gpu_wr(gpu, R_CP_PFP_ME_CNTL * 4u, 0u);
			printf("SWEET GPU: CP release ME=%#x VBIF=%#x\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u), gpu_rd(gpu, 0x3081u * 4u));
			gpu_wr(gpu, R_CP_RB_WPTR * 4u, cp_rb_n);
			int consumed = 0;
			for (int i = 0; i < 200; ++i) {
				if (gpu_rd(gpu, R_CP_RB_RPTR * 4u) == cp_rb_n) {
					consumed = 1; break;
				}
				delay(1000);
			}
			sweet_gpu_step(0x96);
			delay(1000);
			flush_dcache(scratch_pa, PAGE_SIZE, TRUE);
			uint32_t got = scratchp[0];
			printf("SWEET GPU: CP consumed=%d RPTR=%u WPTR=%u(want %u) cntl=%#x scratch=%#x %s\n",
			    consumed, gpu_rd(gpu, R_CP_RB_RPTR * 4u),
			    gpu_rd(gpu, R_CP_RB_WPTR * 4u), cp_rb_n,
			    gpu_rd(gpu, R_CP_RB_CNTL * 4u), got,
			    got == 0xc0ffee01u ? "CP WORKS" : "no write");
			printf("SWEET GPU: CP ME_CNTL=%#x RBBM=%#x HW_FAULT=%#x\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u),
			    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, R_CP_HW_FAULT * 4u));
			if (level >= 8) {
				vm_offset_t sm3 = ml_io_map(0x05040000u, 0x00010000u);
				if (sm3) {
					vm_offset_t cb3 = sm3 + 0x8000u;
					printf("SWEET GPU: post-CP sGFSR=%#x CB0 FSR=%#x FAR=%#x%08x FSYNR0=%#x SMR0=%#x S2CR0=%#x SCTLR=%#x\n",
					    gpu_rd(sm3, 0x48), gpu_rd(cb3, 0x58),
					    gpu_rd(cb3, 0x64), gpu_rd(cb3, 0x60), gpu_rd(cb3, 0x68),
					    gpu_rd(sm3, 0x800), gpu_rd(sm3, 0xc00), gpu_rd(cb3, 0x0));
					printf("SWEET GPU: SMMU banks SMR1=%#x S2CR1=%#x SMR2=%#x S2CR2=%#x CB1=%#x CB2=%#x sACR=%#x\n",
					    gpu_rd(sm3, 0x804), gpu_rd(sm3, 0xc04),
					    gpu_rd(sm3, 0x808), gpu_rd(sm3, 0xc08),
					    gpu_rd(sm3 + 0x9000u, 0x0), gpu_rd(sm3 + 0xa000u, 0x0),
					    gpu_rd(sm3, 0x10));
				}
			}
			printf("SWEET GPU: CP later ME=%#x VBIF=%#x CP_INT=%#x INT=%#x ST3=%#x trust=%#x gpmu=%#x/%#x cm3=%#x\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u), gpu_rd(gpu, 0x3081u * 4u),
			    gpu_rd(gpu, R_CP_INTERRUPT_STATUS * 4u), gpu_rd(gpu, 0x04e1u * 4u),
			    gpu_rd(gpu, 0x0530u * 4u), gpu_rd(gpu, 0xf400u * 4u),
			    gpu_rd(gpu, 0xa8e0u * 4u), gpu_rd(gpu, 0xa8e1u * 4u),
			    gpu_rd(gpu, 0xa8d8u * 4u));
			printf("SWEET GPU: CP ahb_err=%#x status3=%#x\n",
			    gpu_rd(gpu, 0x04f4u * 4u), gpu_rd(gpu, 0x0530u * 4u));
			printf("SWEET GPU: CP micro2 ME=%#x st=%#x waddr=%#x raddr=%#x data=%#x stat=%#x wfi=%#x\n",
			    gpu_rd(gpu, R_CP_PFP_ME_CNTL * 4u), gpu_rd(gpu, 0x01f7u * 4u),
			    gpu_rd(gpu, 0x01f8u * 4u), gpu_rd(gpu, 0x01f9u * 4u),
			    gpu_rd(gpu, 0x01fau * 4u), gpu_rd(gpu, 0x0b14u * 4u),
			    gpu_rd(gpu, 0x0b15u * 4u));
			sweet_gpu_step(0x97);
		}
	}

	if (level >= 13) {
		sweet_gpu_step(0xb1);
		a540_want_gpmu = (level >= 13); /* v207: GPMU off at level 12 */
		printf("SWEET GPU: A540 pre-zap A540 start (gpmu=%d)\n", a540_want_gpmu);
		int ok13 = sweet_a540_start(0);
		printf("SWEET GPU: A540 pre-zap %s\n", ok13 ? "OK" : "FAIL");
	}

	if (level >= 12 && level < 13) {
		sweet_gpu_step(0xa1);
		printf("SWEET GPU: A540 start enter\n");
		int ok = sweet_a540_start(0); /* v206: no secure CB writes post-auth either */
		printf("SWEET GPU: A540 start %s\n", ok ? "OK" : "FAIL");
		sweet_gpu_step(0xa2);
	}

	int pause = 0;
	if (PE_parse_boot_argn("sweetgpupause", &pause, (int)sizeof(pause)) &&
	    pause > 0 && pause <= 150) {
		printf("SWEET GPU: holding console %d s\n", pause);
		delay(pause * 1000000);
	}
	{
		int hold = 0;
		if (PE_parse_boot_argn("sweetgpuhalt", &hold,
		    (int)sizeof(hold)) && hold == 1) {
			printf("SWEET GPU: sweetgpuhalt=1, spinning; WDT/auto-reset recovers\n");
			for (;;)
				__asm__ volatile("wfe" ::: "memory");
		}
	}
	printf("SWEET GPU: probe done\n");
}
#endif /* ARM64_BOARD_CONFIG_SWEET */
