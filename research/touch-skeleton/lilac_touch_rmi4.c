/*
 * lilac_touch_rmi4.c — Sony Xperia XZ1 Compact (lilac) touch skeleton
 *
 * TARGET: Synaptics ClearPad in-cell (RMI4 protocol family)
 *   - i2c@c179000 = BLSP1 QUP5 (QUPv3 SE), slave addr 0x2c, 355 kHz
 *   - SDA/SCL = TLMM GPIO 87/88 (pinctrl msm_gpio_87/88)
 *   - IRQ   = TLMM GPIO 125, falling edge
 *   - 10 fingers, coord domain 2159x3839 -> panel 720x1280
 *   - source of truth: sony-kernel-4.4 clearpad_incell_core.c
 *
 * STATUS: WIP SKELETON — registers from Sony driver (trusted, runs on
 * this hardware). NOT yet hardware-verified under shim/XNU. Do NOT copy
 * UniXNU-sheng RMI assumptions (different TPIC generation) — read PDT
 * on-device first.
 *
 * Bring-up plan: MSM8998 BLSP QUP supports i2c FIFO/Block mode -> we can
 * POLL the QUP without GPI-DMA (sheng needed GPI because SM8550 QUP has
 * no FIFO path). Steps: QUP i2c probe -> RMI4 page-select/PDT scan ->
 * F$01 device ctrl -> F$11 finger data -> (kernel) HID digitizer report.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---------------- hardware constants (lilac DTS, verified) ------------- */

#define QUP5_BASE        0x00C179000UL   /* BLSP1 QUP5 */
#define TLMM_BASE        0x010100000UL   /* tlmm pinctrl */
#define TP_SDA_GPIO      87
#define TP_SCL_GPIO      88
#define TP_IRQ_GPIO      125
#define TP_I2C_ADDR      0x2C
#define TP_I2C_HZ        355000

/* NOTE: BLSP1 QUP5 also needs GCC gate clk ungate (in qup_init) and pinctrl
 * func select on 87/88. TPIC rails likely always-on via PM8998 (clearpad
 * node has none listed in dtsi). Register offsets below are the sagit-
 * verified i2c-qup.c layout (same QUP v2.2.1 silicon on MSM8998). */

/* ---------------- RMI4 register constants (sony clearpad_incell_core) --- */

#define SYN_DEVICE_STATUS      0x13   /* unconfigured/flash-fail bits */
#define SYN_PAGE_SELECT_OFFSET 0xFF   /* write page index here */
#define SYN_SUPPORTED_PAGE_NUM 0x04
#define SYN_PDT_START          0xEF   /* PDT scan goes DOWN from here */

/* PDT entry (6 bytes, scan page0 regs 0xEF,0xE9,0xE3,... downto ~0x11):
 *   [0] query_base   [1] command_base  [2] control_base
 *   [3] data_base    [4] intr_src(3b)+version(5b)  [5] function_num
 * function_num==0 -> end of table */
struct rmi_pdt_entry {
    uint8_t query_base, command_base, control_base;
    uint8_t data_base, intr_version, function;
};

/* key functions we care about */
#define FN01_DEVCTRL   0x01   /* power/reset/interrupt enable */
#define FN11_2D_DATA   0x11   /* fingers: F11 data regs (abs x,y,z,wx,wy) */
#define FN34_FLASH     0x34   /* firmware reflash (skip for bring-up) */
#define FN51_CUSTOM    0x51   /* clearpad gesture/aux */
#define FN54_ANALOG    0x54   /* analog test */

/* F$11 finger-state byte codes */
#define F11_STATE_NONE  0x00
#define F11_STATE_ACC   0x01  /* accurate touch */
#define F11_STATE_INACC 0x02

/* ---------------- QUP engine (sagit-verified, 2026-10-25) -----------------
 * Register values/state-machine copied from ios-sweet-sagit commit
 * 17ea09f + 070de55 — SAME QUP5 @ 0xc179000 on MSM8998, polled v2 tags.
 * sagit verified on Mi 6 hardware; our register map was independently
 * derived from sony i2c-msm-v2.h and matches. */

/* GCC clock gates (sagit: same MSM8998 addresses) */
#define SGT_GCC_PA                0x00100000u
#define GCC_BLSP1_AHB_CBCR        0x17004u
#define GCC_BLSP1_QUP5_APPS_CBCR  0x21008u
#define GCC_BLSP1_QUP5_APPS_RCGR  0x21020u
#define GCC_CBCR_CLK_OFF          (1u << 31)

/* QUP register offsets — sagit/proven values. NOTE these differ from the
 * older draft guesses: MX_OUTPUT_CNT is 0x100 not 0x024, FIFO bases and
 * the CLK_CTL/STATUS regs are at the i2c-qup.c (older driver) offsets
 * which sagit verified working on the same silicon. */
#define QUP_CONFIG          0x000u
#define QUP_STATE           0x004u
#define QUP_IO_MODES        0x008u
#define QUP_SW_RESET        0x00cu
#define QUP_OPERATIONAL     0x018u
#define QUP_ERROR_FLAGS     0x01cu
#define QUP_ERROR_FLAGS_EN  0x020u
#define QUP_OPERATIONAL_MASK 0x028u
#define QUP_HW_VERSION      0x030u
#define QUP_MX_OUTPUT_CNT   0x100u
#define QUP_OUT_FIFO_CNT    0x10cu
#define QUP_OUT_FIFO        0x110u
#define QUP_MX_WRITE_CNT    0x150u
#define QUP_MX_INPUT_CNT    0x200u
#define QUP_MX_READ_CNT     0x208u
#define QUP_IN_FIFO_CNT     0x214u
#define QUP_IN_FIFO         0x218u
#define QUP_I2C_CLK_CTL     0x400u
#define QUP_I2C_STATUS      0x404u
#define QUP_I2C_MASTER_CFG  0x408u
#define QUP_I2C_BUS_CLR     0x40cu

/* QUP_STATE fields */
#define QUP_STATE_RESET     0u
#define QUP_STATE_RUN       1u
#define QUP_STATE_PAUSE     3u
#define QUP_STATE_MASK      3u
#define QUP_STATE_VALID     (1u << 2)
#define QUP_I2C_MAST_GEN    (1u << 4)

/* QUP_CONFIG / IO_MODES */
#define QUP_MINI_CORE_I2C   (2u << 8)
#define QUP_N_VAL_8BIT      7u
#define QUP_NO_INPUT        (1u << 7)
#define QUP_NO_OUTPUT       (1u << 6)
#define QUP_PACK_EN         (1u << 15)
#define QUP_UNPACK_EN       (1u << 14)

/* QUP_OPERATIONAL flags */
#define QUP_IN_FIFO_NOT_EMPTY (1u << 5)
#define QUP_OUT_SVC_FLAG    (1u << 8)
#define QUP_IN_SVC_FLAG     (1u << 9)
#define QUP_MX_OUT_DONE     (1u << 10)
#define QUP_MX_IN_DONE      (1u << 11)

/* I2C_STATUS */
#define QUP_STATUS_NACK     (1u << 3)
#define QUP_STATUS_ERR_MASK 0x0380003cu
#define QUP_ERR_FLAGS_MASK  0x3cu

/* tag-v2 (same as i2c-msm-v2.h) */
#define QUP_TAG_START       0x81u
#define QUP_TAG_DATA_WR     0x82u   /* no stop — used for write phase of wr-then-rd */
#define QUP_TAG_DATA_WR_STOP 0x83u
#define QUP_TAG_DATA_RD     0x85u
#define QUP_TAG_DATA_RD_STOP 0x87u

/* clock ctl (sagit): HS_DIV<<16 | FS_DIV at 0x400 */
#define QUP_CLK_CTL_400K    ((14u << 16) | 28u)
#define QUP_CLK_CTL_100K    ((62u << 16) | 124u)
/* 355 kHz (lilac DTS): interpolate FS_DIV ~31; sagit only tested 400/100 —
 * start with 400k for bring-up, tune later if TPIC misbehaves. */

#define I2C_ADDR8(a)        ((a) << 1)

static volatile uint32_t *qup = (volatile uint32_t *)QUP5_BASE;
static volatile uint32_t *gcc = (volatile uint32_t *)SGT_GCC_PA;
static volatile uint32_t *tlmm = (volatile uint32_t *)TLMM_BASE;

static inline void wr32(uint32_t off, uint32_t v) { qup[off >> 2] = v; }
static inline uint32_t rd32(uint32_t off) { return qup[off >> 2]; }

#define TLMM_GPIO_CFG(g)  (0x1000 + (g) * 0x10)

static int qup_state_set(uint32_t state)
{
    wr32(QUP_STATE, state);
    for (int i = 0; i < 5000; i++) {
        uint32_t v = rd32(QUP_STATE);
        if ((v & QUP_STATE_VALID) && ((v & QUP_STATE_MASK) == state))
            return 0;
    }
    return -1;
}

static void qup_init(void)
{
    /* 1. pinmux GPIO87/88 (func=2 per sony tlmm, VERIFY alt-num) + IRQ 125 */
    tlmm[(TLMM_GPIO_CFG(TP_SDA_GPIO)) >> 2] = (2u << 0) | (0 << 4) | (3 << 6) | (0x8 << 10);
    tlmm[(TLMM_GPIO_CFG(TP_SCL_GPIO)) >> 2] = (2u << 0) | (0 << 4) | (3 << 6) | (0x8 << 10);
    tlmm[(TLMM_GPIO_CFG(TP_IRQ_GPIO)) >> 2] = (0 << 0) | (0 << 4) | (3 << 6);

    /* 2. ungate clocks: clear CLK_OFF on ahb + qup5 apps (sagit) */
    gcc[GCC_BLSP1_AHB_CBCR >> 2]       &= ~GCC_CBCR_CLK_OFF;
    gcc[GCC_BLSP1_QUP5_APPS_CBCR >> 2] &= ~GCC_CBCR_CLK_OFF;

    /* 3. core init (sagit order): reset -> RESET -> configure -> MASTER_CFG=1
     *    (enables version-two tags) -> MAST_GEN check */
    wr32(QUP_SW_RESET, 1);
    qup_state_set(QUP_STATE_RESET);
    wr32(QUP_CONFIG, QUP_N_VAL_8BIT | QUP_MINI_CORE_I2C);
    wr32(QUP_ERROR_FLAGS_EN, QUP_ERR_FLAGS_MASK);
    wr32(QUP_OPERATIONAL_MASK, QUP_IN_SVC_FLAG | QUP_OUT_SVC_FLAG);
    wr32(QUP_I2C_MASTER_CFG, 1);              /* v2 tags */
    /* sagit: verify MAST_GEN bit set after MASTER_CFG=1 */
}

/* one QUP fifo transaction (sagit qup_msg): RESET -> counts -> flags ->
 * RUN -> CLK -> PAUSE -> push -> RUN -> poll MX_*_DONE */
static void fifo_push(const uint8_t *p, unsigned n)
{
    for (unsigned i = 0; i < n; i += 4) {
        uint32_t w = 0;
        for (int j = 0; j < 4 && i + j < n; j++)
            w |= (uint32_t)p[i + j] << (8 * j);
        wr32(QUP_OUT_FIFO, w);
    }
}
static void fifo_pull(uint8_t *p, unsigned n)
{
    unsigned i = 0;
    while (i < n) {
        uint32_t w = rd32(QUP_IN_FIFO);
        for (int j = 0; j < 4 && i < n; j++)
            p[i++] = (w >> (8 * j)) & 0xff;
    }
}

#define I2C_ADDR8_RD(a)     ((uint8_t)(((a) << 1) | 1u))

/* single FIFO stream, both phases (sagit 070de55 fix):
 *   w+r: START+addrW DATA_WR(no stop) wlen data... START+addrR DATA_RD_STOP rlen
 *   w:   START+addrW DATA_WR_STOP wlen data...
 *   r:   START+addrR DATA_RD_STOP rlen */
static int qup_msg(uint8_t addr, const uint8_t *wr, unsigned wlen,
                   uint8_t *rd, unsigned rlen)
{
    uint8_t tx[4 + 4 + 32];
    unsigned ntx = 0, mx_wr, mx_rd;
    if (wlen > 32 || rlen > 64) return -1;

    if (wlen) {
        tx[ntx++] = QUP_TAG_START;
        tx[ntx++] = I2C_ADDR8(addr);
        if (rlen) {
            tx[ntx++] = QUP_TAG_DATA_WR;          /* no stop! */
            tx[ntx++] = (uint8_t)wlen;
            for (unsigned i = 0; i < wlen; i++) tx[ntx++] = wr[i];
            tx[ntx++] = QUP_TAG_START;            /* repeated start */
            tx[ntx++] = I2C_ADDR8_RD(addr);
            tx[ntx++] = QUP_TAG_DATA_RD_STOP;
            tx[ntx++] = (uint8_t)rlen;
        } else {
            tx[ntx++] = QUP_TAG_DATA_WR_STOP;
            tx[ntx++] = (uint8_t)wlen;
            for (unsigned i = 0; i < wlen; i++) tx[ntx++] = wr[i];
        }
    } else if (rlen) {
        tx[ntx++] = QUP_TAG_START;
        tx[ntx++] = I2C_ADDR8_RD(addr);
        tx[ntx++] = QUP_TAG_DATA_RD_STOP;
        tx[ntx++] = (uint8_t)rlen;
    }
    mx_wr = ntx;
    mx_rd = rlen ? rlen + 2 : 0;

    if (qup_state_set(QUP_STATE_RESET)) return -2;
    wr32(QUP_MX_INPUT_CNT, 0);
    wr32(QUP_MX_OUTPUT_CNT, 0);
    wr32(QUP_MX_READ_CNT, mx_rd);
    wr32(QUP_MX_WRITE_CNT, mx_wr);
    wr32(QUP_CONFIG, QUP_N_VAL_8BIT | QUP_MINI_CORE_I2C | (rlen ? 0u : QUP_NO_INPUT));
    wr32(QUP_IO_MODES, QUP_PACK_EN | QUP_UNPACK_EN);
    wr32(QUP_OPERATIONAL_MASK, QUP_IN_SVC_FLAG | QUP_OUT_SVC_FLAG);
    wr32(QUP_OPERATIONAL, 0xffffffffu);
    wr32(QUP_I2C_STATUS, QUP_STATUS_ERR_MASK);
    wr32(QUP_ERROR_FLAGS, QUP_ERR_FLAGS_MASK);

    if (qup_state_set(QUP_STATE_RUN)) return -3;
    wr32(QUP_I2C_CLK_CTL, QUP_CLK_CTL_400K);
    if (qup_state_set(QUP_STATE_PAUSE)) return -4;
    fifo_push(tx, ntx);
    if (qup_state_set(QUP_STATE_RUN)) return -5;

    for (unsigned i = 0; i < 40000; i++) {
        uint32_t op = rd32(QUP_OPERATIONAL);
        uint32_t status = rd32(QUP_I2C_STATUS);
        uint32_t errs = rd32(QUP_ERROR_FLAGS);
        if (status & QUP_STATUS_ERR_MASK)
            return (status & QUP_STATUS_NACK) ? -6 : -7;
        if (errs & QUP_ERR_FLAGS_MASK) return -8;
        if (rlen) {
            if (op & QUP_MX_IN_DONE) break;
        } else if (op & (QUP_OUT_SVC_FLAG | QUP_MX_OUT_DONE)) break;
        if (i == 39999) return -9;
    }
    if (rlen) {
        /* first 2 IN-fifo bytes are tag echoes; data follows */
        uint8_t dump[2];
        fifo_pull(dump, 2);
        fifo_pull(rd, rlen);
    }
    qup_state_set(QUP_STATE_RESET);
    return 0;
}

/* RMI4 write-then-read (sagit 070de55): write reg w/ DATA_WR (no stop),
 * then repeated-START + DATA_RD — implemented as two qup_msg calls:
 * msg1 = write(addr, reg,1) via DATA_WR (no STOP tag? sagit issues the
 * write phase inside ONE stream; simplest verified form = two msgs).
 * NOTE: sagit emits both phases in one FIFO stream for atomicity; for
 * bring-up the two-msg form below is equivalent if bus stays claimed —
 * if TPIC needs true repeated-start, merge streams per 070de55. */
static int tp_read(uint8_t reg, uint8_t *buf, int len)
{
    return qup_msg(TP_I2C_ADDR, &reg, 1, buf, (unsigned)len);
}

static int tp_write(uint8_t reg, const uint8_t *buf, int len)
{
    uint8_t tmp[33];
    if (len + 1 > (int)sizeof(tmp)) return -3;
    tmp[0] = reg;
    for (int i = 0; i < len; i++) tmp[1 + i] = buf[i];
    return qup_msg(TP_I2C_ADDR, tmp, (unsigned)(len + 1), NULL, 0);
}

/* ---------------- RMI4 layer ------------------------------------------- */

static uint8_t tp_rdb(uint8_t page, uint8_t reg)
{
    uint8_t v = 0;
    tp_write(SYN_PAGE_SELECT_OFFSET, &page, 1);
    tp_read(reg, &v, 1);
    return v;
}

static int rmi_scan_pdt(struct rmi_pdt_entry *out, int max)
{
    int n = 0;
    for (uint8_t a = SYN_PDT_START; a >= 0x10 && n < max; a -= 6) {
        struct rmi_pdt_entry e;
        tp_read(a, (uint8_t *)&e, 6);
        if (e.function == 0)
            break;
        out[n++] = e;
        /* note: same function can appear on multiple pages — track page */
    }
    return n;
}

/* F$11 data read — real layout from sony rmi4/rmi_f11.c (VERIFIED):
 *   data regs at f11->data_base, layout per interrupt packet:
 *     [0 .. n_state-1]   finger-state regs: 2 bits/finger, packed 4/byte
 *     then n_fingers * 5 abs bytes: per finger
 *        b0=x[11:4], b1=y[11:4], b2={y[3:0],x[3:0]}, b3={wy,wx}, b4=z
 *     then n_fingers * 2 rel bytes (skipped; we poll abs only)
 *   state codes: 0=none 1=present 2=inaccurate 3=reserved
 *   coord domain 2159x3839 -> panel 720x1280 (scale /3, /3) */
#define RMI_F11_ABS_BYTES 5
#define RMI_F11_REL_BYTES 2
#define FINGER_STATE_MASK 0x03
#define TP_DOMAIN_X 2159
#define TP_DOMAIN_Y 3839
#define PANEL_X     720
#define PANEL_Y     1280

struct tp_finger { uint16_t x, y; uint8_t z, wx, wy, present; };

static int rmi_read_fingers(const struct rmi_pdt_entry *f11,
                            struct tp_finger *out, int max,
                            uint8_t n_fingers)
{
    int n_state = (n_fingers + 3) / 4;
    int abs_len = n_fingers * RMI_F11_ABS_BYTES;
    uint8_t pkt[4 /*state*/ + 10 * 5];
    int total = n_state + abs_len;
    if (n_fingers > 10 || total > (int)sizeof(pkt)) return -1;

    /* F11 data_base at page0 in most clearpads — PDT row records page.
     * Caller must pass the entry with its page; here we assume page0 or
     * that the caller already selected the right page (TODO: carry page
     * in pdt entry). */
    if (tp_read(f11->data_base, pkt, total)) return -2;

    int n = 0;
    for (uint8_t i = 0; i < n_fingers && n < max; i++) {
        uint8_t st = (pkt[i / 4] >> (2 * (i % 4))) & FINGER_STATE_MASK;
        if (st == F11_STATE_NONE || st == 3) continue;
        const uint8_t *p = &pkt[n_state + i * RMI_F11_ABS_BYTES];
        struct tp_finger *f = &out[n++];
        f->x = ((p[0] << 4) | (p[2] & 0x0F)) * PANEL_X / TP_DOMAIN_X;
        f->y = ((p[1] << 4) | (p[2] >> 4)) * PANEL_Y / TP_DOMAIN_Y;
        f->wx = p[3] & 0x0F;
        f->wy = p[3] >> 4;
        f->z  = p[4];
        f->present = (st == F11_STATE_ACC);
    }
    return n;
}

/* ---------------- entry: shim-side probe ------------------------------- */

/* expected first-light probe (run inside shim after UFS probe):
 *   qup_init();
 *   dev_status = tp_rdb(0, SYN_DEVICE_STATUS);  // expect configured
 *   n = rmi_scan_pdt(pdt, 16);                  // find F$01/F$11/F$34
 *   // F$01 ctrl: clear sleep, enable interrupt reporting
 *   // poll GPIO125 low -> rmi_read_fingers(f11e, fingers, 10, 10)
 *   // -> {x,y,z,wx,wy,present} in PANEL coords, ready for HID digits
 *
 * Remaining VERIFY items before hardware run:
 *   - I2C_STATUS bit positions (BUS_ACTIVE/NACK) vs i2c-msm-v2.h
 *   - QUP_IO_MODES encoding for FIFO-vs-BLOCK mode
 *   - I2C_MASTER_CTL clock divider formula for 355kHz
 *   - whether read needs 2nd START tag (is_new_addr on rx msg)
 *   - GCC gate clock enable for blsp1_qup5 (gcc-msm8998.c)
 */
