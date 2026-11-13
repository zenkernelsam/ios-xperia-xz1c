/* Bounds of the actual existing SweetScatter window and16KiB backing pages. */
#pragma once
#include "MetalBackingAPI.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
static inline uint64_t sweet_metal_backing_span(uint64_t bytes)
{
    return bytes && bytes<=(UINT64_C(256)<<20)?(bytes+16383)&~UINT64_C(16383):0;
}
static inline bool sweet_metal_backing_valid(const SweetMetalBacking *b)
{
    uint64_t span=b?sweet_metal_backing_span(b->logical_bytes):0;
    return span && b->allocated_bytes==span && b->handle && b->handle<=4096 && !b->reserved && b->contents &&
        !((uintptr_t)b->contents&16383) && b->gpu_address>=UINT64_C(0x40000000) &&
        b->gpu_address<UINT64_C(0x80000000) && !(b->gpu_address&16383) &&
        span<=UINT64_C(0x80000000)-b->gpu_address;
}
