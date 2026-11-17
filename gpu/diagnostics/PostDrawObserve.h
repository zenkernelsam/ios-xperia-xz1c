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
#include <objc/runtime.h>
#if defined(SWEET_PNG_FILTER_ENCODER)
#include "../compat/PNGImageDispatch.h"
#include "PNGEncoderProof.h"
#endif

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
    bool recorded, cpu_valid, original_forwarded;
} SweetPostDrawCall;
static SweetPostDrawEmit postdraw_emit;
static mach_timebase_info_data_t postdraw_timebase;
static atomic_uint_fast64_t postdraw_calls;
static _Thread_local unsigned postdraw_depth;
static _Thread_local uint64_t postdraw_root;
static pid_t postdraw_pid;
enum { SWEET_POSTDRAW_LIMIT = 64 };

// This protocol describes UIKit's getters. The Mac test supplies a forwarding
// fixture because UIImagePNGRepresentation is an iOS-only API.
@protocol SweetPNGInputGetters
- (CGImageRef)CGImage;
- (CGSize)size;
- (CGFloat)scale;
- (NSInteger)imageOrientation;
@end
typedef struct {
    bool image_nil, getters_available, getter_error, cgimage_nil, color_nil;
    const char *image_class;
    CGSize points;
    CGFloat scale;
    NSInteger orientation;
    size_t width, height, bpc, bpp, pitch, components;
    uint32_t bitmap;
    int alpha, intent, color_model;
    bool wide, extended, named, name_sanitized;
    char color_name[64];
    uint64_t getter_ns;
} SweetPNGInput;

static bool postdraw_input_getter(Class type, SEL selector, const char *encoding)
{
    Method method = type ? class_getInstanceMethod(type, selector) : NULL;
    char actual[96] = {0};
    if (method) method_getReturnType(method, actual, sizeof(actual));
    return method && method_getNumberOfArguments(method) == 2 && !strcmp(actual, encoding);
}

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
static SweetPNGInput postdraw_read_png_input(SweetPostDrawImage image)
{
    int saved_errno = errno;
    uint64_t begin = postdraw_wall_ns();
    SweetPNGInput input = {.image_nil = image == nil, .cgimage_nil = true,
        .color_nil = true, .color_model = -1, .image_class = "nil", .color_name = "none"};
    Class type = image ? object_getClass(image) : Nil;
    if (type) input.image_class = class_getName(type);
    input.getters_available = type &&
        postdraw_input_getter(type, @selector(CGImage), @encode(CGImageRef)) &&
        postdraw_input_getter(type, @selector(size), @encode(CGSize)) &&
        postdraw_input_getter(type, @selector(scale), @encode(CGFloat)) &&
        postdraw_input_getter(type, @selector(imageOrientation), @encode(NSInteger));
    CFStringRef name = NULL;
    @try {
        if (input.getters_available) {
            id<SweetPNGInputGetters> original = (id<SweetPNGInputGetters>)image;
            CGImageRef cg = [original CGImage];
            input.points = [original size];
            input.scale = [original scale];
            input.orientation = [original imageOrientation];
            input.cgimage_nil = cg == NULL;
            if (cg) {
                input.width = CGImageGetWidth(cg); input.height = CGImageGetHeight(cg);
                input.bpc = CGImageGetBitsPerComponent(cg); input.bpp = CGImageGetBitsPerPixel(cg);
                input.pitch = CGImageGetBytesPerRow(cg); input.bitmap = CGImageGetBitmapInfo(cg);
                input.alpha = CGImageGetAlphaInfo(cg); input.intent = CGImageGetRenderingIntent(cg);
                CGColorSpaceRef color = CGImageGetColorSpace(cg);
                input.color_nil = color == NULL;
                if (color) {
                    input.color_model = CGColorSpaceGetModel(color);
                    input.components = CGColorSpaceGetNumberOfComponents(color);
                    input.wide = CGColorSpaceIsWideGamutRGB(color);
                    input.extended = CGColorSpaceUsesExtendedRange(color);
                    name = CGColorSpaceCopyName(color);
                    input.named = name && CFStringGetCString(name, input.color_name, sizeof(input.color_name), kCFStringEncodingUTF8);
                    if (!input.named) strcpy(input.color_name, "unknown");
                    for (char *at = input.color_name; *at; ++at) {
                        bool plain = (*at >= 'a' && *at <= 'z') || (*at >= 'A' && *at <= 'Z') ||
                            (*at >= '0' && *at <= '9') || *at == '_' || *at == '-' || *at == '.';
                        if (!plain) { *at = '_'; input.name_sanitized = true; }
                    }
                }
            }
        }
    } @catch (NSException *exception) {
        // A diagnostic getter must not replace the original API's exception.
        // Partial metadata remains labelled as an error; the PNG call follows.
        (void)exception; input.getter_error = true;
    }
    if (name) CFRelease(name);
    uint64_t finish = postdraw_wall_ns();
    input.getter_ns = finish >= begin ? finish - begin : 0;
    errno = saved_errno;
    return input;
}
static void postdraw_png_input(SweetPostDrawCall call, SweetPostDrawImage image)
{
    if (!call.recorded) return;
    SweetPNGInput input = postdraw_read_png_input(image);
    postdraw_emit("SWEET POSTDRAW PNG_INPUT pid=%d call=%llu image_nil=%u getters=%u error=%u class=%.48s getters_only=1 pixels_read=0",
        postdraw_pid, (unsigned long long)call.call, input.image_nil, input.getters_available, input.getter_error, input.image_class);
    postdraw_emit("SWEET POSTDRAW PNG_UI pid=%d call=%llu width_pt=%.9g height_pt=%.9g scale=%.9g orientation=%lld getter_ns=%llu",
        postdraw_pid, (unsigned long long)call.call, (double)input.points.width, (double)input.points.height,
        (double)input.scale, (long long)input.orientation, (unsigned long long)input.getter_ns);
    postdraw_emit("SWEET POSTDRAW PNG_CG pid=%d call=%llu cg_nil=%u width=%zu height=%zu bpc=%zu bpp=%zu pitch=%zu bitmap=0x%x alpha=%d intent=%d",
        postdraw_pid, (unsigned long long)call.call, input.cgimage_nil, input.width, input.height, input.bpc, input.bpp, input.pitch,
        input.bitmap, input.alpha, input.intent);
    postdraw_emit("SWEET POSTDRAW PNG_COLOR pid=%d call=%llu color_nil=%u model=%d components=%zu wide=%u extended=%u named=%u sanitized=%u name=%.63s",
        postdraw_pid, (unsigned long long)call.call, input.color_nil, input.color_model, input.components,
        input.wide, input.extended, input.named, input.name_sanitized, input.color_name);
}
static SweetPostDrawCall postdraw_begin_route(const char *api,bool original_forwarded)
{
    SweetPostDrawCall call = {0};
    call.original_forwarded=original_forwarded;
    call.call = atomic_fetch_add_explicit(&postdraw_calls, 1, memory_order_relaxed) + 1;
    call.depth = ++postdraw_depth;
    if (call.depth == 1) postdraw_root = call.call;
    call.root = postdraw_root;
    call.recorded = call.call <= SWEET_POSTDRAW_LIMIT;
    if (call.recorded) {
        (void)pthread_threadid_np(NULL, &call.thread);
        uint64_t begin = postdraw_wall_ns();
        postdraw_emit("SWEET POSTDRAW BEGIN pid=%d tid=%llu call=%llu root=%llu depth=%u api=%s at_ns=%llu original_forwarded=%u",
            postdraw_pid, (unsigned long long)call.thread, (unsigned long long)call.call,
            (unsigned long long)call.root, call.depth, api, (unsigned long long)begin,call.original_forwarded);
    }
    return call;
}
static SweetPostDrawCall postdraw_begin(const char *api){return postdraw_begin_route(api,true);}
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
        postdraw_emit("SWEET POSTDRAW END pid=%d tid=%llu call=%llu api=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u outcome=%s bytes=%llu returned_nil=%u original_behavior=%u",
            postdraw_pid, (unsigned long long)call.thread, (unsigned long long)call.call, api,
            (unsigned long long)(wall >= call.wall ? wall - call.wall : 0),
            (unsigned long long)(cpu_valid ? cpu - call.cpu : 0), cpu_valid, outcome,
            (unsigned long long)bytes, returned_nil,call.original_forwarded);
    }
    --postdraw_depth;
    if (!postdraw_depth) postdraw_root = 0;
}
#if defined(SWEET_PNG_FILTER_ENCODER)
static NSData *postdraw_original_png(id image){return UIImagePNGRepresentation((SweetPostDrawImage)image);}
#endif
static NSData *postdraw_png(SweetPostDrawImage image)
{
    if (!postdraw_emit) return UIImagePNGRepresentation(image);
    int incoming_errno = errno;
    bool eligible=false;
#if defined(SWEET_PNG_FILTER_ENCODER)
    CGImageRef cg=NULL;CGFloat scale=0;int route=SWEET_PNG_ORIGINAL;
    eligible=sweet_png_image_eligible(image,objc_lookUpClass("UIImage"),&cg,&scale);
#endif
    SweetPostDrawCall call = postdraw_begin_route("png",!eligible);
    NSData *result = nil;
    bool returned = false;
    @try {
        postdraw_png_input(call, image);
        postdraw_start(&call);
        errno = incoming_errno;
#if defined(SWEET_PNG_FILTER_ENCODER)
        result=sweet_png_dispatch(image,postdraw_original_png,sweet_png_imageio_encode,eligible,cg,scale,&route);
#else
        result = UIImagePNGRepresentation(image);
#endif
        returned = true;
        return result;
    } @finally {
        int outgoing_errno = errno;
#if defined(SWEET_PNG_FILTER_ENCODER)
        call.original_forwarded=route!=SWEET_PNG_UP;
        if(call.recorded)postdraw_emit("SWEET POSTDRAW PNG_ROUTE pid=%d call=%llu route=%s qualified=%u original_forwarded=%u filter=32 source_sha=%s",
            postdraw_pid,(unsigned long long)call.call,route==SWEET_PNG_UP?"ImageIO_UP":route==SWEET_PNG_ORIGINAL?"UIImage_original":route==SWEET_PNG_FAILED_FALLBACK?"UIImage_fallback":"UIImage_throw_fallback",
            eligible,call.original_forwarded,SWEET_PNG_IMAGEIO_SOURCE_SHA);
#endif
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
#if defined(SWEET_PNG_FILTER_ENCODER)
        emit("SWEET POSTDRAW observer installed=%u interpose_pairs=4 limit=64 metadata_only=0 original_arguments_results=0", valid);
#else
        emit("SWEET POSTDRAW observer installed=%u interpose_pairs=4 limit=64 metadata_only=1 original_arguments_results=1", valid);
#endif
        if (valid) {
            postdraw_original(emit, "png", (const void *)(uintptr_t)&UIImagePNGRepresentation);
            postdraw_original(emit, "add_image", (const void *)(uintptr_t)&CGImageDestinationAddImage);
            postdraw_original(emit, "finalize", (const void *)(uintptr_t)&CGImageDestinationFinalize);
            postdraw_original(emit, "colorsync", (const void *)(uintptr_t)&ColorSyncTransformCreate);
            emit("SWEET POSTDRAW PNG_CONFIG pid=%d revision=1 getters_only=1 pixels_read=0 profile_read=0", postdraw_pid);
#if defined(SWEET_PNG_FILTER_ENCODER)
            emit("SWEET PNG ENCODER installed pid=%d backend=ImageIO_UP filter=32 source_sha=%s proof_token=%s fallbacks_original=1",postdraw_pid,SWEET_PNG_IMAGEIO_SOURCE_SHA,SWEET_PNG_REFERENCE_TOKEN);
#endif
        }
    }
    errno = saved_errno;
    return valid;
}
