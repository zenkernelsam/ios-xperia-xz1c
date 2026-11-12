#ifndef SURFACE_INITIAL_COPY_H
#define SURFACE_INITIAL_COPY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SC_MAX_BYTES (16u << 20)
enum sc_status { SC_COPIED = 0, SC_ALIAS = 1, SC_EMPTY = 2, SC_LAYOUT = 3,
    SC_NO_MEMORY = -1, SC_READ_FAILED = -2, SC_LOCK_FAILED = -3,
    SC_WRITE_FAILED = -4, SC_UNLOCK_FAILED = -5 };
struct sc_shape {
    uint64_t source, destination, source_bytes, destination_bytes;
    size_t width, height, pitch, bytes_per_element;
};
struct sc_ops {
    void *context;
    void *(*allocate)(void *, size_t);
    void (*release)(void *, void *, size_t);
    int (*read)(void *, uint64_t, void *, size_t, size_t *);
    int (*write)(void *, uint64_t, const void *, size_t);
    int (*lock)(void *);
    int (*unlock)(void *);
};
struct sc_result {
    int status, read_status, write_status, lock_status, unlock_status;
    unsigned attempted;
    size_t bytes, source_nonzero;
    uint64_t source_hash;
};
static bool sc_new_surface(size_t planes, size_t element_width, size_t element_height, uint32_t seed)
{
    return planes == 0 && element_width == 1 && element_height == 1 && seed == 0;
}
static bool sc_extent(const struct sc_shape *s, size_t *bytes)
{
    *bytes = 0;
    if (!s || !s->source || !s->destination || !s->width || !s->height ||
        s->width > 16384 || s->height > 16384 ||
        !(s->bytes_per_element == 1 || s->bytes_per_element == 2 || s->bytes_per_element == 4 ||
          s->bytes_per_element == 8 || s->bytes_per_element == 16) ||
        s->width > SIZE_MAX / s->bytes_per_element) return false;
    size_t row = s->width * s->bytes_per_element;
    if (s->pitch < row || s->height - 1 > (SIZE_MAX - row) / s->pitch) return false;
    size_t visible = (s->height - 1) * s->pitch + row;
    if (!visible || visible > SC_MAX_BYTES || visible > s->destination_bytes ||
        (s->source_bytes && visible > s->source_bytes) ||
        s->source > UINT64_MAX - visible || s->destination > UINT64_MAX - visible) return false;
    *bytes = visible;
    return true;
}
static struct sc_result sc_snapshot(const struct sc_ops *ops, const struct sc_shape *shape)
{
    struct sc_result r = {.status = SC_LAYOUT};
    if (!ops || !ops->allocate || !ops->release || !ops->read || !ops->write ||
        !ops->lock || !ops->unlock || !sc_extent(shape, &r.bytes)) return r;
    if (shape->source == shape->destination) { r.status = SC_ALIAS; return r; }
    void *temporary = ops->allocate(ops->context, r.bytes);
    if (!temporary) { r.status = SC_NO_MEMORY; return r; }
    size_t read = 0;
    r.attempted |= 1;
    r.read_status = ops->read(ops->context, shape->source, temporary, r.bytes, &read);
    if (r.read_status || read != r.bytes) { r.status = SC_READ_FAILED; goto done; }
    r.source_hash = UINT64_C(14695981039346656037);
    const uint8_t *p = temporary;
    for (size_t i = 0; i < r.bytes; ++i) {
        if (p[i]) ++r.source_nonzero;
        r.source_hash = (r.source_hash ^ p[i]) * UINT64_C(1099511628211);
    }
    if (!r.source_nonzero) { r.status = SC_EMPTY; goto done; }
    r.attempted |= 2;
    r.lock_status = ops->lock(ops->context);
    if (r.lock_status) { r.status = SC_LOCK_FAILED; goto done; }
    r.attempted |= 4;
    r.write_status = ops->write(ops->context, shape->destination, temporary, r.bytes);
    r.attempted |= 8;
    r.unlock_status = ops->unlock(ops->context);
    r.status = r.write_status ? SC_WRITE_FAILED : r.unlock_status ? SC_UNLOCK_FAILED : SC_COPIED;
done:
    ops->release(ops->context, temporary, r.bytes);
    return r;
}
#endif
