/* Production codec against an independent decoder, full panel and bounds. */
#define SWEET_SCREEN_HOST_TEST 1
#include "../../xnu/support/SweetFramebufferScreen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bytes { uint8_t *data; size_t count, capacity; };
static int append(void *context, const uint8_t *data, size_t count)
{
    struct bytes *b = context;
    assert(count <= b->capacity - b->count);
    memcpy(b->data + b->count, data, count); b->count += count;
    return 1;
}

static void decode(const struct bytes *b, uint8_t *target, size_t bytes)
{
    size_t at = 0, out = 0;
    while (at < b->count) {
        uint8_t tag = b->data[at++]; size_t count = (tag & 127u) + 1;
        assert(count * 3 <= bytes - out);
        if (tag & 128u) {
            assert(b->count - at >= 3);
            for (size_t i = 0; i < count; ++i) memcpy(target + out + i * 3, b->data + at, 3);
            at += 3;
        } else {
            assert(b->count - at >= count * 3);
            memcpy(target + out, b->data + at, count * 3); at += count * 3;
        }
        out += count * 3;
    }
    assert(out == bytes);
}

static int reject(void *context, const uint8_t *data, size_t bytes)
{ (void)context; (void)data; (void)bytes; return 0; }

int main(int argc, char **argv)
{
    assert(argc == 2);
    assert(sweet_screen_crc((const uint8_t *)"123456789", 9) == 0xcbf43926u);
    assert(sweet_screen_fnv((const uint8_t *)"a", 1) == 0xaf63dc4c8601ec8cull);
    const size_t maximum = SWEET_SCREEN_BYTES + (SWEET_SCREEN_WIDTH * SWEET_SCREEN_HEIGHT + 127) / 128;
    uint8_t *source = malloc(SWEET_SCREEN_BYTES), *decoded = malloc(SWEET_SCREEN_BYTES);
    uint8_t *encoded = malloc(maximum + 32);
    assert(source && decoded && encoded);
    struct bytes b = { encoded + 16, 0, maximum };
    memset(encoded, 0xa5, maximum + 32);
    uint32_t random = 0x5e13beefu;
    for (unsigned mode = 0; mode < 8; ++mode) {
        for (size_t pixel = 0; pixel < SWEET_SCREEN_WIDTH * SWEET_SCREEN_HEIGHT; ++pixel) {
            for (unsigned c = 0; c < 3; ++c) {
                random = random * 1664525u + 1013904223u;
                source[pixel * 3 + c] = mode == 0 ? (uint8_t)(c * 113u) :
                    mode == 1 ? (uint8_t)((pixel / 127u + c) & 255u) :
                    mode == 2 ? (uint8_t)((pixel / 128u + c) & 255u) :
                    mode == 3 ? (uint8_t)((pixel / 129u + c) & 255u) :
                    mode == 4 ? (uint8_t)((pixel % 2u) * 239u + c) :
                    mode == 5 ? (uint8_t)((pixel / 2u + c) & 255u) :
                    mode == 6 ? (uint8_t)((pixel / SWEET_SCREEN_WIDTH + c) & 255u) : (uint8_t)(random >> 24);
            }
        }
        b.count = 0; assert(sweet_screen_encode(source, SWEET_SCREEN_BYTES, append, &b));
        decode(&b, decoded, SWEET_SCREEN_BYTES); assert(!memcmp(source, decoded, SWEET_SCREEN_BYTES));
        for (size_t i = 0; i < 16; ++i) assert(encoded[i] == 0xa5 && encoded[maximum + 16 + i] == 0xa5);
        if (mode == 7) {
            char path[4096];
            int n = snprintf(path, sizeof(path), "%s/full.raw", argv[1]); assert(n > 0 && (size_t)n < sizeof(path));
            FILE *file = fopen(path, "wb"); assert(file && fwrite(source, 1, SWEET_SCREEN_BYTES, file) == SWEET_SCREEN_BYTES); fclose(file);
            n = snprintf(path, sizeof(path), "%s/full.prle", argv[1]); assert(n > 0 && (size_t)n < sizeof(path));
            file = fopen(path, "wb"); assert(file && fwrite(b.data, 1, b.count, file) == b.count); fclose(file);
            n = snprintf(path, sizeof(path), "%s/full.checks", argv[1]); assert(n > 0 && (size_t)n < sizeof(path));
            file = fopen(path, "wb"); assert(file);
            fprintf(file, "%016llx %08x %zu\n", (unsigned long long)sweet_screen_fnv(source, SWEET_SCREEN_BYTES),
                sweet_screen_crc(b.data, b.count), b.count); fclose(file);
        }
    }
    for (size_t count = 1; count <= 300; ++count) {
        b.count = 0; assert(sweet_screen_encode(source, count * 3, append, &b));
        decode(&b, decoded, count * 3); assert(!memcmp(source, decoded, count * 3));
    }
    assert(!sweet_screen_encode(source, 0, append, &b));
    assert(!sweet_screen_encode(source, 4, append, &b));
    assert(!sweet_screen_encode(source, SWEET_SCREEN_BYTES + 3, append, &b));
    assert(!sweet_screen_encode(NULL, 3, append, &b));
    assert(!sweet_screen_encode(source, 3, NULL, &b));
    assert(!sweet_screen_encode(source, 3, reject, NULL));
    struct sweet_screen_regs r = {0x9d400000u, 2160u, 0x50002d0u, 0x50002d0u, 0, 0x50002d0u, 0,
        0x2243fu, 0x20001u, 0x80000000u, 0, 0x20020u, 0};
    assert(sweet_screen_contract(&r, r.source, SWEET_SCREEN_BYTES));
    assert(!sweet_screen_contract(&r, r.source + 1, SWEET_SCREEN_BYTES));
    assert(!sweet_screen_contract(&r, r.source, SWEET_SCREEN_BYTES - 1));
    for (size_t i = 0; i < sizeof(r) / sizeof(uint32_t) - 1; ++i) {
        struct sweet_screen_regs altered = r; ((uint32_t *)&altered)[i] ^= 1;
        assert(!sweet_screen_regs_equal(&r, &altered));
        assert(!sweet_screen_contract(&altered, r.source, SWEET_SCREEN_BYTES));
    }
    free(source); free(decoded); free(encoded);
    puts("PASS screen codec: 8 full panels, 300 boundaries, guards, source contract, CRC/FNV, sink refusal");
    return 0;
}
