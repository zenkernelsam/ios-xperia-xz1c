#!/usr/bin/env python3
"""v143: pet the APSS watchdog from the timer tick.
- arm_vm_init.c: install alias slot[21] -> 0x17817000 and publish the
  WDT_RST alias pointer (same VA arithmetic as the mapping).
- sleh.c: write 1 to that register on every virtual-timer IRQ.
"""
XNU = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
P_VM = XNU + '/osfmk/arm64/arm_vm_init.c'
P_SLEH = XNU + '/osfmk/arm64/sleh.c'

v = open(P_VM).read()

# publish pointer global just before sweet_early_map_final()
anchor0 = '''#define SWEET_EARLY_FB_VA (VM_MIN_KERNEL_ADDRESS + ARM_TT_L2_SIZE)
'''
add0 = anchor0 + '''volatile uint32_t *sweet_wdt_rst;
'''
assert v.count(anchor0) == 1, 'alias macro anchor'
v = v.replace(anchor0, add0, 1)

old = '''	/* Dedicated alias slots: IMEM, durable+pstore, SPMI (2 MiB each). */
	assert(slot[18] == ARM_TTE_EMPTY && slot[19] == ARM_TTE_EMPTY &&
	    slot[20] == ARM_TTE_EMPTY);
'''
new = '''	/* Dedicated alias slots: IMEM, durable+pstore, SPMI, WDT (2 MiB each). */
	assert(slot[18] == ARM_TTE_EMPTY && slot[19] == ARM_TTE_EMPTY &&
	    slot[20] == ARM_TTE_EMPTY && slot[21] == ARM_TTE_EMPTY);
'''
assert v.count(old) == 1, 'alias assert anchor'
v = v.replace(old, new, 1)

anchor = '''	slot[20] = 0x08400000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
'''
add = anchor + '''	/* slot[21]: APSS watchdog (0x17817000), petted from the timer tick. */
	slot[21] = 0x17817000ULL | ARM_TTE_VALID | ARM_TTE_TYPE_BLOCK |
	    ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) |
	    ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DISABLE) |
	    ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_PNX | ARM_TTE_BLOCK_NX;
	sweet_wdt_rst = (volatile uint32_t *)(uintptr_t)
	    (va + 21 * ARM_TT_L2_SIZE + 0x04);
'''
assert v.count(anchor) == 1, 'slot20 anchor'
v = v.replace(anchor, add, 1)
open(P_VM, 'w').write(v)
print('arm_vm_init.c: WDT alias slot[21] + sweet_wdt_rst')

s = open(P_SLEH).read()
anchor2 = '''	if (intid == sweet_vtimer_intid) {
		/* Auto-return black tech: ~90 s after the first timer tick, arm
'''
add2 = '''	if (intid == sweet_vtimer_intid) {
		/* Pet the APSS watchdog: start.s pets it once; without this the
		 * 120 s hardware bite masquerades as a userspace reboot. */
		{
			extern volatile uint32_t *sweet_wdt_rst;
			if (sweet_wdt_rst) {
				*sweet_wdt_rst = 1;
			}
		}
		/* Auto-return black tech: ~90 s after the first timer tick, arm
'''
assert s.count(anchor2) == 1, 'vtimer anchor'
s = s.replace(anchor2, add2, 1)
open(P_SLEH, 'w').write(s)
print('sleh.c: WDT petted every timer tick via sweet_wdt_rst')
