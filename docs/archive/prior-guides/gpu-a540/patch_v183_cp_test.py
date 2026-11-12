#!/usr/bin/env python3
# v183: CP bring-up test (level 9): ring + PFP/PM4 ucode + ME_INIT +
# CP_MEM_WRITE self-check, no zap (CP stays secure). Step codes 0x90..0x98.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'CP test enter' in s:
    print('v183 already applied')
    raise SystemExit(0)

# includes for device tree
old = '#include <pexpert/pexpert.h>\n'
assert s.count(old) == 1
s = s.replace(old, old + '#include <pexpert/device_tree.h>\n')

# constants + statics after the PON defines
anchor = '#define PON_PPID 0x008u\n#define PON_SCRATCH 0x088eu\n'
assert s.count(anchor) == 1
consts = '''
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

#define CP_ME_INIT_OP 0x48u
#define CP_MEM_WRITE_OP 0x3du
#define CP_NOP_OP 0x10u

static uint32_t cp_ring[4096] __attribute__((aligned(16384)));
static uint32_t cp_scratch[4096] __attribute__((aligned(16384)));
static uint32_t cp_rb_n;

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

static void
cp_emit(uint32_t v)
{
	cp_ring[cp_rb_n] = v;
	cp_rb_n = (cp_rb_n + 1) & 4095u;
}

'''
s = s.replace(anchor, anchor + consts)

# CP block before pause
start = s.index('\tint pause = 0;')
block = (
    '\tif (level >= 9) {\n'
    '\t\tsweet_gpu_step(0x90);\n'
    '\t\tprintf("SWEET GPU: CP test enter\\n");\n'
    '\t\tvm_offset_t gpu = ml_io_map(0x05000000u, 0x00040000u);\n'
    '\t\tuint64_t pm4 = 0, pfp = 0;\n'
    '\t\tuint32_t pm4_len = 0, pfp_len = 0;\n'
    '\t\tint have = blob("GPU-PM4", &pm4, &pm4_len) &&\n'
    '\t\t    blob("GPU-PFP", &pfp, &pfp_len);\n'
    '\t\tprintf("SWEET GPU: CP blobs %d pm4=%#llx(%u) pfp=%#llx(%u)\\n",\n'
    '\t\t    have, pm4, pm4_len, pfp, pfp_len);\n'
    '\t\tif (gpu && have && pm4_len > 4 && pfp_len > 4) {\n'
    '\t\t\tsweet_gpu_step(0x91);\n'
    '\t\t\tuint64_t ring_pa = (uint64_t)kvtophys((vm_offset_t)cp_ring);\n'
    '\t\t\tuint64_t scratch_pa = (uint64_t)kvtophys((vm_offset_t)cp_scratch);\n'
    '\t\t\tprintf("SWEET GPU: CP ring=%#llx scratch=%#llx\\n", ring_pa, scratch_pa);\n'
    '\t\t\tsweet_gpu_step(0x92);\n'
    '\t\t\t/* A530 subset of a5xx_hw_init. */\n'
    '\t\t\tgpu_wr(gpu, R_VBIF_RR_QOS_ARB * 4u, 3u);\n'
    '\t\t\tgpu_wr(gpu, R_RBBM_PERFCTR_GPU_BUSY_MASKED * 4u, 0xffffffffu);\n'
    '\t\t\tgpu_wr(gpu, R_RBBM_AHB_CNTL0 * 4u, 1u);\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_CACHE_WAYS * 4u, 2u);\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_TRAP_BASE * 4u, 0x0001ffffu);\n'
    '\t\t\tgpu_wr(gpu, (R_UCHE_TRAP_BASE + 1u) * 4u, 0x0001ffffu);\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_WRITE_THRU_BASE * 4u, 0x0001ffffu);\n'
    '\t\t\tgpu_wr(gpu, (R_UCHE_WRITE_THRU_BASE + 1u) * 4u, 0x0001ffffu);\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_GMEM_RANGE_MIN * 4u, 0x00100000u);\n'
    '\t\t\tgpu_wr(gpu, (R_UCHE_GMEM_RANGE_MIN + 1u) * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_GMEM_RANGE_MAX * 4u, 0x001fffffu);\n'
    '\t\t\tgpu_wr(gpu, (R_UCHE_GMEM_RANGE_MAX + 1u) * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, R_CP_MEQ_THRESHOLDS * 4u, 0x40u);\n'
    '\t\t\tgpu_wr(gpu, R_CP_MERCIU_SIZE * 4u, 0x40u);\n'
    '\t\t\tgpu_wr(gpu, R_CP_ROQ_THRESHOLDS_2 * 4u, 0x80000060u);\n'
    '\t\t\tgpu_wr(gpu, R_CP_ROQ_THRESHOLDS_1 * 4u, 0x40201b16u);\n'
    '\t\t\tgpu_wr(gpu, R_PC_DBG_ECO_CNTL * 4u, (0x400u << 11) | (0x300u << 22));\n'
    '\t\t\tgpu_wr(gpu, R_UCHE_MODE_CNTL * 4u, 1u << 29);\n'
    '\t\t\tgpu_wr(gpu, R_CP_CHICKEN_DBG * 4u, 0x02000000u);\n'
    '\t\t\tgpu_wr(gpu, R_RBBM_AHB_CNTL1 * 4u, 0xa6ffffffu);\n'
    '\t\t\tgpu_wr(gpu, R_RBBM_AHB_CNTL2 * 4u, 0x3fu);\n'
    '\t\t\tgpu_wr(gpu, R_TPL1_MODE_CNTL * 4u, 2u << 7);\n'
    '\t\t\tgpu_wr(gpu, R_RB_MODE_CNTL * 4u, 2u << 1);\n'
    '\t\t\tgpu_wr(gpu, R_VPC_DBG_ECO_CNTL * 4u,\n'
    '\t\t\t    gpu_rd(gpu, R_VPC_DBG_ECO_CNTL * 4u) | (1u << 10));\n'
    '\t\t\tgpu_wr(gpu, R_SECVID_TSB_CNTL * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, R_SECVID_TSB_BASE * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, (R_SECVID_TSB_BASE + 1u) * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, R_SECVID_TSB_SIZE * 4u, 0u);\n'
    '\t\t\tstatic const uint16_t addr_mode[] = {\n'
    '\t\t\t    0x0860u, 0x0bc1u, 0x0c81u, 0x0cc5u, 0x0d01u, 0x0e05u,\n'
    '\t\t\t    0x0e41u, 0x0e61u, 0x0e80u, 0x0ec1u, 0x0f01u, 0xf810u };\n'
    '\t\t\tfor (unsigned i = 0; i < sizeof(addr_mode) / sizeof(addr_mode[0]); i++) {\n'
    '\t\t\t\tgpu_wr(gpu, addr_mode[i] * 4u, 1u);\n'
    '\t\t\t}\n'
    '\t\t\tsweet_gpu_step(0x93);\n'
    '\t\t\t/* CP instruction bases, firmware files carry a 4-byte header. */\n'
    '\t\t\tgpu_wr(gpu, R_CP_ME_INSTR_BASE * 4u, (uint32_t)(pm4 + 4));\n'
    '\t\t\tgpu_wr(gpu, (R_CP_ME_INSTR_BASE + 1u) * 4u, (uint32_t)((pm4 + 4) >> 32));\n'
    '\t\t\tgpu_wr(gpu, R_CP_PFP_INSTR_BASE * 4u, (uint32_t)(pfp + 4));\n'
    '\t\t\tgpu_wr(gpu, (R_CP_PFP_INSTR_BASE + 1u) * 4u, (uint32_t)((pfp + 4) >> 32));\n'
    '\t\t\tgpu_wr(gpu, R_CP_RB_BASE * 4u, (uint32_t)ring_pa);\n'
    '\t\t\tgpu_wr(gpu, R_CP_RB_BASE_HI * 4u, (uint32_t)(ring_pa >> 32));\n'
    '\t\t\tgpu_wr(gpu, R_CP_RB_CNTL * 4u, 11u | (2u << 8) | (1u << 27));\n'
    '\t\t\tsweet_gpu_step(0x94);\n'
    '\t\t\t/* ME_INIT + CP_MEM_WRITE(0xc0ffee01). */\n'
    '\t\t\tcp_rb_n = 0;\n'
    '\t\t\tcp_emit(cp_pkt7(CP_ME_INIT_OP, 8));\n'
    '\t\t\tcp_emit(0x2fu);\n'
    '\t\t\tcp_emit(0x3u);\n'
    '\t\t\tcp_emit(0x20000000u);\n'
    '\t\t\tcp_emit(0u);\n'
    '\t\t\tcp_emit(0u);\n'
    '\t\t\tcp_emit(0xbu);\n'
    '\t\t\tcp_emit(0u);\n'
    '\t\t\tcp_emit(0u);\n'
    '\t\t\tcp_emit(cp_pkt7(CP_MEM_WRITE_OP, 3));\n'
    '\t\t\tcp_emit((uint32_t)scratch_pa);\n'
    '\t\t\tcp_emit((uint32_t)(scratch_pa >> 32));\n'
    '\t\t\tcp_emit(0xc0ffee01u);\n'
    '\t\t\tcp_emit(cp_pkt7(CP_NOP_OP, 0));\n'
    '\t\t\tflush_dcache((vm_offset_t)cp_ring, PAGE_SIZE, FALSE);\n'
    '\t\t\tflush_dcache(pm4 + 4, pm4_len - 4, TRUE);\n'
    '\t\t\tflush_dcache(pfp + 4, pfp_len - 4, TRUE);\n'
    '\t\t\tsweet_gpu_step(0x95);\n'
    '\t\t\tgpu_wr(gpu, R_CP_PFP_ME_CNTL * 4u, 0u);\n'
    '\t\t\tgpu_wr(gpu, R_CP_RB_WPTR * 4u, cp_rb_n);\n'
    '\t\t\tint consumed = 0;\n'
    '\t\t\tfor (int i = 0; i < 200; ++i) {\n'
    '\t\t\t\tif (gpu_rd(gpu, R_CP_RB_RPTR * 4u) == cp_rb_n) {\n'
    '\t\t\t\t\tconsumed = 1; break;\n'
    '\t\t\t\t}\n'
    '\t\t\t\tdelay(1000);\n'
    '\t\t\t}\n'
    '\t\t\tsweet_gpu_step(0x96);\n'
    '\t\t\tdelay(1000);\n'
    '\t\t\tflush_dcache((vm_offset_t)cp_scratch, PAGE_SIZE, FALSE);\n'
    '\t\t\tuint32_t got = cp_scratch[0];\n'
    '\t\t\tprintf("SWEET GPU: CP consumed=%d RPTR=%u WPTR=%u scratch=%#x %s\\n",\n'
    '\t\t\t    consumed, gpu_rd(gpu, R_CP_RB_RPTR * 4u), cp_rb_n, got,\n'
    '\t\t\t    got == 0xc0ffee01u ? "CP WORKS" : "no write");\n'
    '\t\t\tprintf("SWEET GPU: CP RBBM=%#x HW_FAULT=%#x INT=%#x\\n",\n'
    '\t\t\t    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, R_CP_HW_FAULT * 4u),\n'
    '\t\t\t    gpu_rd(gpu, R_CP_INTERRUPT_STATUS * 4u));\n'
    '\t\t\tsweet_gpu_step(0x97);\n'
    '\t\t}\n'
    '\t}\n\n'
)
s = s[:start] + block + s[start:]
open(p, 'w').write(s)
print('v183 applied')
