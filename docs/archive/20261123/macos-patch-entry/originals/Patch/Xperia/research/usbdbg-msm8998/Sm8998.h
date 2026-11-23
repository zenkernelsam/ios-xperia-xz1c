/* SPDX-License-Identifier: MIT
 * QcomXnuUsbDebug SoC profile — MSM8998 (Sony yoshino/lilac), DRAFT.
 * Every value sourced from sony-kernel-4.4 stock DTS/clk driver; see
 * PORTING-NOTES-MSM8998.md for the evidence table and open items.
 *
 * BIG DELTA vs SM8550: USB3 on MSM8998 has NO SMMU (no iommus on
 * ssusb@a800000). DMA is direct-PA. The "private S1 domain" layer must be
 * replaced by a contiguous-PA arena platform variant — do NOT fabricate an
 * SMMU. Also: board caps USB at high-speed -> QMP SS PHY unused.
 */
#ifndef QUD_SM8998_H
#define QUD_SM8998_H
#define QUD_SOC_NAME "msm8998-lilac-inherited"
#define QUD_USB_VID 0x1209u
#define QUD_USB_PID 0x316eu /* provisional: bump when allocated */
#define QUD_USB_SERIAL "QcomXnuUsbDebug"
#define QUD_USB_PRODUCT "Qualcomm XNU Debug (lilac)"

/* GCC — qcom,gcc@100000 (same base as SM8550) */
#define QUD_GCC_BASE 0x00100000ULL
#define QUD_GCC_BYTES 0x100000u

/* DWC3 — ssusb@a800000, core window 0xcd00; wrapper+a2phy 0xf8f00 */
#define QUD_DWC3_BASE 0x0a800000ULL
#define QUD_DWC3_BYTES 0x100000u
/* USB BAM @0xa904000 intentionally unused: polled direct-DWC3 mode. */

/* NO SMMU on this path. Private-domain layer -> contiguous-PA arena.
 * QUD_SMMU_* kept at 0 as sentinel: platform code must skip SMMU bring-up. */
#define QUD_SMMU_BASE 0x0ULL
#define QUD_SMMU_BYTES 0x0u
#define QUD_STREAM_ID 0x0u
#define QUD_DMA_IOVA_IS_PA 1

/* Branch-clock enable regs (clk_branch .enable_reg), from gcc-msm8998.c:
 * master 0xf008, sleep 0xf00c, mock_utmi 0xf010,
 * cfg_noc_usb3_axi 0x5018, aggre1_usb3_axi 0x82024, phy_cfg_ahb2phy 0x6a004 */
#define QUD_GCC_CBCR_OFFSETS {0xf008u, 0xf00cu, 0xf010u, 0x5018u, 0x82024u, 0x6a004u}
#define QUD_GCC_BCR_OFFSET 0xf000u            /* USB_30_BCR */
#define QUD_GCC_GDSCR_OFFSET 0x10f004u        /* gdsc_usb30 */
#define QUD_GCC_CFG_GDSCR_OFFSET 0x0u         /* none on 8998 */

/* PHYs: QUSB2 HS phy @0xc012000 (+TCSR clamp 0x1fcb24c, ref_clk 0x188014);
 * init seq in msm8998.dtsi qusb_phy0. QMP SS @0xc010000 unused (HS only). */
#define QUD_QUSB2_BASE 0x0c012000ULL
#define QUD_TCSR_CLAMP_DIG_N 0x01fcb24cULL
#define QUD_TCSR_REF_CLK 0x00188014ULL
/* VBUS: driven by PMI8998 extcon, NOT Kalama QSCRATCH bits — qualify must
 * instead check DSTS link state / PHY regs. Define accordingly: */
#define QUD_QSCRATCH_HS_PHY_CTRL_OFFSET 0x0u
#define QUD_VBUS_VIA_PMIC_EXTCON 1

/* PSCI SMC reset not yet qualified on this board via this path -> 0. */
#define QUD_PSCI_SMC_RESET 0
#endif
