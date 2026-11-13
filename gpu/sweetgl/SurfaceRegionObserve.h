#ifndef SWEET_SURFACE_REGION_OBSERVE_H
#define SWEET_SURFACE_REGION_OBSERVE_H
#include <stdint.h>
struct sro_span { uint64_t source, raw, start, end; };
static inline int sro_plan(uint64_t source, uint64_t bytes, struct sro_span *out)
{
    if (!out) return 0;
    *out = (struct sro_span){0, 0, 0, 0};
    if (!source || !bytes || bytes > (UINT64_C(256) << 20) ||
        bytes > UINT64_MAX - source || source + bytes > UINT64_MAX - 16383) return 0;
    *out = (struct sro_span){source, bytes, source & ~UINT64_C(16383),
        (source + bytes + 16383) & ~UINT64_C(16383)};
    return out->end > out->start;
}
static inline int sro_advance(const struct sro_span *span, uint64_t cursor,
    uint64_t region, uint64_t bytes, uint64_t *next)
{
    if (!span || !next || cursor < span->start || cursor >= span->end || !bytes ||
        bytes > UINT64_MAX - region || region > cursor || region + bytes <= cursor) return 0;
    *next = region + bytes < span->end ? region + bytes : span->end;
    return 1;
}
#endif
