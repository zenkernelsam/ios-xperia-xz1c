/* Typed original method timing. No instances, buffers or pixels are created. */
#pragma once
#include "PostDrawObserve.h"
#if defined(SWEET_SNAPSHOT_RENDER_QUEUE_OBSERVE)
#include "SnapshotRenderQueueObserve.h"
#endif
#if defined(SWEET_SNAPSHOT_READBACK_OBSERVE)
#include "SnapshotReadbackObserve.h"
#endif
#import <Metal/Metal.h>
#include <objc/runtime.h>
#include <stdlib.h>

typedef id (*SweetTimedFactory)(id, SEL, id, NSError **);
typedef id (*SweetTimedSnapshot)(id, SEL, CGRect, CGFloat, double);
typedef void (*SweetTimedVoid)(id, SEL);
typedef void (*SweetTimedRead)(id, SEL, void *, NSUInteger, MTLRegion, NSUInteger);
typedef void (*SweetTimedReadFull)(id, SEL, void *, NSUInteger, NSUInteger, MTLRegion, NSUInteger, NSUInteger);
static IMP snapshot_pipeline_original, snapshot_url_original, snapshot_bundle_original;
static IMP snapshot_quad_original, snapshot_wait_original, snapshot_execute_original;
static IMP snapshot_read_original, snapshot_read_full_original;

static SweetPostDrawCall snapshot_method_begin(const char *api, id object)
{
    SweetPostDrawCall call = postdraw_begin(api);
    if (call.recorded) postdraw_emit("SWEET SNAPSHOT OBJECT pid=%d call=%llu api=%s object=0x%llx metadata_only=1",
        postdraw_pid, (unsigned long long)call.call, api, (unsigned long long)(uintptr_t)object);
    return call;
}
static id snapshot_factory(id object, SEL selector, id argument, NSError **error,
                           const char *api, SweetTimedFactory original)
{
    if (!postdraw_emit) return original(object, selector, argument, error);
    int incoming_errno = errno;
    SweetPostDrawCall call = snapshot_method_begin(api, object);
    id result = nil; bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; result = original(object, selector, argument, error); returned = true; return result; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, api, returned ? "return" : "throw", 0, result == nil);
        errno = outgoing_errno;
    }
}
static id snapshot_pipeline(id object, SEL selector, id argument, NSError **error)
{ return snapshot_factory(object, selector, argument, error, "pipeline", (SweetTimedFactory)snapshot_pipeline_original); }
static id snapshot_url(id object, SEL selector, id argument, NSError **error)
{ return snapshot_factory(object, selector, argument, error, "library_url", (SweetTimedFactory)snapshot_url_original); }
static id snapshot_bundle(id object, SEL selector, id argument, NSError **error)
{ return snapshot_factory(object, selector, argument, error, "library_bundle", (SweetTimedFactory)snapshot_bundle_original); }
static id snapshot_quad(id object, SEL selector, CGRect rectangle, CGFloat scale, double time)
{
    if (!postdraw_emit) return ((SweetTimedSnapshot)snapshot_quad_original)(object, selector, rectangle, scale, time);
    int incoming_errno = errno;
    SweetPostDrawCall call = snapshot_method_begin("snapshot", object);
    id result = nil; bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; result = ((SweetTimedSnapshot)snapshot_quad_original)(object, selector, rectangle, scale, time); returned = true; return result; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "snapshot", returned ? "return" : "throw", 0, result == nil);
        errno = outgoing_errno;
    }
}
static void snapshot_void(id object, SEL selector, const char *api, SweetTimedVoid original)
{
    if (!postdraw_emit) { original(object, selector); return; }
    int incoming_errno = errno;
    SweetPostDrawCall call = snapshot_method_begin(api, object);
    bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; original(object, selector); returned = true; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, api, returned ? "return" : "throw", 0, false);
        errno = outgoing_errno;
    }
}
static void snapshot_wait(id object, SEL selector)
{ snapshot_void(object, selector, "wait", (SweetTimedVoid)snapshot_wait_original); }
static void snapshot_execute(id object, SEL selector)
{
    snapshot_void(object, selector, "execute", (SweetTimedVoid)snapshot_execute_original);
#if defined(SWEET_SNAPSHOT_RENDER_QUEUE_OBSERVE)
    snapshot_render_queue_result(object);
#endif
}
static void snapshot_region(SweetPostDrawCall call, MTLRegion region, NSUInteger row, NSUInteger image, NSUInteger level, NSUInteger slice)
{
    if (call.recorded) postdraw_emit("SWEET SNAPSHOT REGION pid=%d call=%llu width=%lu height=%lu depth=%lu row=%lu image=%lu mip=%lu slice=%lu pixels_read=0",
        postdraw_pid, (unsigned long long)call.call, (unsigned long)region.size.width, (unsigned long)region.size.height,
        (unsigned long)region.size.depth, (unsigned long)row, (unsigned long)image, (unsigned long)level, (unsigned long)slice);
}
static void snapshot_read(id object, SEL selector, void *bytes, NSUInteger row, MTLRegion region, NSUInteger level)
{
    if (!postdraw_emit) { ((SweetTimedRead)snapshot_read_original)(object, selector, bytes, row, region, level); return; }
    int incoming_errno = errno;
    SweetPostDrawCall call = snapshot_method_begin("read", object);
    snapshot_region(call, region, row, 0, level, 0);
    bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; ((SweetTimedRead)snapshot_read_original)(object, selector, bytes, row, region, level); returned = true; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "read", returned ? "return" : "throw", 0, false);
#if defined(SWEET_SNAPSHOT_READBACK_OBSERVE)
        if(returned && call.recorded)snapshot_readback_once(object,bytes,row,region,level,0,postdraw_pid,call.call,postdraw_emit);
#endif
        errno = outgoing_errno;
    }
}
static void snapshot_read_full(id object, SEL selector, void *bytes, NSUInteger row, NSUInteger image, MTLRegion region, NSUInteger level, NSUInteger slice)
{
    if (!postdraw_emit) { ((SweetTimedReadFull)snapshot_read_full_original)(object, selector, bytes, row, image, region, level, slice); return; }
    int incoming_errno = errno;
    SweetPostDrawCall call = snapshot_method_begin("read_full", object);
    snapshot_region(call, region, row, image, level, slice);
    bool returned = false;
    @try { postdraw_start(&call); errno = incoming_errno; ((SweetTimedReadFull)snapshot_read_full_original)(object, selector, bytes, row, image, region, level, slice); returned = true; }
    @finally {
        int outgoing_errno = errno;
        postdraw_end(call, "read_full", returned ? "return" : "throw", 0, false);
#if defined(SWEET_SNAPSHOT_READBACK_OBSERVE)
        if(returned && call.recorded)snapshot_readback_once(object,bytes,row,region,level,slice,postdraw_pid,call.call,postdraw_emit);
#endif
        errno = outgoing_errno;
    }
}

static bool snapshot_method_install(Class type, const char *name, const char *api,
                                    IMP observer, IMP *original, const char *result, const char *const *arguments, unsigned count)
{
    SEL selector = sel_registerName(name);
    Method method = type ? class_getInstanceMethod(type, selector) : NULL;
    bool owned = false;
    if (type) {
        unsigned total = 0; Method *methods = class_copyMethodList(type, &total);
        for (unsigned i=0; i<total; ++i) if (method_getName(methods[i]) == selector) owned = true;
        free(methods);
    }
    char actual[192] = {0};
    if (method) method_getReturnType(method, actual, sizeof(actual));
    bool valid = method && owned && method_getNumberOfArguments(method) == count+2 && !strcmp(actual, result);
    for (unsigned i=0; valid && i<count; ++i) {
        method_getArgumentType(method, i+2, actual, sizeof(actual));
        valid = !strcmp(actual, arguments[i]);
    }
    IMP prior = valid ? method_getImplementation(method) : NULL;
    valid = valid && prior && prior != observer;
    if (valid) { *original = prior; method_setImplementation(method, observer); }
    postdraw_emit("SWEET SNAPSHOT METHOD pid=%d api=%s present=%u owned=%u installed=%u arguments=%u original_forwarded=1",
        postdraw_pid, api, method != NULL, owned, valid, method ? method_getNumberOfArguments(method) : 0);
    return valid;
}
static unsigned snapshot_methods_observe(Class device, Class quad, Class commands, Class texture)
{
    if (!postdraw_emit) return 0;
    int saved_errno = errno;
    postdraw_emit("SWEET SNAPSHOT INIT pid=%d at_ns=%llu expected=8 metadata_only=1", postdraw_pid, (unsigned long long)postdraw_wall_ns());
    const char *factory[] = {@encode(id), @encode(NSError **)};
    const char *quad_arguments[] = {@encode(CGRect), @encode(CGFloat), @encode(double)};
    const char *read_arguments[] = {@encode(void *), @encode(NSUInteger), @encode(MTLRegion), @encode(NSUInteger)};
    const char *full_arguments[] = {@encode(void *), @encode(NSUInteger), @encode(NSUInteger), @encode(MTLRegion), @encode(NSUInteger), @encode(NSUInteger)};
    unsigned installed = 0;
    installed += snapshot_method_install(device, "newRenderPipelineStateWithDescriptor:error:", "pipeline", (IMP)snapshot_pipeline, &snapshot_pipeline_original, @encode(id), factory, 2);
    installed += snapshot_method_install(device, "newLibraryWithURL:error:", "library_url", (IMP)snapshot_url, &snapshot_url_original, @encode(id), factory, 2);
    installed += snapshot_method_install(device, "newDefaultLibraryWithBundle:error:", "library_bundle", (IMP)snapshot_bundle, &snapshot_bundle_original, @encode(id), factory, 2);
    installed += snapshot_method_install(quad, "snapshotInRect:scale:time:", "snapshot", (IMP)snapshot_quad, &snapshot_quad_original, @encode(id), quad_arguments, 3);
    installed += snapshot_method_install(commands, "waitUntilCompleted", "wait", (IMP)snapshot_wait, &snapshot_wait_original, @encode(void), NULL, 0);
    installed += snapshot_method_install(commands, "executeOperations", "execute", (IMP)snapshot_execute, &snapshot_execute_original, @encode(void), NULL, 0);
    installed += snapshot_method_install(texture, "getBytes:bytesPerRow:fromRegion:mipmapLevel:", "read", (IMP)snapshot_read, &snapshot_read_original, @encode(void), read_arguments, 4);
    installed += snapshot_method_install(texture, "getBytes:bytesPerRow:bytesPerImage:fromRegion:mipmapLevel:slice:", "read_full", (IMP)snapshot_read_full, &snapshot_read_full_original, @encode(void), full_arguments, 6);
    postdraw_emit("SWEET SNAPSHOT observer pid=%d installed=%u expected=8 limit=64 metadata_only=1 original_arguments_results=1", postdraw_pid, installed);
    errno = saved_errno;
    return installed;
}
_Static_assert(sizeof(CGRect) == 32 && sizeof(CGFloat) == 8, "Snapshot rectangle/scale ABI differs");
_Static_assert(sizeof(MTLRegion) == 48 && sizeof(NSUInteger) == 8, "Texture readback region ABI differs");
