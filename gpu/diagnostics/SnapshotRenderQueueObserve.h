/* SPDX-License-Identifier: BSD-2-Clause
 * Observe the original draw bindings and terminal queue result, without drawing. */
#pragma once
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

typedef void (*SweetRenderQueueEmit)(const char *, ...);
#if defined(SWEET_ORIGINAL_QUEUE_RETIREMENT_OBSERVE)
extern BOOL SweetMTLCommandResult(id<MTLCommandBuffer>,uint32_t *,uint32_t *);
#endif
static SweetRenderQueueEmit render_queue_emit;
static IMP render_queue_draw_original;
static Ivar render_queue_vertex, render_queue_fragment, render_queue_pipeline, render_queue_descriptor;
static atomic_uint render_queue_draw_calls, render_queue_result_calls;
enum { SWEET_RENDER_QUEUE_LIMIT = 8 };

static Ivar render_queue_object_ivar(Class owner, const char *name)
{
    Ivar ivar = owner ? class_getInstanceVariable(owner, name) : NULL;
    const char *type = ivar ? ivar_getTypeEncoding(ivar) : NULL;
    ptrdiff_t at = ivar ? ivar_getOffset(ivar) : -1;
    size_t bytes = owner ? class_getInstanceSize(owner) : 0;
    return type && type[0] == '@' && at >= 0 && bytes >= sizeof(id) &&
        (size_t)at <= bytes - sizeof(id) ? ivar : NULL;
}
static bool render_queue_getter(Class owner, SEL selector, const char *result)
{
    Method method = owner ? class_getInstanceMethod(owner, selector) : NULL;
    char actual[64] = {0};
    if (method) method_getReturnType(method, actual, sizeof(actual));
    return method && method_getNumberOfArguments(method) == 2 && !strcmp(actual, result);
}
static bool render_queue_mask(id bindings, unsigned *mask)
{
    *mask = 0;
    if (![bindings isKindOfClass:[NSDictionary class]] || [(NSDictionary *)bindings count] > 8) return false;
    for (id key in (NSDictionary *)bindings) {
        if (![key isKindOfClass:[NSNumber class]]) return false;
        uint64_t index = [(NSNumber *)key unsignedLongLongValue];
        if (index >= 8 || (*mask & (1u << index))) return false;
        *mask |= 1u << index;
    }
    return true;
}
static void render_queue_name(NSString *name, char output[49])
{
    strcpy(output, "none");
    if (![name isKindOfClass:[NSString class]]) return;
    const char *value = name.UTF8String;
    if (!value) return;
    size_t count = strnlen(value, 49);
    if (!count || count > 48) return;
    for (size_t i = 0; i < count; ++i) {
        char c = value[i];
        output[i] = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' ? c : '_';
    }
    output[count] = 0;
}
static void render_queue_draw(id receiver, SEL selector, MTLPrimitiveType primitive,
                              NSUInteger first, NSUInteger count, NSUInteger instances)
{
    int incoming = errno;
    unsigned call = atomic_fetch_add_explicit(&render_queue_draw_calls, 1, memory_order_relaxed) + 1;
    if (render_queue_emit && call <= SWEET_RENDER_QUEUE_LIMIT) {
        @try {
            unsigned vertex = 0, fragment = 0;
            bool valid = render_queue_mask(object_getIvar(receiver, render_queue_vertex), &vertex) &&
                render_queue_mask(object_getIvar(receiver, render_queue_fragment), &fragment);
            id pipeline = object_getIvar(receiver, render_queue_pipeline);
            MTLRenderPipelineDescriptor *descriptor = pipeline &&
                !strcmp(object_getClassName(pipeline), "SweetMTLRenderPipeline") ?
                object_getIvar(pipeline, render_queue_descriptor) : nil;
            char vname[49], fname[49];
            render_queue_name(descriptor.vertexFunction.name, vname);
            render_queue_name(descriptor.fragmentFunction.name, fname);
            render_queue_emit("SWEET RQUEUE DRAW pid=%d call=%u primitive=%lu first=%lu count=%lu instances=%lu",
                getpid(), call, (unsigned long)primitive, (unsigned long)first,
                (unsigned long)count, (unsigned long)instances);
            render_queue_emit("SWEET RQUEUE BIND pid=%d call=%u vertex=%u fragment=%u valid=%u original_state=1",
                getpid(), call, vertex, fragment, valid);
            render_queue_emit("SWEET RQUEUE PIPE pid=%d call=%u vertex=%s fragment=%s",
                getpid(), call, vname, fname);
        } @catch (id diagnostic_error) {
            (void)diagnostic_error;
            render_queue_emit("SWEET RQUEUE REFUSED pid=%d call=%u original_forwarded=1", getpid(), call);
        }
    }
    bool returned = false;
    @try {
        errno = incoming;
        ((void (*)(id, SEL, MTLPrimitiveType, NSUInteger, NSUInteger, NSUInteger))render_queue_draw_original)(
            receiver, selector, primitive, first, count, instances);
        returned = true;
    } @finally {
        int outgoing = errno;
        if (render_queue_emit && call <= SWEET_RENDER_QUEUE_LIMIT)
            render_queue_emit("SWEET RQUEUE DRAW_END pid=%d call=%u outcome=%s original_behavior=1",
                getpid(), call, returned ? "return" : "throw");
        errno = outgoing;
    }
}
static void snapshot_render_queue_result(id receiver)
{
    int incoming = errno;
    unsigned call = atomic_fetch_add_explicit(&render_queue_result_calls, 1, memory_order_relaxed) + 1;
    if (!render_queue_emit || call > SWEET_RENDER_QUEUE_LIMIT) { errno = incoming; return; }
    @try {
        Class owner = object_getClass(receiver);
        bool valid = receiver && !strcmp(class_getName(owner), "SweetMTLCommandBuffer") &&
            render_queue_getter(owner, @selector(status), @encode(NSUInteger)) &&
            render_queue_getter(owner, @selector(error), @encode(id));
        if (valid) {
            id<MTLCommandBuffer> commands = receiver;
            NSUInteger status = commands.status;
            NSError *error = commands.error;
            char text[129] = "none";
            if (error) {
                const char *value = error.localizedDescription.UTF8String;
                if (value) {
                    size_t size = strnlen(value, 128);
                    for (size_t i = 0; i < size; ++i) {
                        unsigned char c = value[i];
                        text[i] = c >= 33 && c <= 126 ? c : '_';
                    }
                    text[size] = 0;
                }
            }
            render_queue_emit("SWEET RQUEUE RESULT pid=%d call=%u status=%lu error_nil=%u code=%lld terminal_getters_only=1",
                getpid(), call, (unsigned long)status, error == nil, (long long)(error ? error.code : 0));
            render_queue_emit("SWEET RQUEUE ERROR pid=%d call=%u text=%s", getpid(), call, text);
#if defined(SWEET_ORIGINAL_QUEUE_RETIREMENT_OBSERVE)
            if(status==MTLCommandBufferStatusCompleted && !error) {
                uint32_t fence=0,operations=0;
                BOOL complete=SweetMTLCommandResult(commands,&fence,&operations);
                render_queue_emit("SWEET RQUEUE RETIRED pid=%d call=%u fence=%u operations=%u complete=%u original_command=1",
                    getpid(),call,fence,operations,complete);
            }
#endif
        } else render_queue_emit("SWEET RQUEUE RESULT_REFUSED pid=%d call=%u terminal_getters_only=1", getpid(), call);
    } @catch (id diagnostic_error) {
        (void)diagnostic_error;
        render_queue_emit("SWEET RQUEUE RESULT_REFUSED pid=%d call=%u terminal_getters_only=1", getpid(), call);
    }
    errno = incoming;
}
static bool snapshot_render_queue_observe(Class encoder, Class pipeline, SweetRenderQueueEmit emit)
{
    int incoming = errno;
    render_queue_emit = emit;
    bool valid = encoder && pipeline && !strcmp(class_getName(encoder), "SweetMTLRenderEncoder") &&
        !strcmp(class_getName(pipeline), "SweetMTLRenderPipeline");
    render_queue_vertex = valid ? render_queue_object_ivar(encoder, "_vertexBuffers") : NULL;
    render_queue_fragment = valid ? render_queue_object_ivar(encoder, "_fragmentBuffers") : NULL;
    render_queue_pipeline = valid ? render_queue_object_ivar(encoder, "_pipeline") : NULL;
    render_queue_descriptor = valid ? render_queue_object_ivar(pipeline, "_descriptor") : NULL;
    SEL selector = @selector(drawPrimitives:vertexStart:vertexCount:instanceCount:);
    Method method = valid ? class_getInstanceMethod(encoder, selector) : NULL;
    char types[64] = {0};
    if (method) method_getReturnType(method, types, sizeof(types));
    valid = method && method_getNumberOfArguments(method) == 6 && !strcmp(types, @encode(void)) &&
        render_queue_vertex && render_queue_fragment && render_queue_pipeline && render_queue_descriptor &&
        !render_queue_draw_original;
    const char *arguments[] = {@encode(MTLPrimitiveType), @encode(NSUInteger), @encode(NSUInteger), @encode(NSUInteger)};
    for (unsigned i = 0; valid && i < 4; ++i) {
        method_getArgumentType(method, i + 2, types, sizeof(types));
        valid = !strcmp(types, arguments[i]);
    }
    IMP original = valid ? method_getImplementation(method) : NULL;
    valid = valid && original && original != (IMP)render_queue_draw;
    if (valid) {
        render_queue_draw_original = original;
        method_setImplementation(method, (IMP)render_queue_draw);
    }
    if (emit) emit("SWEET RQUEUE CONFIG pid=%d revision=1 installed=%u limit=8 original_arguments_results=1 extra_GPU_work=0",
        getpid(), valid);
    errno = incoming;
    return valid;
}
