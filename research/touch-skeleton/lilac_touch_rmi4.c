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

/* ---------------- hardware constants (lilac DTS, verified) ------------- */

#define QUP5_BASE        0x00C179000UL   /* BLSP1 QUP5 */
#define TLMM_BASE        0x010100000UL   /* tlmm pinctrl */
#define TP_SDA_GPIO      87
#define TP_SCL_GPIO      88
#define TP_IRQ_GPIO      125
#define TP_I2C_ADDR      0x2C
#define TP_I2C_HZ        355000

/* QUP i2c engine regs (i2c-msm-v2 register map, verify against sony tree) */
#define QUP_CONFIG          0x000
#define QUP_STATE           0x004
#define QUP_IO_MODES        0x008
#define QUP_SW_RESET        0x00C
#define QUP_TIME_OUT        0x010
#define QUP_TIME_OUT_CURRENT 0x014
#define QUP_OPERATIONAL     0x018
#define QUP_ERROR_FLAGS     0x01C
#define QUP_ERROR_FLAGS_EN  0x020
#define QUP_MX_OUTPUT_CNT   0x024
#define QUP_MX_OUTPUT_CNT_CUR 0x028
#define QUP_MX_INPUT_CNT    0x02C
#define QUP_MX_INPUT_CNT_CUR  0x030
#define QUP_MX_READ_CNT     0x034
#define QUP_MX_WRITE_CNT    0x038
#define QUP_READ_CNT        0x03C
#define QUP_WRITE_CNT       0x040
#define QUP_INPUT_DEBUG     0x044
#define QUP_OUT_FIFO        0x110
#define QUP_IN_FIFO         0x218  /* verify: input FIFO base per i2c-msm-v2 */
#define I2C_STATUS          0x300
#define I2C_MASTER_STATUS   0x300  /* master err/busy fields */
#define I2C_BUS_ERROR       0x304
#define I2C_ADDR            0x308
#define I2C_CONFIG          0x30C  /* BUS_FREE/AUDIO... verify */
#define I2C_MASTER_CTL      0x310
#define I2C_SDA_EARLY_TS    0x314
#define I2C_MASTER_TS       0x318
#define I2C_RESET_CAUSE     0x31C
#define I2C_RESET_TSGEN     0x320
#define I2C_RESET_CORE      0x324
#define I2C_DEBUG_CTL       0x328

/* TODO(verify): above offsets follow i2c-msm-v2.h; BLSP1 QUP5 also needs
 * GCC gate clk (gcc_blsp1_qup5_i2c_apps_*), pinctrl func select on 87/88,
 * and possibly an RPM voltage rail for TPIC (check sony dtsi for
 * vdd/vio regulators on this node — clearpad node has none listed,
 * rails likely always-on via PM8998). */

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

/* ---------------- minimal i2c engine (polled) -------------------------- */

/* QUP tag-v2 constants — real values from sony
 * include/linux/i2c/i2c-msm-v2.h (VERIFIED, not guessed) */
#define QT2_START             0x81
#define QT2_DATA_WRITE        0x82
#define QT2_DATA_WRITE_N_STOP 0x83
#define QT2_DATA_READ         0x85
#define QT2_DATA_READ_N_NACK  0x86
#define QT2_DATA_READ_N_STOP  0x87
#define QT2_INPUT_EOT         0x93
#define QT2_FLUSH_STOP        0x96
/* tag packing (i2c_msm_tag_create):
 *   with START:    {0x81, slave_addr, op, len}  -> 4 bytes
 *   without START: {op, len}                    -> 2 bytes
 * FIFO is byte-serial: write tag bytes then data bytes, in order. */

/* i2c-msm-v2 status bits */
#define QUP_I2C_STATUS_RESET 0xfffffc
#define QUP_I2C_NACK_FLAG    (1u << 3)
#define QUP_I2C_BUS_ACTIVE   (1u << 10)   /* VERIFY bit pos in I2C_STATUS */
#define QUP_I2C_FLUSH        (1u << 6)    /* MASTER_CONFIG flush? verify  */
#define QUP_RUN              0x1
#define QUP_RESET            0x3          /* RESET_STATE */
#define QUP_STATE_VALID      (1u << 1)

static volatile uint32_t *qup = (volatile uint32_t *)QUP5_BASE;
static volatile uint32_t *tlmm = (volatile uint32_t *)TLMM_BASE;

static inline void wr32(uint32_t off, uint32_t v) { qup[off >> 2] = v; }
static inline uint32_t rd32(uint32_t off) { return qup[off >> 2]; }

/* tlmm gpio cfg: cfg[gpio] = {func,dir,pull,drive} packed — verify offset */
#define TLMM_GPIO_CFG(g)  (0x1000 + (g) * 0x10)
#define TLMM_GPIO_INOUT(g) (0x1004 + (g) * 0x10)

static void qup_init(void)
{
    /* 1. pinmux: GPIO87/88 -> blsp_i2c func (verify alt-func number in
     *    sony tlmm dtsi; msm8998 i2c = func 2 typically for blsp1 qup5) */
    tlmm[(TLMM_GPIO_CFG(TP_SDA_GPIO)) >> 2] = (2u << 0) | (0 << 4) | (3 << 6) | (0x8 << 10); /* func,OE,pull,drv — VERIFY */
    tlmm[(TLMM_GPIO_CFG(TP_SCL_GPIO)) >> 2] = (2u << 0) | (0 << 4) | (3 << 6) | (0x8 << 10);

    /* 2. IRQ gpio 125: input + pull-up (or rely on TPIC drive) */
    tlmm[(TLMM_GPIO_CFG(TP_IRQ_GPIO)) >> 2] = (0 << 0) | (0 << 4) | (3 << 6); /* func0,in,pull-up */

    /* 3. QUP core: mini-core to i2c mode, reset, set state RUN */
    wr32(QUP_SW_RESET, 1);
    /* poll RESET_CAUSE/state per i2c-msm-v2 reset sequence */
    wr32(QUP_CONFIG, 0);
    wr32(QUP_IO_MODES, (0 << 0));           /* fifo mode for both — VERIFY encodings */
    wr32(I2C_MASTER_CTL, (TP_I2C_HZ / 65536)); /* verify divider formula */
    wr32(QUP_STATE, QUP_RUN);
}

/* FIFO-mode byte pump: pack stream (tag+data...) then read engine runs it */
static void fifo_push(const uint8_t *p, int n)
{
    while (n--) {
        uint32_t w = *p++;
        /* i2c-msm-v2 packs 4 bytes/fifo word little-endian; here we emit
         * 1 byte/word for clarity (legal but 4x slower — pack later) */
        wr32(QUP_OUT_FIFO, w);
    }
}
static void fifo_pull(uint8_t *p, int n)
{
    while (n--)
        *p++ = rd32(QUP_IN_FIFO) & 0xff;
}

static int qup_wait_idle(void)
{
    for (int i = 0; i < 100000; i++)
        if (!(rd32(I2C_STATUS) & QUP_I2C_BUS_ACTIVE)) return 0;
    return -1;
}

/* i2c write: START tag4{0x81,addr,DATA_WRITE_N_STOP,len} + data */
static int qup_i2c_write(uint8_t addr, const uint8_t *buf, int len)
{
    uint8_t tag[4] = {QT2_START, addr, QT2_DATA_WRITE_N_STOP, (uint8_t)len};
    wr32(QUP_MX_OUTPUT_CNT, 4 + len);   /* block-mode total out bytes */
    fifo_push(tag, 4);
    fifo_push(buf, len);
    if (qup_wait_idle()) return -1;
    return (rd32(I2C_STATUS) & QUP_I2C_NACK_FLAG) ? -2 : 0;
}

/* i2c read:  write reg index via WRITE tag, then read via
 *  DATA_READ_N_STOP (repeated-start handled by engine keeping addr).
 *  VERIFY: sony path may issue second START tag for the read segment —
 *  check i2c_msm_xfer rx msg is_new_addr behavior on device. */
static int qup_i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, int len)
{
    if (qup_i2c_write(addr, &reg, 1)) return -1;
    uint8_t tag[4] = {QT2_START, addr, QT2_DATA_READ_N_STOP, (uint8_t)len};
    wr32(QUP_MX_INPUT_CNT, len);
    wr32(QUP_MX_READ_CNT, len);
    fifo_push(tag, 4);   /* tag goes to OUT fifo; engine fills IN fifo */
    fifo_pull(buf, len);
    if (qup_wait_idle()) return -1;
    return 0;
}

static int tp_read(uint8_t reg, uint8_t *buf, int len)
{
    return qup_i2c_read(TP_I2C_ADDR, reg, buf, len);
}

static int tp_write(uint8_t reg, const uint8_t *buf, int len)
{
    uint8_t tmp[32];
    if (len + 1 > (int)sizeof(tmp)) return -3;
    tmp[0] = reg;
    for (int i = 0; i < len; i++) tmp[1 + i] = buf[i];
    return qup_i2c_write(TP_I2C_ADDR, tmp, len + 1);
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
