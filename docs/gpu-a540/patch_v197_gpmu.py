#!/usr/bin/env python3
# v197: A540 GPMU bring-up after ME_INIT (pre-zap, level 14), ported from
# sagit AdrenoStartup::gpmu + Firmware::gpmuCommands.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

if 'a540 gpmu' in s:
    print('v197 already applied')
    raise SystemExit(0)

# pkt4 helper after cp_pkt7
anchor = '''static uint32_t
cp_pkt7(uint32_t op, uint32_t cnt)
{
	return (7u << 28) | cnt | (cp_parity(cnt) << 15) |
	    ((op & 0x7fu) << 16) | (cp_parity(op) << 23);
}
'''
assert s.count(anchor) == 1
helper = anchor + '''
static uint32_t
cp_pkt4(uint32_t reg, uint32_t cnt)
{
	return (4u << 28) | cnt | (cp_parity(cnt) << 7) |
	    ((reg & 0x3ffffu) << 8) | (cp_parity(reg) << 27);
}
'''
s = s.replace(anchor, helper)

# global flag near the a540 statics
anchor = 'static uint64_t smmu_l1[2048] __attribute__((aligned(16384)));\n'
assert s.count(anchor) == 1
s = s.replace(anchor, 'static int a540_want_gpmu;\n' + anchor)

# insert GPMU block before the final consumed print/return of sweet_a540_start
anchor = '''	uint32_t got = scratch[0];
	printf("SWEET GPU: a540 consumed=%d RPTR=%u WPTR=%u scratch=%#x RBBM=%#x FAULT=%#x\\n",
	    consumed, gpu_rd(gpu, 0x0806u * 4u), n, got,
	    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, 0x0b1au * 4u));
	return consumed && got == 0xc0ffee01u;
'''
assert s.count(anchor) == 1
gpmu = r'''	uint32_t got = scratch[0];
	printf("SWEET GPU: a540 consumed=%d RPTR=%u WPTR=%u scratch=%#x RBBM=%#x FAULT=%#x\n",
	    consumed, gpu_rd(gpu, 0x0806u * 4u), n, got,
	    gpu_rd(gpu, 0x04f5u * 4u), gpu_rd(gpu, 0x0b1au * 4u));
	if (!(consumed && got == 0xc0ffee01u)) {
		return 0;
	}
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
		/* CP: protected-mode off, write commands to 0x8800+, on. */
		cp_rb_n = 0;
		cp_emit(cp_pkt7(0x5fu, 1));
		cp_emit(0u);
		cp_emit(cp_pkt7(0x3fu, 3));
		cp_emit((uint32_t)cmd_pa);
		cp_emit((uint32_t)(cmd_pa >> 32));
		cp_emit(cnt);
		cp_emit(cp_pkt7(0x5fu, 1));
		cp_emit(1u);
		if (ring != cp_ring) {
			memcpy(ring, cp_ring, cp_rb_n * 4u);
		}
		flush_dcache(ring_pa, 32768, TRUE);
		gpu_wr(gpu, 0x0807u * 4u, cp_rb_n);
		int gread = 0;
		for (int i = 0; i < 300; ++i) {
			if (gpu_rd(gpu, 0x0806u * 4u) == cp_rb_n) {
				gread = 1; break;
			}
			delay(1000);
		}
		gpu_wr(gpu, 0xa8d8u * 4u, 0u);		/* release CM3 */
		int gready = 0;
		for (int i = 0; i < 300; ++i) {
			if (gpu_rd(gpu, 0xa8e0u * 4u) == 0xbabefaceu) {
				gready = 1; break;
			}
			delay(1000);
		}
		printf("SWEET GPU: a540 gpmu read=%d ready=%d (%#x/%#x) cm3=%#x\n",
		    gread, gready, gpu_rd(gpu, 0xa8e0u * 4u),
		    gpu_rd(gpu, 0xa8e1u * 4u), gpu_rd(gpu, 0xa8d8u * 4u));
	}
	return 1;
'''
s = s.replace(anchor, gpmu)

# level 14 flag in the pre-zap block
old = '''		sweet_gpu_step(0xb1);
		printf("SWEET GPU: A540 pre-zap A540 start\\n");
		int ok13 = sweet_a540_start();
'''
new = '''		sweet_gpu_step(0xb1);
		a540_want_gpmu = (level >= 14);
		printf("SWEET GPU: A540 pre-zap A540 start (gpmu=%d)\\n", a540_want_gpmu);
		int ok13 = sweet_a540_start();
'''
assert s.count(old) == 1
s = s.replace(old, new)

old = '\tif (level >= 12 && level != 13) {\n'
new = '\tif (level >= 12 && level < 13) {\n'
assert s.count(old) == 1, s.count(old)
s = s.replace(old, new)
open(p, 'w').write(s)
print('v197 applied')
