#!/usr/bin/env python3
# v168: GPUCC reachability probe (level 4).
#   - enable GCC GPU_CFG_AHB (GCC 0x100000 + 0x71004 bit0) so GPUCC regs clock
#   - read GPUCC CX/GX GDSCR, GX clamp/reset, gfx3d RCG/branch status
#   - no GDSC writes yet
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

s = s.replace("""extern int sweet_spmi_describe(unsigned ppid);
""", """extern int sweet_spmi_describe(unsigned ppid);
extern vm_offset_t ml_io_map(vm_offset_t, vm_size_t);

#define GCC_PA 0x00100000u
#define GCC_SIZE 0x000b0000u
#define GCC_GPU_CFG_AHB_CBCR 0x00071004u
#define GPUCC_PA 0x05065000u
#define GPUCC_SIZE 0x00009000u
""", 1)

s = s.replace("""void
sweet_gpu_probe(void)
""", """static uint32_t
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

void
sweet_gpu_probe(void)
""", 1)

s = s.replace("""	int pause = 0;
""", """	if (level >= 4) {
		vm_offset_t gcc = ml_io_map(GCC_PA + 0x71000u, 0x1000);
		vm_offset_t gpuc = ml_io_map(GPUCC_PA, 0x2000);
		printf("SWEET GPU: gcc=%#lx gpuc=%#lx\\n", gcc, gpuc);
		if (gcc && gpuc) {
			uint32_t cbc = gpu_rd(gcc, 0x004u);
			printf("SWEET GPU: gpu_cfg_ahb %#x\\n", cbc);
			if (!(cbc & 1u)) {
				gpu_wr(gcc, 0x004u, cbc | 1u);
				delay(100);
				cbc = gpu_rd(gcc, 0x004u);
				printf("SWEET GPU: gpu_cfg_ahb now %#x\\n", cbc);
			}
			printf("SWEET GPU: CX_GDSCR %#x GX_GDSCR %#x\\n",
			    gpu_rd(gpuc, 0x1004), gpu_rd(gpuc, 0x1094));
			printf("SWEET GPU: GX_BCR %#x clamp %#x\\n",
			    gpu_rd(gpuc, 0x1090), gpu_rd(gpuc, 0x0130));
			printf("SWEET GPU: gfx3d_rcg %#x branch %#x\\n",
			    gpu_rd(gpuc, 0x1070), gpu_rd(gpuc, 0x1098));
		}
	}

	int pause = 0;
""", 1)

open(p, 'w').write(s)
print('v168 applied')
