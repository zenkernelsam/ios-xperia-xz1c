/* Bound timing of the original snapshot APIs; arguments and results are kept. */
#pragma once
#import <Foundation/Foundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <ColorSync/ColorSyncTransform.h>
#include <TargetConditionals.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
typedef UIImage *SweetPostDrawImage;
#else
// The host fixture supplies this iOS-only API, never a device implementation.
extern NSData *UIImagePNGRepresentation(id image);
typedef id SweetPostDrawImage;
#endif

typedef void (*SweetPostDrawEmit)(const char *, ...);
typedef struct {
    uint64_t call, root, thread, wall, cpu;
    unsigned depth;
    bool recorded, cpu_valid;
} SweetPostDrawCall;
static SweetPostDrawEmit postdraw_emit;
static mach_timebase_info_data_t postdraw_timebase;
static atomic_uint_fast64_t postdraw_calls;
static _Thread_local unsigned postdraw_depth;
static _Thread_local uint64_t postdraw_root;
static pid_t postdraw_pid;
enum { SWEET_POSTDRAW_LIMIT = 64 };

static uint64_t postdraw_wall_ns(void)
{
    uint64_t ticks = mach_absolute_time();
    return (uint64_t)(((__uint128_t)ticks * postdraw_timebase.numer) / postdraw_timebase.denom);
}
static uint64_t postdraw_cpu_ns(bool *valid)
{
    struct timespec value = {0};
    *valid = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &value) == 0;
    return *valid ? (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec : 0;
}
static SweetPostDrawCall postdraw_begin(const char *api)
{
    SweetPostDrawCall call = {0};
    call.call = atomic_fetch_add_explicit(&postdraw_calls, 1, memory_order_relaxed) + 1;
    call.depth = ++postdraw_depth;
    if (call.depth == 1) postdraw_root = call.call;
    call.root = postdraw_root;
    call.recorded = call.call <= SWEET_POSTDRAW_LIMIT;
    if (call.recorded) {
        (void)pthread_threadid_np(NULL, &call.thread);
        uint64_t begin = postdraw_wall_ns();
        postdraw_emit("SWEET POSTDRAW BEGIN pid=%d tid=%llu call=%llu root=%llu depth=%u api=%s at_ns=%llu original_forwarded=1",
            postdraw_pid, (unsigned long long)call.thread, (unsigned long long)call.call,
            (unsigned long long)call.root, call.depth, api, (unsigned long long)begin);
    }
    return call;
}
static void postdraw_start(SweetPostDrawCall *call)
{
    // Exclude this observer's entry/metadata records. Nested durations remain
    // inclusive and must never be summed as independent work.
    if (call->recorded) {
        call->cpu = postdraw_cpu_ns(&call->cpu_valid);
        call->wall = postdraw_wall_ns();
    }
}
static void postdraw_end(SweetPostDrawCall call, const char *api, const char *outcome,
                         uint64_t bytes, bool returned_nil)
{
    if (call.recorded) {
        uint64_t wall = postdraw_wall_ns();
        bool cpu_valid = false;
        uint64_t cpu = postdraw_cpu_ns(&cpu_valid);
        cpu_valid = call.cpu_valid && cpu_valid && cpu >= call.cpu;
        postdraw_emit("SWEET POSTDRAW END pid=%d tid=%llu call=%llu api=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u outcome=%s bytes=%llu returned_nil=%u original_behavior=1",
            postdraw_pid, (unsigned long long)call.thread, (unsigned long long)call.call, api,
            (unsigned long long)(wall >= call.wall ? wall - call.wall : 0),
            (unsigned long long)(cpu_valid ? cpu - call.cpu : 0), cpu_valid, outcome,
            (unsigned long long)bytes, returned_nil);
    }
    --postdraw_depth;
    if (!postdraw_depth) postdraw_root = 0;
}
static NSData *postdraw_png(SweetPostDrawImage image)
{
    if (!postdraw_emit) return UIImagePNGRepresentation(image);
    int incoming_errno = errno;
    SweetPostDrawCall call = postdraw_begin("png");
    NSData *result = nil;
    bool returned = false;
    @try {
        postdraw_start(&call);
        errno = incoming_errno;
        result = UIImagePNGRepresentation(image);
        returned = true;
        return result;
    } @finally {
        int outgoing_errno = errno;
        // NSData length is metadata only. No PNG data or pixel pages are read.
        uint64_t bytes = returned && result ? (uint64_t)result.length : 0;
        postdraw_end(call, "png", returned ? "return" : "throw", bytes, result == nil);
        errno = outgoing_errno;
    }
}
static void postdraw_add_image(CGImageDestinationRef destination, CGImageRef image, CFDictionaryRef properties)
{
    if (!postdraw_emit) { CGImageDestinationAddImage(destination, image, properties); return; }
    int incoming_errno = errno;
    SweetPostDrawCall call = postdraw_begin("add_image");
    if (call.recorded) {
        CGColorSpaceRef color = image ? CGImageGetColorSpace(image) : NULL;
        postdraw_emit("SWEET POSTDRAW IMAGE pid=%d call=%llu width=%zu height=%zu bpp=%zu pitch=%zu color_model=%d properties_nil=%u pixels_read=0",
            postdraw_pid, (unsigned long long)call.call, image ? CGImageGetWidth(image) : 0,
            image ? CGImageGetHeight(image) : 0, image ? CGImageGetBitsPerPixel(image) : 0,
            image ? CGImageGetBytesPerRow(image) : 0, color ? (int)CGColorSpaceGetModel(color) : -1,
            properties == NULL);
    }
    bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; CGImageDestinationAddImage(destination, image, properties); returned = true; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "add_image", returned ? "return" : "throw", 0, false);
        errno = outgoing_errno;
    }
}
static bool postdraw_finalize(CGImageDestinationRef destination)
{
    if (!postdraw_emit) return CGImageDestinationFinalize(destination);
    int incoming_errno = errno;
    SweetPostDrawCall call = postdraw_begin("finalize");
    bool result = false, returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; result = CGImageDestinationFinalize(destination); returned = true; return result; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "finalize", returned ? (result ? "true" : "false") : "throw", 0, false);
        errno = outgoing_errno;
    }
}
static ColorSyncTransformRef postdraw_colorsync(CFArrayRef sequence, CFDictionaryRef options)
{
    if (!postdraw_emit) return ColorSyncTransformCreate(sequence, options);
    int incoming_errno = errno;
    SweetPostDrawCall call = postdraw_begin("colorsync");
    ColorSyncTransformRef result = NULL;
    bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; result = ColorSyncTransformCreate(sequence, options); returned = true; return result; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "colorsync", returned ? "return" : "throw", 0, result == NULL);
        errno = outgoing_errno;
    }
}

_Static_assert(__builtin_types_compatible_p(__typeof__(&postdraw_png), __typeof__(&UIImagePNGRepresentation)), "PNG observer ABI differs");
_Static_assert(__builtin_types_compatible_p(__typeof__(&postdraw_add_image), __typeof__(&CGImageDestinationAddImage)), "ImageIO add observer ABI differs");
_Static_assert(__builtin_types_compatible_p(__typeof__(&postdraw_finalize), __typeof__(&CGImageDestinationFinalize)), "ImageIO finalize observer ABI differs");
_Static_assert(__builtin_types_compatible_p(__typeof__(&postdraw_colorsync), __typeof__(&ColorSyncTransformCreate)), "ColorSync observer ABI differs");

static void postdraw_original(SweetPostDrawEmit emit, const char *api, const void *address)
{
    Dl_info info = {0};
    bool valid = dladdr(address, &info) && info.dli_fname && info.dli_fbase;
    const char *name = valid ? strrchr(info.dli_fname, '/') : NULL;
    emit("SWEET POSTDRAW ORIGINAL pid=%d api=%s image=%.96s offset=0x%llx resolved=%u",
        postdraw_pid, api, valid ? (name ? name+1 : info.dli_fname) : "missing",
        (unsigned long long)(valid ? (uintptr_t)address-(uintptr_t)info.dli_fbase : 0), valid);
}

#if !defined(LILAC_POSTDRAW_TEST)
#define SWEET_POSTDRAW_INTERPOSE(replacement, replacee) \
    __attribute__((used, section("__DATA,__interpose"))) \
    static const struct { const void *to; const void *from; } postdraw_pair_##replacee = \
        {(const void *)(uintptr_t)&replacement, (const void *)(uintptr_t)&replacee}
SWEET_POSTDRAW_INTERPOSE(postdraw_png, UIImagePNGRepresentation);
SWEET_POSTDRAW_INTERPOSE(postdraw_add_image, CGImageDestinationAddImage);
SWEET_POSTDRAW_INTERPOSE(postdraw_finalize, CGImageDestinationFinalize);
SWEET_POSTDRAW_INTERPOSE(postdraw_colorsync, ColorSyncTransformCreate);
#undef SWEET_POSTDRAW_INTERPOSE
#endif

static bool postdraw_observe(SweetPostDrawEmit emit)
{
    int saved_errno = errno;
    bool valid = emit && mach_timebase_info(&postdraw_timebase) == KERN_SUCCESS &&
                 postdraw_timebase.numer && postdraw_timebase.denom;
    postdraw_pid = getpid();
    if (valid) postdraw_emit = emit;
    if (emit) {
        emit("SWEET POSTDRAW observer installed=%u interpose_pairs=4 limit=64 metadata_only=1 original_arguments_results=1", valid);
        if (valid) {
            postdraw_original(emit, "png", (const void *)(uintptr_t)&UIImagePNGRepresentation);
            postdraw_original(emit, "add_image", (const void *)(uintptr_t)&CGImageDestinationAddImage);
            postdraw_original(emit, "finalize", (const void *)(uintptr_t)&CGImageDestinationFinalize);
            postdraw_original(emit, "colorsync", (const void *)(uintptr_t)&ColorSyncTransformCreate);
        }
    }
    errno = saved_errno;
    return valid;
}
