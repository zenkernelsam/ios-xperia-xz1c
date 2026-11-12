#ifndef SURFACE_PROBE_UTILS_H
#define SURFACE_PROBE_UTILS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Only the single-plane 32-bit layout used by this diagnostic. */
static bool sp_layout(size_t w, size_t h, size_t pitch, size_t bpe, size_t allocated)
{
    return w && h && w <= 16384 && h <= 16384 && bpe == 4 &&
        w <= SIZE_MAX / bpe && pitch >= w * bpe &&
        h <= SIZE_MAX / pitch && allocated >= h * pitch;
}
static void sp_pattern(uint8_t out[4], unsigned x, unsigned y)
{
    out[0] = (uint8_t)(x * 17u + y * 3u);
    out[1] = (uint8_t)(x * 5u + y * 11u);
    out[2] = (uint8_t)(x ^ y);
    out[3] = 255;
}
static uint64_t sp_hash_byte(uint64_t hash, uint8_t value)
{
    return (hash ^ value) * UINT64_C(1099511628211);
}
static unsigned sp_check_pattern(const uint8_t *pixels, unsigned w, unsigned h, size_t pitch,
                                bool bgra, uint64_t *hash)
{
    unsigned mismatches = 0;
    *hash = UINT64_C(14695981039346656037);
    for (unsigned y = 0; y < h; ++y) for (unsigned x = 0; x < w; ++x) {
        uint8_t expected[4]; sp_pattern(expected, x, y);
        const uint8_t *p = pixels + (size_t)y * pitch + (size_t)x * 4;
        unsigned red = bgra ? 2 : 0, blue = bgra ? 0 : 2;
        if (p[red] != expected[0] || p[1] != expected[1] ||
            p[blue] != expected[2] || p[3] != expected[3]) ++mismatches;
        for (unsigned i = 0; i < 4; ++i) *hash = sp_hash_byte(*hash, p[i]);
    }
    return mismatches;
}
static unsigned sp_check_bands(const uint8_t *pixels, unsigned w, unsigned h, size_t pitch,
                              uint64_t *hash)
{
    unsigned mismatches = 0;
    *hash = UINT64_C(14695981039346656037);
    for (unsigned y = 0; y < h; ++y) for (unsigned x = 0; x < w; ++x) {
        unsigned right = x >= w / 2, upper = y >= h / 2;
        uint8_t expected[4] = {upper ? 255 : 0, right ? 255 : 0,
                              (!upper || right) ? 255 : 0, 255};
        if (!upper && right) expected[2] = 0; /* BGRA: red, green, blue, white */
        const uint8_t *p = pixels + (size_t)y * pitch + (size_t)x * 4;
        bool different = false;
        for (unsigned i = 0; i < 4; ++i) {
            if (p[i] != expected[i]) different = true;
            *hash = sp_hash_byte(*hash, p[i]);
        }
        if (different) ++mismatches;
    }
    return mismatches;
}
#endif
