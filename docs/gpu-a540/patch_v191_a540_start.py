#!/usr/bin/env python3
# v191: port the A540 startup sequence from sagit QcomGPU (AdrenoStartup.h):
# RBBM soft reset, ME/VBIF halt, A540 configure, 32 KiB ring, A540 ME_INIT.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'a540_start' in s:
    print('v191 already applied')
    raise SystemExit(0)

anchor = 'static int\nsweet_zap_load(void)\n'
assert s.count(anchor) == 1
func = r'''
static uint32_t cp_pkt7(uint32_t op, uint32_t cnt);
static void cp_emit(uint32_t v);

/* A540 startup, ported from research/ios_sagit/drivers/gpu/QcomGPU/
 * AdrenoStartup.h (A540-only adaptation of Linux a5xx_gpu.c). */
static uint32_t a540_ring[8192] __attribute__((aligned(16384)));
static uint32_t a540_scratch[4096] __attribute__((aligned(16384)));

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
sweet_a540_start(void)
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
	uint64_t pm4_base = pm4 + 4, pfp_base = pfp + 4;
	uint32_t pm4_bytes = pm4_len - 4, pfp_bytes = pfp_len - 4;
	uint64_t ring_pa = (uint64_t)kvtophys((vm_offset_t)a540_ring);
	uint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)a540_scratch);
	memset(a540_ring, 0, sizeof(a540_ring));
	memset(a540_scratch, 0, sizeof(a540_scratch));
	printf("SWEET GPU: a540 ring=%#llx scratch=%#llx pm4=%#llx pfp=%#llx\n",
	    ring_pa, scratch_pa, pm4_base, pfp_base);

	/* reset: halt ME/VBIF, RBBM soft reset, GPMU CM3 reset. */
	gpu_wr(gpu, 0x0038u * 4u, 0u);			/* InterruptMask */
	gpu_wr(gpu, 0x0832u * 4u, A540_ME_HALT);	/* MEControl halt */
	gpu_wr(gpu, 0x3080u * 4u, 15u);			/* VBIF halt */
	delay(100);
	gpu_wr(gpu, 0x0043u * 4u, 1u);			/* RBBM_SW_RESET */
	(void)gpu_rd(gpu, 0x0043u * 4u);
	gpu_wr(gpu, 0x0043u * 4u, 0u);
	delay(100);
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
	static const uint32_t protv[] = {
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
	a540_ring[n++] = cp_pkt7(CP_ME_INIT_OP, 8);
	static const uint32_t init[] = { 0x2fu, 3u, 0x20000000u, 0u, 0u, 0u, 0u, 0u };
	for (unsigned i = 0; i < 8; i++) {
		a540_ring[n++] = init[i];
	}
	a540_ring[n++] = cp_pkt7(CP_MEM_WRITE_OP, 3);
	a540_ring[n++] = (uint32_t)scratch_pa;
	a540_ring[n++] = (uint32_t)(scratch_pa >> 32);
	a540_ring[n++] = 0xc0ffee01u;
	a540_ring[n++] = cp_pkt7(CP_NOP_OP, 0);
	flush_dcache((vm_offset_t)a540_ring, PAGE_SIZE * 2, FALSE);
	gpu_wr(gpu, 0x0807u * 4u, n);
	int consumed = 0;
	for (int i = 0; i < 200; ++i) {
		if (gpu_rd(gpu, 0x0806u * 4u) == n) {
			consumed = 1; break;
		}
		delay(1000);
	}
	delay(1000);
	flush_dcache((vm_offset_t)a540_scratch, PAGE_SIZE, FALSE);
	uint32_t got = a540_scratch[0];
	printf("SWEET GPU: a540 consumed=%d RPTR=%u WPTR=%u scratch=%#x RBBM=%#x FAULT=%#x\n",
	    consumed, gpu_rd(gpu, 0x0806u * 4u), n, got,
	    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, 0x0b1au * 4u));
	return consumed && got == 0xc0ffee01u;
}

'''
s = s.replace(anchor, func + anchor)

# level 12 runs it (after zap/identity/power blocks)
anchor2 = '\tint pause = 0;\n'
assert s.count(anchor2) == 1
block = (
    '\tif (level >= 12) {\n'
    '\t\tsweet_gpu_step(0xa1);\n'
    '\t\tprintf("SWEET GPU: A540 start enter\\n");\n'
    '\t\tint ok = sweet_a540_start();\n'
    '\t\tprintf("SWEET GPU: A540 start %s\\n", ok ? "OK" : "FAIL");\n'
    '\t\tsweet_gpu_step(0xa2);\n'
    '\t}\n\n'
)
s = s.replace(anchor2, block + anchor2)
open(p, 'w').write(s)
print('v191 applied')
