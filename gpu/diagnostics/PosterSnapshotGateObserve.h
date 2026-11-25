/* SPDX-License-Identifier: BSD-2-Clause
 * Observe existing poster views and cutout replies; create no scene or draw. */
#pragma once
#import <Foundation/Foundation.h>
#include <CoreGraphics/CGGeometry.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(SWEET_LOADED_SNAPSHOT_PNG_EXPORT)
#include "LoadedSnapshotPNGExport.h"
#endif

typedef void (*SweetPosterGateEmit)(const char *, ...);
typedef void (^SweetPosterGateReply)(CGRect);
enum { PSG_NORMAL, PSG_MAXIMAL, PSG_BACKGROUND, PSG_FLOATING, PSG_VISTA, PSG_IMAGE_LOAD, PSG_IMAGE_SET, PSG_COUNT };
static const char *const poster_gate_apis[] = {"cutout", "max_cutout", "background", "floating", "vista", "image_load", "image_set"};
static SweetPosterGateEmit poster_gate_emit;
static IMP poster_gate_original[PSG_COUNT];
static atomic_uint poster_gate_calls[PSG_COUNT], poster_gate_sequence, poster_gate_replies;
enum { PSG_PER_API = 8, PSG_MAX_REPLIES = 64 };

static unsigned poster_gate_begin(unsigned api, id receiver, id argument)
{
    unsigned sample = atomic_fetch_add_explicit(&poster_gate_calls[api], 1, memory_order_relaxed) + 1;
    if (sample > PSG_PER_API) return 0;
    unsigned call = atomic_fetch_add_explicit(&poster_gate_sequence, 1, memory_order_relaxed) + 1;
    poster_gate_emit("SWEET PSG BEGIN pid=%d call=%u api=%s receiver=%016llx argument=%016llx original_forwarded=1",
        getpid(), call, poster_gate_apis[api], (unsigned long long)(uintptr_t)receiver,
        (unsigned long long)(uintptr_t)argument);
    return call;
}
static void poster_gate_rect(unsigned call, unsigned reply, CGRect rectangle)
{
    uint64_t bits[4];
    memcpy(bits, &rectangle, sizeof(bits));
    poster_gate_emit("SWEET PSG RECT0 pid=%d call=%u reply=%u x=%016llx y=%016llx",
        getpid(), call, reply, (unsigned long long)bits[0], (unsigned long long)bits[1]);
    poster_gate_emit("SWEET PSG RECT1 pid=%d call=%u reply=%u w=%016llx h=%016llx",
        getpid(), call, reply, (unsigned long long)bits[2], (unsigned long long)bits[3]);
}
static void poster_gate_end(unsigned call, unsigned api, id result, bool returned)
{
    if (call) poster_gate_emit("SWEET PSG END pid=%d call=%u api=%s result=%016llx outcome=%s original_behavior=1",
        getpid(), call, poster_gate_apis[api], (unsigned long long)(uintptr_t)result, returned ? "return" : "throw");
}
static void poster_gate_fetch(id receiver, SEL selector, NSInteger orientation,
                             SweetPosterGateReply completion, unsigned api)
{
    int incoming = errno;
    unsigned call = poster_gate_begin(api, receiver, (id)completion);
    if (call) poster_gate_emit("SWEET PSG REQUEST pid=%d call=%u orientation=%016llx callback_nil=%u",
        getpid(), call, (unsigned long long)(uint64_t)orientation, completion == nil);
    SweetPosterGateReply forwarded = completion;
    if (call && completion) {
        forwarded = [^(CGRect rectangle) {
            int callback_errno = errno;
            unsigned reply = atomic_fetch_add_explicit(&poster_gate_replies, 1, memory_order_relaxed) + 1;
            bool recorded = reply <= PSG_MAX_REPLIES;
            if (recorded) {
                poster_gate_emit("SWEET PSG REPLY pid=%d call=%u reply=%u original_forwarded=1",
                    getpid(), call, reply);
                poster_gate_rect(call, reply, rectangle);
            }
            bool returned = false;
            @try { errno = callback_errno; completion(rectangle); returned = true; }
            @finally {
                int outgoing = errno;
                if (recorded) poster_gate_emit("SWEET PSG REPLY_END pid=%d call=%u reply=%u outcome=%s original_behavior=1",
                    getpid(), call, reply, returned ? "return" : "throw");
                errno = outgoing;
            }
        } copy];
    }
    bool returned = false;
    @try {
        errno = incoming;
        ((void (*)(id, SEL, NSInteger, SweetPosterGateReply))poster_gate_original[api])(
            receiver, selector, orientation, forwarded);
        returned = true;
    } @finally {
        int outgoing = errno;
        if (forwarded != completion) [forwarded release];
        poster_gate_end(call, api, nil, returned);
        errno = outgoing;
    }
}
static void poster_gate_cutout(id receiver, SEL selector, NSInteger orientation, SweetPosterGateReply completion)
{ poster_gate_fetch(receiver, selector, orientation, completion, PSG_NORMAL); }
static void poster_gate_max_cutout(id receiver, SEL selector, NSInteger orientation, SweetPosterGateReply completion)
{ poster_gate_fetch(receiver, selector, orientation, completion, PSG_MAXIMAL); }
static id poster_gate_view(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = poster_gate_begin(api, receiver, nil);
    id answer = nil; bool returned = false;
    @try { errno = incoming; answer = ((id (*)(id, SEL))poster_gate_original[api])(receiver, selector); returned = true; return answer; }
    @finally { int outgoing = errno; poster_gate_end(call, api, answer, returned); errno = outgoing; }
}
static id poster_gate_background(id receiver, SEL selector) { return poster_gate_view(receiver, selector, PSG_BACKGROUND); }
static id poster_gate_floating(id receiver, SEL selector) { return poster_gate_view(receiver, selector, PSG_FLOATING); }
static id poster_gate_vista(id receiver, SEL selector, CGRect frame, id configuration)
{
    int incoming = errno; unsigned call = poster_gate_begin(PSG_VISTA, receiver, configuration);
    if (call) poster_gate_rect(call, 0, frame);
    id answer = nil; bool returned = false;
    @try { errno = incoming; answer = ((id (*)(id, SEL, CGRect, id))poster_gate_original[PSG_VISTA])(receiver, selector, frame, configuration); returned = true; return answer; }
    @finally { int outgoing = errno; poster_gate_end(call, PSG_VISTA, answer, returned); errno = outgoing; }
}
static void poster_gate_path(unsigned call, id path)
{
    if (!call) return;
    int incoming = errno; char bytes[257] = {0}; unsigned count = 0; bool truncated = false, getter_error = false;
    @try {
        if ([path isKindOfClass:[NSString class]]) {
            const char *text = [(NSString *)path UTF8String];
            size_t size = text ? strnlen(text, sizeof(bytes)) : 0;
            truncated = size > 256; count = (unsigned)(size > 256 ? 256 : size);
            if (count) memcpy(bytes, text, count);
        }
    } @catch (id error) { (void)error; getter_error = true; }
    poster_gate_emit("SWEET PSG PATH pid=%d call=%u bytes=%u truncated=%u getter_error=%u pixels_read=0",
        getpid(), call, count, truncated, getter_error);
    for (unsigned offset = 0; offset < count; offset += 24) {
        unsigned size = count - offset < 24 ? count - offset : 24; char hex[49] = {0};
        for (unsigned i = 0; i < size; ++i) snprintf(hex + 2*i, 3, "%02x", (unsigned char)bytes[offset+i]);
        poster_gate_emit("SWEET PSG PATH_DATA pid=%d call=%u offset=%u bytes=%u hex=%s",
            getpid(), call, offset, size, hex);
    }
    errno = incoming;
}
static id poster_gate_image_load(id receiver, SEL selector, id path)
{
    int incoming = errno; unsigned call = poster_gate_begin(PSG_IMAGE_LOAD, receiver, path);
    poster_gate_path(call, path); id answer = nil; bool returned = false;
    @try { errno = incoming; answer = ((id (*)(id, SEL, id))poster_gate_original[PSG_IMAGE_LOAD])(receiver, selector, path); returned = true; return answer; }
    @finally {
        int outgoing = errno; poster_gate_end(call, PSG_IMAGE_LOAD, answer, returned);
#if defined(SWEET_LOADED_SNAPSHOT_PNG_EXPORT)
        if(returned && answer && call)loaded_snapshot_png_export_once(path,answer,(unsigned)getpid(),call,poster_gate_emit);
#endif
        errno = outgoing;
    }
}
static void poster_gate_image_set(id receiver, SEL selector, id image)
{
    int incoming = errno; unsigned call = poster_gate_begin(PSG_IMAGE_SET, receiver, image); bool returned = false;
    @try { errno = incoming; ((void (*)(id, SEL, id))poster_gate_original[PSG_IMAGE_SET])(receiver, selector, image); returned = true; }
    @finally { int outgoing = errno; poster_gate_end(call, PSG_IMAGE_SET, nil, returned); errno = outgoing; }
}
static bool poster_gate_install(Class owner, const char *owner_name, const char *name, unsigned api,
                                IMP replacement, const char *result, const char *const *arguments, unsigned count)
{
    int incoming = errno; SEL selector = sel_registerName(name);
    bool valid_owner = owner && !class_isMetaClass(owner) && !strcmp(class_getName(owner), owner_name);
    Method method = valid_owner ? class_getInstanceMethod(owner, selector) : NULL;
    unsigned total = 0; Method *list = valid_owner ? class_copyMethodList(owner, &total) : NULL;
    bool owned = false;
    for (unsigned i = 0; i < total; ++i) if (method_getName(list[i]) == selector) owned = true;
    free(list);
    char actual[192] = {0}; if (method) method_getReturnType(method, actual, sizeof(actual));
    bool valid = owned && method && method_getNumberOfArguments(method) == count + 2 && !strcmp(actual, result);
    for (unsigned i = 0; valid && i < count; ++i) {
        method_getArgumentType(method, i + 2, actual, sizeof(actual));
        valid = !strcmp(actual, arguments[i]);
    }
    IMP original = valid ? method_getImplementation(method) : NULL;
    valid = valid && original && original != replacement && !poster_gate_original[api];
    if (valid) { poster_gate_original[api] = original; method_setImplementation(method, replacement); }
    poster_gate_emit("SWEET PSG METHOD pid=%d api=%s present=%u owner=%u installed=%u argc=%u exact_ABI=1",
        getpid(), poster_gate_apis[api], method != NULL, valid_owner, valid, method ? method_getNumberOfArguments(method) : 0);
    errno = incoming; return valid;
}
static unsigned poster_snapshot_gate_observe(Class service, Class renderer, Class vista, Class image, Class image_view, SweetPosterGateEmit emit)
{
    if (!emit) return 0;
    int incoming = errno; poster_gate_emit = emit;
    const char *request[] = {@encode(NSInteger), @encode(SweetPosterGateReply)};
    const char *initialize[] = {@encode(CGRect), @encode(id)};
    unsigned installed = 0;
    installed += poster_gate_install(service, "SBSWallpaperService", "fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:",
        PSG_NORMAL, (IMP)poster_gate_cutout, @encode(void), request, 2);
    installed += poster_gate_install(service, "SBSWallpaperService", "fetchMaximalLockScreenContentCutoutBoundsForOrientation:completionHandler:",
        PSG_MAXIMAL, (IMP)poster_gate_max_cutout, @encode(void), request, 2);
    installed += poster_gate_install(renderer, "PRRenderer", "backgroundView", PSG_BACKGROUND,
        (IMP)poster_gate_background, @encode(id), NULL, 0);
    installed += poster_gate_install(renderer, "PRRenderer", "floatingView", PSG_FLOATING,
        (IMP)poster_gate_floating, @encode(id), NULL, 0);
    installed += poster_gate_install(vista, "NUNIAstronomyVistaView", "initWithFrame:configuration:", PSG_VISTA,
        (IMP)poster_gate_vista, @encode(id), initialize, 2);
    const char *object[] = {@encode(id)};
    installed += poster_gate_install(image, "UIImage", "initWithContentsOfFile:", PSG_IMAGE_LOAD,
        (IMP)poster_gate_image_load, @encode(id), object, 1);
    installed += poster_gate_install(image_view, "UIImageView", "setImage:", PSG_IMAGE_SET,
        (IMP)poster_gate_image_set, @encode(void), object, 1);
    poster_gate_emit("SWEET PSG CONFIG pid=%d revision=2 installed=%u expected=7 per_api=8 max_calls=56 max_replies=64 scene_getters=0 path_metadata=1 GPU_work=0",
        getpid(), installed);
    errno = incoming; return installed;
}
_Static_assert(sizeof(CGRect) == 32 && sizeof(NSInteger) == 8, "Original poster geometry/orientation ABI differs");
