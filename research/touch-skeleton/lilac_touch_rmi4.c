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
    wr32(QUP_STATE, 0x1);                    /* RUN_STATE=RUN */
}

/* polled i2c write-then-read (single register address) */
static int tp_read(uint8_t reg, uint8_t *buf, int len)
{
    /* OUT FIFO: tag(QPACKET addr W) + reg ; IN: tag(QPACKET addr R)+data */
    /* i2c-msm-v2 QPACKET: {flags[31:24], addr[23:17], tag[15:8], data[7:0]} — VERIFY packing */
    wr32(QUP_OUT_FIFO, 0);      /* placeholder: real impl packs QPACKET */
    (void)reg; (void)buf; (void)len;
    return -1; /* TODO: implement QPACKET fifo writer per i2c-msm-v2.c */
}

static int tp_write(uint8_t reg, const uint8_t *buf, int len)
{
    (void)reg; (void)buf; (void)len;
    return -1; /* TODO */
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

/* F$11 data read — finger count + coords, then map 2159x3839 -> 720x1280 */
struct tp_finger { uint16_t x, y; uint8_t z, wx, wy, present; };

static int rmi_read_fingers(const struct rmi_pdt_entry *f11,
                            struct tp_finger *out, int max)
{
    /* F11 data regs: data0 = n_fingers+state?, per-finger 5-byte records:
     *   [0] state, [1] x_lsb, [2] x_msb|(y_msb<<4 pack varies),
     *   actual F11 layout on this TPIC = read sony clearpad parser.
     *   FOR BRING-UP: dump raw F11 data regs and reverse the packing
     *   from Android logcat of a known touch. */
    (void)f11; (void)out; (void)max;
    return -1;
}

/* ---------------- entry: shim-side probe ------------------------------- */

/* expected first-light probe (run inside shim after UFS probe):
 *   qup_init();
 *   dev_status = tp_rdb(0, SYN_DEVICE_STATUS);  // expect configured
 *   n = rmi_scan_pdt(pdt, 16);                  // find F$01/F$11/F$34
 *   // F$01 ctrl: clear sleep, enable interrupt reporting
 *   // poll GPIO125 low -> rmi_read_fingers()
 */
