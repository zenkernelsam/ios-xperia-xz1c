#ifndef SWEET_SURFACE_TEXTURE_LAYOUT_H
#define SWEET_SURFACE_TEXTURE_LAYOUT_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/* FD540 actual offset comparison: +32 samples from +0; +64/+128 are exact.
 * Incompatible source layouts use the existing seed-aware upload path. */
static inline bool sweet_surface_texture_needs_copy(uintptr_t base, size_t pitch)
{
    return (base & 63u) != 0 || (pitch & 63u) != 0;
}
#endif
