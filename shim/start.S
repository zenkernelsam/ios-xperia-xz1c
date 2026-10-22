/*
 * sweet boot shim entry: arm64 Linux Image header so Qualcomm ABL treats
 * us like a kernel. ABL enters at EL1, MMU/D-cache off, x0 = DTB pointer.
 */
	.section .text.head, "ax"
	.globl _head
_head:
	b	_start			/* code0 */
	.long	0			/* code1 */
	.quad	0			/* text_offset */
	.quad	_end			/* image_size (linked at 0) */
	.quad	0xa			/* flags: LE, 4K pages, place anywhere */
	.quad	0			/* res2 */
	.quad	0			/* res3 */
	.quad	0			/* res4 */
	.ascii	"ARM\x64"		/* magic */
	.long	0			/* res5 (PE offset) */

	.text
	.globl _start
_start:
	msr	daifset, #0xf
	mov	x19, x0			/* DTB pointer from ABL */
	msr	spsel, #1
	adr	x1, _stack_top
	mov	sp, x1
	adr	x1, vectors
	msr	vbar_el1, x1
	isb
	/* zero .bss */
	adr	x1, _bss_start
	adr	x2, _bss_end
1:	cmp	x1, x2
	b.hs	2f
	str	xzr, [x1], #8
	b	1b
2:	mov	x0, x19
	bl	shim_main
3:	wfe
	b	3b

/* PSCI CPU_ON target. x0 = context id = cpu index (1..7). */
	.globl secondary_entry
secondary_entry:
	msr	daifset, #0xf
	msr	spsel, #1
	adr	x1, _sec_stacks
	add	x1, x1, x0, lsl #12
	add	x1, x1, #0x1000
	mov	sp, x1
	adr	x1, vectors
	msr	vbar_el1, x1
	isb
	bl	secondary_main
4:	wfe
	b	4b

/*
 * Exception vectors. Each slot stashes x29/x30, loads its slot number
 * and jumps to the common path, which saves x0-x28 and calls
 * exc_handler(slot, regs). IRQs return; everything else ends in the
 * on-screen panic.
 */
.macro vent slot
	.balign	0x80
	stp	x29, x30, [sp, #-16]!
	mov	x29, #\slot
	b	exc_common
.endm

	.balign	0x800
	.globl vectors
vectors:
	vent 0
	vent 1
	vent 2
	vent 3
	vent 4
	vent 5
	vent 6
	vent 7
	vent 8
	vent 9
	vent 10
	vent 11
	vent 12
	vent 13
	vent 14
	vent 15

exc_common:
	sub	sp, sp, #(30 * 8)
	stp	x0, x1, [sp, #0]
	stp	x2, x3, [sp, #16]
	stp	x4, x5, [sp, #32]
	stp	x6, x7, [sp, #48]
	stp	x8, x9, [sp, #64]
	stp	x10, x11, [sp, #80]
	stp	x12, x13, [sp, #96]
	stp	x14, x15, [sp, #112]
	stp	x16, x17, [sp, #128]
	stp	x18, x19, [sp, #144]
	stp	x20, x21, [sp, #160]
	stp	x22, x23, [sp, #176]
	stp	x24, x25, [sp, #192]
	stp	x26, x27, [sp, #208]
	str	x28, [sp, #224]
	mov	x0, x29
	mov	x1, sp
	bl	exc_handler
	ldp	x0, x1, [sp, #0]
	ldp	x2, x3, [sp, #16]
	ldp	x4, x5, [sp, #32]
	ldp	x6, x7, [sp, #48]
	ldp	x8, x9, [sp, #64]
	ldp	x10, x11, [sp, #80]
	ldp	x12, x13, [sp, #96]
	ldp	x14, x15, [sp, #112]
	ldp	x16, x17, [sp, #128]
	ldp	x18, x19, [sp, #144]
	ldp	x20, x21, [sp, #160]
	ldp	x22, x23, [sp, #176]
	ldp	x24, x25, [sp, #192]
	ldp	x26, x27, [sp, #208]
	ldr	x28, [sp, #224]
	add	sp, sp, #(30 * 8)
	ldp	x29, x30, [sp], #16
	eret

/*
 * vectors2 — table installed right before jumping into the xnu payload.
 * The kernel has no vectors of its own yet, so anything it raises lands
 * here. Every slot dumps the exception registers into IMEM (survives a
 * WDT bite, readable from Android via /dev/fmem), writes 0x7766F0<slot>
 * to the restart_reason cell and spins for the watchdog.
 *
 * Exc dump area: 0x146bf780..0x146bf7ff  (pre-jump state dump uses
 * 0x146bf700..0x146bf77f; both inside the IMEM page at 0x146bf000).
 */
#define IMEM_DUMP	0x146bf780
#define IMEM_RR		0x146bf65c

.macro vent2 slot
	.balign	0x80
	mov	w9, #\slot
	b	exc2_common
.endm

	.balign	0x800
	.globl vectors2
vectors2:
	vent2 0
	vent2 1
	vent2 2
	vent2 3
	vent2 4
	vent2 5
	vent2 6
	vent2 7
	vent2 8
	vent2 9
	vent2 10
	vent2 11
	vent2 12
	vent2 13
	vent2 14
	vent2 15

exc2_common:
	mov	w10, #(IMEM_DUMP & 0xffff)
	movk	w10, #(IMEM_DUMP >> 16), lsl #16
	str	w9, [x10]			/* +0x00 slot */
	mrs	x11, CurrentEL
	str	w11, [x10, #4]			/* +0x04 CurrentEL */
	mov	x11, sp
	str	x11, [x10, #8]			/* +0x08 SP */
	str	x30, [x10, #16]			/* +0x10 LR */
	stp	x0, x1, [x10, #0x40]		/* GPRs */
	stp	x2, x3, [x10, #0x50]
	stp	x4, x5, [x10, #0x60]
	stp	x6, x7, [x10, #0x70]
	mrs	x11, CurrentEL
	cmp	x11, #8				/* taken at EL2? */
	b.eq	1f
	mrs	x11, ESR_EL1
	str	x11, [x10, #0x18]
	mrs	x11, ELR_EL1
	str	x11, [x10, #0x20]
	mrs	x11, FAR_EL1
	str	x11, [x10, #0x28]
	mrs	x11, SPSR_EL1
	str	x11, [x10, #0x30]
	b	2f
1:	mrs	x11, ESR_EL2
	str	x11, [x10, #0x18]
	mrs	x11, ELR_EL2
	str	x11, [x10, #0x20]
	mrs	x11, FAR_EL2
	str	x11, [x10, #0x28]
	mrs	x11, SPSR_EL2
	str	x11, [x10, #0x30]
2:	mov	w11, #0xf000
	orr	w11, w11, w9			/* mark = 0x7766F000|slot */
	movk	w11, #0x7766, lsl #16
	str	w11, [x10, #0x38]
	mov	w12, #(IMEM_RR & 0xffff)
	movk	w12, #(IMEM_RR >> 16), lsl #16
	str	w11, [x12]
	dsb	sy
	isb
3:	wfe
	b	3b

/*
 * hios_return_stub — return address handed to a hackios-style kernel via
 * its "XNULDRST" slot: it jumps here (MMU off, physical address) with
 *   x0/x1 = kernel log PA/len, x2 = reason (0 orderly, 1 early exception,
 *   2 checkpoint), x3..x7 = ESR, ELR, FAR, LR, SP on reason 1.
 * We stash the whole incoming context at 0x146bf800..0x146bf87f, write
 * 0x7766E7<reason*16+15> to restart_reason and spin for the watchdog.
 */
	.balign	64
	.globl	hios_return_stub
hios_return_stub:
	msr	daifset, #0xf
	mrs	x9, sctlr_el1
	bic	x9, x9, #1			/* MMU off, whatever the kernel left */
	msr	sctlr_el1, x9
	isb
	mov	w10, #(0x146bf800 & 0xffff)
	movk	w10, #(0x146bf800 >> 16), lsl #16
	stp	x0, x1, [x10, #0x00]		/* log PA + len */
	stp	x2, x3, [x10, #0x10]		/* reason + ESR */
	stp	x4, x5, [x10, #0x20]		/* ELR + FAR */
	stp	x6, x7, [x10, #0x30]		/* LR + SP */
	mov	w11, #0xe700
	orr	w11, w11, w2, lsl #4		/* mark = 0x7766E700|reason<<4 */
	movk	w11, #0x7766, lsl #16
	str	w11, [x10, #0x40]
	mov	w12, #(IMEM_RR & 0xffff)
	movk	w12, #(IMEM_RR >> 16), lsl #16
	str	w11, [x12]
	dsb	sy
	isb
4:	wfe
	b	4b
