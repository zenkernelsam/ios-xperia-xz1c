/* Lossless, bounded scanout snapshot helpers. No allocations, MMIO or I/O. */
#ifndef SWEET_FRAMEBUFFER_SCREEN_H
#define SWEET_FRAMEBUFFER_SCREEN_H
#ifdef SWEET_SCREEN_HOST_TEST
#include <stddef.h>
#include <stdint.h>
#else
#include <sys/types.h>
#endif

#define SWEET_SCREEN_WIDTH 720u
#define SWEET_SCREEN_HEIGHT 1280u
#define SWEET_SCREEN_STRIDE (SWEET_SCREEN_WIDTH * 3u)
#define SWEET_SCREEN_BYTES (SWEET_SCREEN_STRIDE * SWEET_SCREEN_HEIGHT)
#define SWEET_SCREEN_CHUNK 128u
#define SWEET_SCREEN_LIMIT 4u

struct sweet_screen_regs {
    uint32_t source, stride, size, image, xy, output, output_xy;
    uint32_t format, unpack, op, qseed, ctl, start;
};

/* The current lilac contract has one unscaled VIG0 RGB888 pipe. A different
 * pipe, crop, scaler or format must get its own independently checked reader. */
static inline int sweet_screen_contract(const struct sweet_screen_regs *r,
    uint64_t physical, uint64_t mapping_bytes)
{
    const uint32_t size = (SWEET_SCREEN_HEIGHT << 16) | SWEET_SCREEN_WIDTH;
    return r && physical && physical < (1ull << 32) &&
        SWEET_SCREEN_BYTES <= (1ull << 32) - physical &&
        mapping_bytes >= SWEET_SCREEN_BYTES && r->source == physical &&
        r->stride == SWEET_SCREEN_STRIDE && r->size == size && r->image == size &&
        r->xy == 0 && r->output == size && r->output_xy == 0 &&
        r->format == 0x2243fu && r->unpack == 0x20001u &&
        r->op == 0x80000000u && r->qseed == 0 && r->ctl == 0x20020u;
}

static inline int sweet_screen_regs_equal(const struct sweet_screen_regs *a,
    const struct sweet_screen_regs *b)
{
    return a->source == b->source && a->stride == b->stride &&
        a->size == b->size && a->image == b->image && a->xy == b->xy &&
        a->output == b->output && a->output_xy == b->output_xy &&
        a->format == b->format && a->unpack == b->unpack && a->op == b->op &&
        a->qseed == b->qseed && a->ctl == b->ctl && a->start == b->start;
}

static inline uint64_t sweet_screen_fnv(const uint8_t *bytes, size_t count)
{
    uint64_t value = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < count; ++i) value = (value ^ bytes[i]) * 0x100000001b3ull;
    return value;
}

static inline uint32_t sweet_screen_crc_update(uint32_t value,
    const uint8_t *bytes, size_t count)
{
    static const uint32_t table[16] = {
        0x00000000u,0x1db71064u,0x3b6e20c8u,0x26d930acu,
        0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,
        0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,
        0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu
    };
    for (size_t i = 0; i < count; ++i) {
        value ^= bytes[i];
        value = (value >> 4) ^ table[value & 15u];
        value = (value >> 4) ^ table[value & 15u];
    }
    return value;
}

static inline uint32_t sweet_screen_crc(const uint8_t *bytes, size_t count)
{
    return sweet_screen_crc_update(0xffffffffu, bytes, count) ^ 0xffffffffu;
}

typedef int (*sweet_screen_sink)(void *, const uint8_t *, size_t);

static inline int sweet_screen_pixel_equal(const uint8_t *a, const uint8_t *b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

/* PRLE1: tag bit 7 selects a repeated BGR pixel; otherwise literal BGR pixels.
 * Both packet lengths are (tag & 127)+1, in 1..128. Packets may span wire chunks.
 * Worst case adds one byte per 128 pixels. Every original colour byte survives. */
static inline int sweet_screen_encode(const uint8_t *pixels, size_t bytes,
    sweet_screen_sink sink, void *context)
{
    if (!pixels || !sink || !bytes || bytes % 3 || bytes > SWEET_SCREEN_BYTES) return 0;
    const size_t count = bytes / 3;
    size_t i = 0;
    while (i < count) {
        size_t run = 1;
        while (run < 128 && run < count - i &&
            sweet_screen_pixel_equal(pixels + i * 3, pixels + (i + run) * 3)) ++run;
        if (run >= 2) {
            uint8_t tag = (uint8_t)(0x80u | (run - 1));
            if (!sink(context, &tag, 1) || !sink(context, pixels + i * 3, 3)) return 0;
            i += run;
        } else {
            const size_t start = i++;
            while (i < count && i - start < 128) {
                if (i + 1 < count && sweet_screen_pixel_equal(pixels + i * 3, pixels + (i + 1) * 3)) break;
                ++i;
            }
            uint8_t tag = (uint8_t)(i - start - 1);
            if (!sink(context, &tag, 1) || !sink(context, pixels + start * 3, (i - start) * 3)) return 0;
        }
    }
    return 1;
}

#endif
