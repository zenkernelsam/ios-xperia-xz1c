#!/usr/bin/env python3
"""v144: pet the WDT through a proper ml_io_map() KVA instead of the early
alias slot (a wrong alias faults inside the timer IRQ)."""
XNU = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
P_VM = XNU + '/osfmk/arm64/arm_vm_init.c'
P_EARLY = XNU + '/osfmk/arm64/sweet_early.c'
P_INIT = XNU + '/bsd/kern/bsd_init.c'

v = open(P_VM).read()
old = '''	/* slot[21]: APSS watchdog (0x17817000), petted from the timer tick. */
	slot[21] = 0x17817000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	sweet_wdt_rst = (volatile uint32_t *)(uintptr_t)
	    (va + 21 * ARM_TT_L2_SIZE + 0x04);
'''
new = '''	/* slot[21]: APSS watchdog (0x17817000), mapped but unused: petting goes
	 * through the ml_io_map KVA installed in bsd_init. */
	slot[21] = 0x17817000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
'''
assert v.count(old) == 1, 'slot21 block'
v = v.replace(old, new, 1)
old2 = '''#define SWEET_EARLY_FB_VA (VM_MIN_KERNEL_ADDRESS + ARM_TT_L2_SIZE)
volatile uint32_t *sweet_wdt_rst;
'''
new2 = '''#define SWEET_EARLY_FB_VA (VM_MIN_KERNEL_ADDRESS + ARM_TT_L2_SIZE)
'''
assert v.count(old2) == 1, 'wdt global'
v = v.replace(old2, new2, 1)
open(P_VM, 'w').write(v)
print('arm_vm_init.c: alias pet removed, slot kept')

e = open(P_EARLY).read()
anchor = '''void
sweet_ddr_mark(uint64_t tag, uint64_t v)
{
'''
assert e.count(anchor) == 1, 'ddr mark anchor'
e = e.replace(anchor, 'volatile uint32_t *sweet_wdt_rst;\n\n' + anchor, 1)
open(P_EARLY, 'w').write(e)
print('sweet_early.c: sweet_wdt_rst defined')

t = open(P_INIT).read()
anchor2 = '''	extern void sweet_pon_arm_fastboot(void);
	sweet_pon_arm_fastboot();
	sweet_early_step_late(155);
'''
add2 = '''	extern void sweet_pon_arm_fastboot(void);
	sweet_pon_arm_fastboot();
	{
		extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);
		extern volatile uint32_t *sweet_wdt_rst;
		if (sweet_wdt_rst == NULL) {
			sweet_wdt_rst = (volatile uint32_t *)
			    (ml_io_map(0x17817000, 0x1000) + 0x04);
		}
	}
	sweet_early_step_late(155);
'''
assert t.count(anchor2) == 1, 'bsd_init step155 anchor'
t = t.replace(anchor2, add2, 1)
open(P_INIT, 'w').write(t)
print('bsd_init.c: WDT KVA installed before mountroot')
