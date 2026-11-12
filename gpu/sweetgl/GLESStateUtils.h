#ifndef SWEET_GLES_STATE_UTILS_H
#define SWEET_GLES_STATE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

/* OpenGL error flags are coalesced; glGetError may return them in any order. */
#define SGL_PENDING_ERRORS 16u
#define SGL_ERROR_POLLS 16u
#define SGL_COPY_RETRY_NS 100000000ull

struct sgl_error_state {
    uint32_t pending[SGL_PENDING_ERRORS];
    unsigned count;
    unsigned depth;
    uint32_t operation_error;
};
struct sgl_error_result { uint32_t error; bool verified; };
typedef uint32_t (*sgl_error_reader)(void *);

static inline bool
sgl_error_capture(struct sgl_error_state *state, sgl_error_reader read_error, void *opaque, bool operation)
{
    if (!state || !read_error || state->count > SGL_PENDING_ERRORS)
        return false;
    for (unsigned poll = 0; poll < SGL_ERROR_POLLS; poll++) {
        /* Do not consume another native flag when there is no place to retain it. */
        if (state->count == SGL_PENDING_ERRORS)
            return false;
        uint32_t error = read_error(opaque);
        if (!error)
            return true;
        bool duplicate = false;
        for (unsigned i = 0; i < state->count; i++)
            duplicate |= state->pending[i] == error;
        if (!duplicate)
            state->pending[state->count++] = error;
        if (operation && !state->operation_error)
            state->operation_error = error;
    }
    return false;
}

static inline bool
sgl_error_begin(struct sgl_error_state *state, sgl_error_reader read_error, void *opaque)
{
    if (!state || !read_error || state->depth >= SGL_ERROR_POLLS)
        return false;
    if (!state->depth) {
        if (!sgl_error_capture(state, read_error, opaque, false))
            return false;
        state->operation_error = 0;
    }
    state->depth++;
    return true;
}

static inline struct sgl_error_result
sgl_error_current(struct sgl_error_state *state, sgl_error_reader read_error, void *opaque)
{
    struct sgl_error_result result = {0, false};
    if (!state || !state->depth || !read_error)
        return result;
    result.verified = sgl_error_capture(state, read_error, opaque, true);
    result.error = state->operation_error;
    return result;
}

static inline struct sgl_error_result
sgl_error_end(struct sgl_error_state *state, sgl_error_reader read_error, void *opaque)
{
    struct sgl_error_result result = sgl_error_current(state, read_error, opaque);
    if (state && state->depth)
        state->depth--;
    return result;
}

static inline uint32_t
sgl_error_pop(struct sgl_error_state *state)
{
    if (!state || !state->count || state->count > SGL_PENDING_ERRORS)
        return 0;
    uint32_t error = state->pending[0];
    for (unsigned i = 1; i < state->count; i++)
        state->pending[i - 1] = state->pending[i];
    state->count--;
    return error;
}

struct sgl_copy_state {
    uint32_t seed;
    uint32_t failed_seed;
    uint64_t retry_after_ns;
    bool uploaded;
    bool retry_pending;
};

/* Bytes consumed by one pixel of the requested OpenGL transfer format/type. */
static inline size_t
sgl_copy_gl_element(uint32_t format, uint32_t type)
{
    size_t components;
    switch (format) {
    case 0x1902: /* DEPTH_COMPONENT */
    case 0x1903: /* RED */
    case 0x1906: /* ALPHA */
    case 0x1909: /* LUMINANCE */
    case 0x8d94: /* RED_INTEGER */
        components = 1; break;
    case 0x8227: /* RG */
    case 0x8228: /* RG_INTEGER */
    case 0x190a: /* LUMINANCE_ALPHA */
        components = 2; break;
    case 0x1907: /* RGB */
    case 0x8d98: /* RGB_INTEGER */
        components = 3; break;
    case 0x1908: /* RGBA */
    case 0x80e1: /* BGRA */
    case 0x8d99: /* RGBA_INTEGER */
        components = 4; break;
    case 0x84f9: /* DEPTH_STENCIL, packed types only */
        return type == 0x84fa ? 4 : type == 0x8dad ? 8 : 0;
    default:
        return 0;
    }
    switch (type) {
    case 0x1400: case 0x1401: return components;      /* BYTE / UNSIGNED_BYTE */
    case 0x1402: case 0x1403:                         /* SHORT / UNSIGNED_SHORT */
    case 0x140b: case 0x8d61: return components * 2;  /* HALF_FLOAT / HALF_FLOAT_OES */
    case 0x1404: case 0x1405: case 0x1406: return components * 4; /* INT / UINT / FLOAT */
    case 0x8363: case 0x8364: case 0x8033: case 0x8034:
    case 0x8365: case 0x8366: return 2;              /* packed 16-bit pixels */
    case 0x8035: case 0x8036: case 0x8367: case 0x8368:
    case 0x8c3b: case 0x8c3e: return 4;              /* packed 32-bit pixels */
    default: return 0;
    }
}

static inline bool
sgl_copy_needs_refresh(const struct sgl_copy_state *state, uint32_t seed, uint64_t now_ns)
{
    if (!state)
        return false;
    if (!state->retry_pending && state->uploaded && state->seed == seed)
        return false;
    if (state->retry_pending && state->failed_seed == seed && now_ns &&
        state->retry_after_ns && now_ns < state->retry_after_ns)
        return false;
    return true;
}

static inline void
sgl_copy_finished(struct sgl_copy_state *state, uint32_t seed, bool success, uint64_t now_ns)
{
    if (success) {
        state->seed = seed;
        state->uploaded = true;
        state->retry_pending = false;
        state->retry_after_ns = 0;
    } else {
        /* The failed writer seed must never become the last successful upload. */
        state->retry_pending = true;
        state->failed_seed = seed;
        state->retry_after_ns = !now_ns ? 0 :
            now_ns > UINT64_MAX - SGL_COPY_RETRY_NS ? UINT64_MAX : now_ns + SGL_COPY_RETRY_NS;
    }
}

static inline bool
sgl_copy_layout(size_t width, size_t height, size_t pitch, size_t element,
    size_t allocation, size_t *row_bytes, size_t *span_bytes)
{
    if (!row_bytes || !span_bytes || !width || !height || !element || !pitch ||
        width > INT_MAX || height > INT_MAX || width > SIZE_MAX / element ||
        pitch % element || pitch / element > INT_MAX)
        return false;
    size_t row = width * element;
    if (row > pitch || height - 1 > (SIZE_MAX - row) / pitch)
        return false;
    size_t span = (height - 1) * pitch + row;
    if (allocation && span > allocation)
        return false;
    *row_bytes = row;
    *span_bytes = span;
    return true;
}

#endif
