#!/usr/bin/env python3
# v185: port scm_call + zap_load (PAS id 13, carveout 0x95200000) and run it
# before the level-9 CP test; then CP_SET_SECURE_MODE 0 + a second marker.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'zap load' in s:
    print('v185 already applied')
    raise SystemExit(0)

# includes
old = '#include <pexpert/device_tree.h>\n'
assert s.count(old) == 1
s = s.replace(old, old + '#include <string.h>\n')

# externs
old = 'extern addr64_t kvtophys(vm_offset_t);\n'
assert s.count(old) == 1
s = s.replace(old, old + 'extern vm_map_address_t phystokv(uint64_t pa);\n')

# zap code after the CP helper statics
anchor = 'static uint32_t cp_rb_n;\n'
assert s.count(anchor) == 1
zap = '''
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

static int
sweet_zap_load(void)
{
	uint64_t mdt = 0, seg = 0;
	uint32_t mdt_len = 0, seg_len = 0;
	uint64_t meta = (uint64_t)kvtophys((vm_offset_t)zap_meta);

	if (!sweet_gpu_blob("GPU-ZAP-MDT", &mdt, &mdt_len) ||
	    !sweet_gpu_blob("GPU-ZAP-SEG", &seg, &seg_len)) {
		printf("SWEET GPU: zap blobs missing\\n");
		return 0;
	}
	if (mdt_len < 0x34 || mdt_len > PAGE_SIZE) {
		printf("SWEET GPU: zap mdt size %u\\n", mdt_len);
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
		printf("SWEET GPU: zap layout n=%u filesz=%u seg=%u memsz=%u\\n",
		    nload, filesz, seg_len, memsz);
		return 0;
	}
	flush_dcache((vm_offset_t)zap_meta, PAGE_SIZE, FALSE);
	uint64_t r1 = 0;
	int64_t r = sweet_scm_call(2, 1, 2u | (2u << 6), GPU_PAS_ID, meta, 0, &r1);
	printf("SWEET GPU: zap init_image -> %lld/%#llx\\n", r, r1);
	if (r != 0 || r1 != 0) {
		return 0;
	}
	r = sweet_scm_call(2, 2, 3, GPU_PAS_ID, ZAP_PA, hi - lo, &r1);
	printf("SWEET GPU: zap mem_setup %#x+%#x -> %lld/%#llx\\n",
	    ZAP_PA, hi - lo, r, r1);
	if (r != 0 || r1 != 0) {
		return 0;
	}
	vm_offset_t zap = ml_io_map(ZAP_PA, ZAP_SIZE);
	if (!zap) {
		printf("SWEET GPU: zap carveout map failed\\n");
		return 0;
	}
	const uint32_t *src = (const uint32_t *)phystokv(seg);
	for (uint32_t off = 0; off < ((memsz + 3) & ~3u); off += 4) {
		*(volatile uint32_t *)(zap + off) =
		    off < seg_len ? src[off / 4] : 0;
	}
	__asm__ volatile("dsb sy" ::: "memory");
	r = sweet_scm_call(2, 5, 1, GPU_PAS_ID, 0, 0, &r1);
	printf("SWEET GPU: zap auth_and_reset -> %lld/%#llx\\n", r, r1);
	return r == 0 && r1 == 0;
}
'''
s = s.replace(anchor, anchor + zap)

# insert zap block before the CP block
anchor2 = '\tif (level >= 9) {\n'
assert s.count(anchor2) == 1
zapblock = (
    '\tif (level >= 10) {\n'
    '\t\tsweet_gpu_step(0x99);\n'
    '\t\tprintf("SWEET GPU: zap load begin\\n");\n'
    '\t\tzap_ok = sweet_zap_load();\n'
    '\t\tprintf("SWEET GPU: zap %s\\n", zap_ok ? "OK" : "FAIL");\n'
    '\t}\n\n'
)
s = s.replace(anchor2, zapblock + anchor2)

# after the first CP test, if zap ok, emit CP_SET_SECURE_MODE + marker
old = '''			sweet_gpu_step(0x97);
		}
	}
'''
new = '''			sweet_gpu_step(0x97);
			if (zap_ok) {
				cp_rb_n = 0;
				cp_emit(cp_pkt7(CP_SET_SECURE_MODE_OP, 1));
				cp_emit(0u);
				cp_emit(cp_pkt7(CP_MEM_WRITE_OP, 3));
				cp_emit((uint32_t)(scratch_pa + 8));
				cp_emit((uint32_t)((scratch_pa + 8) >> 32));
				cp_emit(0x5ec0de00u);
				cp_emit(cp_pkt7(CP_NOP_OP, 0));
				flush_dcache((vm_offset_t)cp_ring, PAGE_SIZE, FALSE);
				gpu_wr(gpu, R_CP_RB_WPTR * 4u, cp_rb_n);
				int rd2 = 0;
				for (int i = 0; i < 200; ++i) {
					if (gpu_rd(gpu, R_CP_RB_RPTR * 4u) == cp_rb_n) {
						rd2 = 1; break;
					}
					delay(1000);
				}
				delay(1000);
				flush_dcache((vm_offset_t)cp_scratch, PAGE_SIZE, FALSE);
				printf("SWEET GPU: secure-off read=%d marker=%#x\\n",
				    rd2, cp_scratch[2]);
			}
		}
	}
'''
assert s.count(old) == 1
s = s.replace(old, new)

open(p, 'w').write(s)
print('v185 applied')
