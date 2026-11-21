/* SPDX-License-Identifier: BSD-2-Clause
 * Observe original scene/queue scheduling. Never request an additional draw. */
#pragma once
#import <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>

typedef void (*SweetQuadRouteEmit)(const char *, ...);
enum SweetQuadRouteAPI {
    QR_MANAGER_DEVICE, QR_MANAGER_QUEUE, QR_DEVICE_QUEUE, QR_ADD_QUAD, QR_ADD_QUADS,
    QR_WINDOW, QR_PAUSED, QR_DISABLE, QR_ENABLE, QR_SYNC, QR_TICK, QR_SCENE,
    QR_PREVIEW, QR_SNAPSHOT, QR_LOW_LIGHT, QR_BACKLIGHT, QR_LINEAR_BACKLIGHT, QR_UNLOCK,
    QR_MIN_SIZE, QR_DESIRED_SIZE, QR_START, QR_STOP, QR_UPDATE, QR_SNAPSHOT_IMAGE,
    QR_ENV_INIT, QR_ENV_UPDATE, QR_RENDERER_ENV,
    QR_APPLY_MODE, QR_TRANSITION_MODE, QR_ACTIVE_MODE, QR_DEACTIVE_MODE, QR_INTERACTIVE_MODE, QR_SUPPLEMENTAL_MODE,
    QR_API_COUNT
};
enum { QR_SAMPLES_PER_API = 8, QR_SAMPLES_MAX = QR_API_COUNT * QR_SAMPLES_PER_API };
static const char *const quad_route_apis[] = {
    "manager_device", "manager_queue", "device_queue", "add_quad", "add_quads",
    "window", "paused", "disable", "enable", "sync", "tick", "scene",
    "preview", "snapshot", "low_light", "backlight", "linear_backlight", "unlock",
    "min_size", "desired_size", "start", "stop", "update", "snapshot_image", "env_init", "env_update", "renderer_env",
    "apply_mode", "transition_mode", "active_mode", "deactive_mode", "interactive_mode", "supplemental_mode"
};
static IMP quad_route_original[QR_API_COUNT];
static atomic_uint quad_route_calls[QR_API_COUNT], quad_route_records;
static SweetQuadRouteEmit quad_route_emit;

static unsigned quad_route_begin(unsigned api, id receiver, id argument)
{
    unsigned seen = atomic_fetch_add_explicit(&quad_route_calls[api], 1, memory_order_relaxed) + 1;
    if (seen > QR_SAMPLES_PER_API) {
        if (seen == QR_SAMPLES_PER_API + 1)
            quad_route_emit("SWEET QUAD ROUTE LIMIT pid=%d api=%s per_api=8 original_forwarded=1", getpid(), quad_route_apis[api]);
        return 0;
    }
    unsigned call = atomic_fetch_add_explicit(&quad_route_records, 1, memory_order_relaxed) + 1;
    quad_route_emit("SWEET QUAD ROUTE BEGIN pid=%d call=%u api=%s original_forwarded=1", getpid(), call, quad_route_apis[api]);
    quad_route_emit("SWEET QUAD ROUTE OBJECT pid=%d call=%u receiver=%.40s argument_nil=%u argument_class=%.40s",
        getpid(), call, receiver ? object_getClassName(receiver) : "nil", argument == nil, argument ? object_getClassName(argument) : "nil");
    quad_route_emit("SWEET QUAD ROUTE ID pid=%d call=%u receiver=%016llx argument=%016llx", getpid(), call,
        (unsigned long long)(uintptr_t)receiver, (unsigned long long)(uintptr_t)argument);
    return call;
}
static void quad_route_end(unsigned call, unsigned api, id answer, bool returned)
{
    if (!call) return;
    quad_route_emit("SWEET QUAD ROUTE RETURN pid=%d call=%u returned_nil=%u return_class=%.40s",
        getpid(), call, answer == nil, answer ? object_getClassName(answer) : "nil");
    quad_route_emit("SWEET QUAD ROUTE RETURN_ID pid=%d call=%u value=%016llx", getpid(), call, (unsigned long long)(uintptr_t)answer);
    quad_route_emit("SWEET QUAD ROUTE END pid=%d call=%u api=%s outcome=%s original_behavior=1",
        getpid(), call, quad_route_apis[api], returned ? "return" : "throw");
}
static id quad_route_id0(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); id answer = nil; bool returned = false;
    @try { errno = incoming; answer = ((id (*)(id, SEL))quad_route_original[api])(receiver, selector); returned = true; return answer; }
    @finally { int outgoing = errno; quad_route_end(call, api, answer, returned); errno = outgoing; }
}
static id quad_route_manager_device(id receiver, SEL selector) { return quad_route_id0(receiver, selector, QR_MANAGER_DEVICE); }
static id quad_route_manager_queue(id receiver, SEL selector) { return quad_route_id0(receiver, selector, QR_MANAGER_QUEUE); }
static id quad_route_device_queue(id receiver, SEL selector) { return quad_route_id0(receiver, selector, QR_DEVICE_QUEUE); }
static id quad_route_snapshot_image(id receiver, SEL selector) { return quad_route_id0(receiver, selector, QR_SNAPSHOT_IMAGE); }
static id quad_route_renderer_env(id receiver, SEL selector) { return quad_route_id0(receiver, selector, QR_RENDERER_ENV); }

static void quad_route_reason(unsigned call, id reason)
{
    if (!call) return;
    char value[97] = {0}; size_t count = 0; bool text = false, truncated = false, getter_error = false;
    @try {
        if ([reason isKindOfClass:[NSString class]]) {
            const char *original = [(NSString *)reason UTF8String]; text = original != NULL;
            size_t size = original ? strnlen(original, sizeof(value)) : 0;
            count = size > 96 ? 96 : size; truncated = size > 96;
            if (count) memcpy(value, original, count);
        }
    } @catch (id error) { (void)error; getter_error = true; count = 0; }
    quad_route_emit("SWEET QUAD ROUTE REASON pid=%d call=%u string=%u bytes=%lu truncated=%u getter_error=%u",
        getpid(), call, text, (unsigned long)count, truncated, getter_error);
    for (size_t at = 0; at < count; at += 24) {
        char hex[49] = {0}; size_t size = count - at < 24 ? count - at : 24;
        for (size_t i = 0; i < size; ++i) snprintf(hex + i*2, 3, "%02x", (unsigned char)value[at+i]);
        quad_route_emit("SWEET QUAD ROUTE REASON_DATA pid=%d call=%u offset=%lu bytes=%lu hex=%s",
            getpid(), call, (unsigned long)at, (unsigned long)size, hex);
    }
}
static void quad_route_void1(id receiver, SEL selector, id argument, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, argument); bool returned = false;
    if (api == QR_DISABLE || api == QR_ENABLE) quad_route_reason(call, argument);
    @try { errno = incoming; ((void (*)(id, SEL, id))quad_route_original[api])(receiver, selector, argument); returned = true; }
    @finally { int outgoing = errno; quad_route_end(call, api, nil, returned); errno = outgoing; }
}
static void quad_route_add(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_ADD_QUAD); }
static void quad_route_adds(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_ADD_QUADS); }
static void quad_route_disable(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_DISABLE); }
static void quad_route_enable(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_ENABLE); }
static void quad_route_tick(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_TICK); }
static void quad_route_scene(id receiver, SEL selector, id argument) { quad_route_void1(receiver, selector, argument, QR_SCENE); }
static void quad_route_void0(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); bool returned = false;
    @try { errno = incoming; ((void (*)(id, SEL))quad_route_original[api])(receiver, selector); returned = true; }
    @finally { int outgoing = errno; quad_route_end(call, api, nil, returned); errno = outgoing; }
}
static void quad_route_window(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_WINDOW); }
static void quad_route_start(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_START); }
static void quad_route_stop(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_STOP); }
static void quad_route_update(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_UPDATE); }
static void quad_route_active(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_ACTIVE_MODE); }
static void quad_route_deactive(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_DEACTIVE_MODE); }
static void quad_route_interactive(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_INTERACTIVE_MODE); }
static void quad_route_supplemental(id receiver, SEL selector) { quad_route_void0(receiver, selector, QR_SUPPLEMENTAL_MODE); }
static void quad_route_mode(id receiver, SEL selector, NSInteger mode, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); bool returned = false;
    uint64_t bits = 0; memcpy(&bits, &mode, sizeof(bits));
    if (call) quad_route_emit("SWEET QUAD ROUTE INTEGER pid=%d call=%u bits=%016llx", getpid(), call, (unsigned long long)bits);
    @try { errno = incoming; ((void (*)(id, SEL, NSInteger))quad_route_original[api])(receiver, selector, mode); returned = true; }
    @finally { int outgoing = errno; quad_route_end(call, api, nil, returned); errno = outgoing; }
}
static void quad_route_apply_mode(id receiver, SEL selector, NSInteger mode) { quad_route_mode(receiver, selector, mode, QR_APPLY_MODE); }
static void quad_route_transition_mode(id receiver, SEL selector, NSInteger mode) { quad_route_mode(receiver, selector, mode, QR_TRANSITION_MODE); }
static BOOL quad_route_bool0(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); BOOL answer = NO; bool returned = false;
    @try { errno = incoming; answer = ((BOOL (*)(id, SEL))quad_route_original[api])(receiver, selector); returned = true; return answer; }
    @finally {
        int outgoing = errno;
        if (call) quad_route_emit("SWEET QUAD ROUTE BOOL pid=%d call=%u value=%u", getpid(), call, answer != NO);
        quad_route_end(call, api, nil, returned); errno = outgoing;
    }
}
static BOOL quad_route_preview(id receiver, SEL selector) { return quad_route_bool0(receiver, selector, QR_PREVIEW); }
static BOOL quad_route_snapshot(id receiver, SEL selector) { return quad_route_bool0(receiver, selector, QR_SNAPSHOT); }
static BOOL quad_route_low_light(id receiver, SEL selector) { return quad_route_bool0(receiver, selector, QR_LOW_LIGHT); }
static double quad_route_double0(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); double answer = 0; bool returned = false;
    @try { errno = incoming; answer = ((double (*)(id, SEL))quad_route_original[api])(receiver, selector); returned = true; return answer; }
    @finally {
        int outgoing = errno; uint64_t bits = 0; memcpy(&bits, &answer, sizeof(bits));
        if (call) quad_route_emit("SWEET QUAD ROUTE DOUBLE pid=%d call=%u bits=%016llx", getpid(), call, (unsigned long long)bits);
        quad_route_end(call, api, nil, returned); errno = outgoing;
    }
}
static double quad_route_backlight(id receiver, SEL selector) { return quad_route_double0(receiver, selector, QR_BACKLIGHT); }
static double quad_route_linear(id receiver, SEL selector) { return quad_route_double0(receiver, selector, QR_LINEAR_BACKLIGHT); }
static double quad_route_unlock(id receiver, SEL selector) { return quad_route_double0(receiver, selector, QR_UNLOCK); }
static CGSize quad_route_size0(id receiver, SEL selector, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, nil); CGSize answer = {0}; bool returned = false;
    @try { errno = incoming; answer = ((CGSize (*)(id, SEL))quad_route_original[api])(receiver, selector); returned = true; return answer; }
    @finally {
        int outgoing = errno; uint64_t width = 0, height = 0; memcpy(&width, &answer.width, 8); memcpy(&height, &answer.height, 8);
        if (call) quad_route_emit("SWEET QUAD ROUTE SIZE pid=%d call=%u width_bits=%016llx height_bits=%016llx", getpid(), call, (unsigned long long)width, (unsigned long long)height);
        quad_route_end(call, api, nil, returned); errno = outgoing;
    }
}
static CGSize quad_route_min_size(id receiver, SEL selector) { return quad_route_size0(receiver, selector, QR_MIN_SIZE); }
static CGSize quad_route_desired_size(id receiver, SEL selector) { return quad_route_size0(receiver, selector, QR_DESIRED_SIZE); }
static void quad_route_environment(id receiver, SEL selector, id renderer, id environment, id transition, unsigned api)
{
    int incoming = errno; unsigned call = quad_route_begin(api, receiver, environment); bool returned = false;
    if (call) quad_route_emit("SWEET QUAD ROUTE ENV pid=%d call=%u renderer=%016llx transition=%016llx", getpid(), call,
        (unsigned long long)(uintptr_t)renderer, (unsigned long long)(uintptr_t)transition);
    @try {
        errno = incoming;
        if (api == QR_ENV_INIT) ((void (*)(id, SEL, id, id))quad_route_original[api])(receiver, selector, renderer, environment);
        else ((void (*)(id, SEL, id, id, id))quad_route_original[api])(receiver, selector, renderer, environment, transition);
        returned = true;
    } @finally { int outgoing = errno; quad_route_end(call, api, nil, returned); errno = outgoing; }
}
static void quad_route_env_init(id receiver, SEL selector, id renderer, id environment) { quad_route_environment(receiver, selector, renderer, environment, nil, QR_ENV_INIT); }
static void quad_route_env_update(id receiver, SEL selector, id renderer, id environment, id transition) { quad_route_environment(receiver, selector, renderer, environment, transition, QR_ENV_UPDATE); }
static void quad_route_paused(id receiver, SEL selector, BOOL paused)
{
    int incoming = errno; unsigned call = quad_route_begin(QR_PAUSED, receiver, nil); bool returned = false;
    if (call) quad_route_emit("SWEET QUAD ROUTE FLAG pid=%d call=%u value=%u", getpid(), call, paused != NO);
    @try { errno = incoming; ((void (*)(id, SEL, BOOL))quad_route_original[QR_PAUSED])(receiver, selector, paused); returned = true; }
    @finally { int outgoing = errno; quad_route_end(call, QR_PAUSED, nil, returned); errno = outgoing; }
}
static BOOL quad_route_sync(id receiver, SEL selector, double time, id group, BOOL check, id completion)
{
    int incoming = errno; unsigned call = quad_route_begin(QR_SYNC, receiver, group); BOOL answer = NO; bool returned = false;
    if (call) quad_route_emit("SWEET QUAD ROUTE SYNC pid=%d call=%u time=%.17g check=%u completion_nil=%u", getpid(), call, time, check != NO, completion == nil);
    @try {
        errno = incoming; answer = ((BOOL (*)(id, SEL, double, id, BOOL, id))quad_route_original[QR_SYNC])(receiver, selector, time, group, check, completion);
        returned = true; return answer;
    } @finally {
        int outgoing = errno;
        if (call) quad_route_emit("SWEET QUAD ROUTE BOOL pid=%d call=%u value=%u", getpid(), call, answer != NO);
        quad_route_end(call, QR_SYNC, nil, returned); errno = outgoing;
    }
}
static bool quad_route_install(Class owner, bool metaclass, const char *owner_name, const char *selector_name,
    unsigned api, IMP replacement, const char *result, const char *const *arguments, unsigned count)
{
    int incoming = errno; SEL selector = sel_registerName(selector_name);
    const char *actual_owner=owner?class_getName(owner):"";
    bool known_swift_owner=!strcmp(owner_name,"_TtC11AegirPoster24AegirRenderingController") &&
        !strcmp(actual_owner,"AegirPoster.AegirRenderingController");
    bool owner_valid = owner && class_isMetaClass(owner) == metaclass && (!strcmp(actual_owner,owner_name) || known_swift_owner);
    Method method = owner_valid ? class_getInstanceMethod(owner, selector) : NULL;
    unsigned total = 0; Method *list = owner_valid ? class_copyMethodList(owner, &total) : NULL; bool owned = false;
    for (unsigned i = 0; i < total; ++i) if (method_getName(list[i]) == selector) owned = true;
    free(list);
    char actual[192] = {0}; if (method) method_getReturnType(method, actual, sizeof(actual));
    bool valid = method && owned && method_getNumberOfArguments(method) == count+2 && !strcmp(actual, result);
    for (unsigned i = 0; valid && i < count; ++i) { method_getArgumentType(method, i+2, actual, sizeof(actual)); valid = !strcmp(actual, arguments[i]); }
    const char *types = method ? method_getTypeEncoding(method) : NULL; size_t bytes = types ? strlen(types) : 0;
    IMP original = valid ? method_getImplementation(method) : NULL;
    valid = valid && original && original != replacement && bytes > 0 && bytes <= 144;
    if (valid) { quad_route_original[api] = original; method_setImplementation(method, replacement); }
    quad_route_emit("SWEET QUAD ROUTE METHOD pid=%d api=%s present=%u owned=%u installed=%u arguments=%u owner_checked=%u meta=%u",
        getpid(), quad_route_apis[api], method != NULL, owned, valid, method ? method_getNumberOfArguments(method) : 0, owner_valid, metaclass);
    quad_route_emit("SWEET QUAD ROUTE ABI pid=%d api=%s bytes=%lu truncated=%u", getpid(), quad_route_apis[api], (unsigned long)(bytes > 144 ? 144 : bytes), bytes > 144);
    for (size_t at = 0; at < bytes && at < 144; at += 24) {
        char hex[49] = {0}; size_t size = bytes - at < 24 ? bytes - at : 24;
        for (size_t i = 0; i < size; ++i) snprintf(hex + i*2, 3, "%02x", (unsigned char)types[at+i]);
        quad_route_emit("SWEET QUAD ROUTE ABI_DATA pid=%d api=%s offset=%lu bytes=%lu hex=%s", getpid(), quad_route_apis[api], (unsigned long)at, (unsigned long)size, hex);
    }
    quad_route_emit("SWEET QUAD ROUTE ABI_END pid=%d api=%s", getpid(), quad_route_apis[api]);
    errno = incoming; return valid;
}
static unsigned quad_route_observe(Class device, Class manager, Class base, Class coordinator, Class scene, Class environment, Class vista, Class controller, Class renderer, Class vista_controller, SweetQuadRouteEmit emit)
{
    int incoming = errno; quad_route_emit = emit; unsigned installed = 0;
    Class mangled=controller,qualified=Nil;
    if(!controller)controller=NSClassFromString(@"_TtC11AegirPoster24AegirRenderingController");
    if(!controller){qualified=NSClassFromString(@"AegirPoster.AegirRenderingController");controller=qualified;}
    quad_route_emit("SWEET POSTER OWNER mangled=%u qualified=%u selected_nil=%u name=%s",
        mangled!=Nil,qualified!=Nil,controller==Nil,controller?class_getName(controller):"nil");
    const char *object[] = {@encode(id)}, *flag[] = {@encode(BOOL)};
    const char *sync[] = {@encode(double), @encode(id), @encode(BOOL), "@?"};
    Class meta = manager ? object_getClass(manager) : Nil;
    installed += quad_route_install(meta, true, "CLKUIMetalResourceManager", "sharedDevice", QR_MANAGER_DEVICE, (IMP)quad_route_manager_device, @encode(id), NULL, 0);
    installed += quad_route_install(meta, true, "CLKUIMetalResourceManager", "sharedCommandQueue", QR_MANAGER_QUEUE, (IMP)quad_route_manager_queue, @encode(id), NULL, 0);
    installed += quad_route_install(device, false, "SweetMTLDevice", "newCommandQueue", QR_DEVICE_QUEUE, (IMP)quad_route_device_queue, @encode(id), NULL, 0);
    installed += quad_route_install(base, false, "CLKUIQuadView", "addQuad:", QR_ADD_QUAD, (IMP)quad_route_add, @encode(void), object, 1);
    installed += quad_route_install(base, false, "CLKUIQuadView", "addQuadsFromArray:", QR_ADD_QUADS, (IMP)quad_route_adds, @encode(void), object, 1);
    installed += quad_route_install(base, false, "CLKUIQuadView", "didMoveToWindow", QR_WINDOW, (IMP)quad_route_window, @encode(void), NULL, 0);
    installed += quad_route_install(base, false, "CLKUIQuadView", "setPaused:", QR_PAUSED, (IMP)quad_route_paused, @encode(void), flag, 1);
    installed += quad_route_install(base, false, "CLKUIQuadView", "addDisabledRenderingReason:", QR_DISABLE, (IMP)quad_route_disable, @encode(void), object, 1);
    installed += quad_route_install(base, false, "CLKUIQuadView", "removeDisabledRenderingReason:", QR_ENABLE, (IMP)quad_route_enable, @encode(void), object, 1);
    installed += quad_route_install(base, false, "CLKUIQuadView", "_sync_prepareAndRenderForTime:inGroup:checkForDrawable:completion:", QR_SYNC, (IMP)quad_route_sync, @encode(BOOL), sync, 4);
    installed += quad_route_install(coordinator, false, "CLKUIQuadViewRenderCoordinator", "_renderLinkFired:", QR_TICK, (IMP)quad_route_tick, @encode(void), object, 1);
    installed += quad_route_install(scene, false, "NUNIQuad", "setScene:", QR_SCENE, (IMP)quad_route_scene, @encode(void), object, 1);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "isPreview", QR_PREVIEW, (IMP)quad_route_preview, @encode(BOOL), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "isSnapshot", QR_SNAPSHOT, (IMP)quad_route_snapshot, @encode(BOOL), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "isLowLuminance", QR_LOW_LIGHT, (IMP)quad_route_low_light, @encode(BOOL), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "backlightProgress", QR_BACKLIGHT, (IMP)quad_route_backlight, @encode(double), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "linearBacklightProgress", QR_LINEAR_BACKLIGHT, (IMP)quad_route_linear, @encode(double), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "unlockProgress", QR_UNLOCK, (IMP)quad_route_unlock, @encode(double), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "minimumContentSize", QR_MIN_SIZE, (IMP)quad_route_min_size, @encode(CGSize), NULL, 0);
    installed += quad_route_install(environment, false, "PRPosterEnvironmentImpl", "desiredContentSize", QR_DESIRED_SIZE, (IMP)quad_route_desired_size, @encode(CGSize), NULL, 0);
    installed += quad_route_install(vista, false, "NUNIAstronomyVistaView", "startAnimation", QR_START, (IMP)quad_route_start, @encode(void), NULL, 0);
    installed += quad_route_install(vista, false, "NUNIAstronomyVistaView", "stopAnimation", QR_STOP, (IMP)quad_route_stop, @encode(void), NULL, 0);
    installed += quad_route_install(vista, false, "NUNIAstronomyVistaView", "_updateAnimation", QR_UPDATE, (IMP)quad_route_update, @encode(void), NULL, 0);
    installed += quad_route_install(vista, false, "NUNIAstronomyVistaView", "snapshotImage", QR_SNAPSHOT_IMAGE, (IMP)quad_route_snapshot_image, @encode(id), NULL, 0);
    const char *pair[] = {@encode(id), @encode(id)}, *triple[] = {@encode(id), @encode(id), @encode(id)};
    installed += quad_route_install(controller, false, "_TtC11AegirPoster24AegirRenderingController", "renderer:didInitializeWithEnvironment:", QR_ENV_INIT, (IMP)quad_route_env_init, @encode(void), pair, 2);
    installed += quad_route_install(controller, false, "_TtC11AegirPoster24AegirRenderingController", "renderer:didUpdateEnvironment:withTransition:", QR_ENV_UPDATE, (IMP)quad_route_env_update, @encode(void), triple, 3);
    installed += quad_route_install(renderer, false, "PRRenderer", "environment", QR_RENDERER_ENV, (IMP)quad_route_renderer_env, @encode(id), NULL, 0);
    const char *mode[] = {@encode(NSInteger)};
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "applyMode:", QR_APPLY_MODE, (IMP)quad_route_apply_mode, @encode(void), mode, 1);
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "animateTransitionToMode:", QR_TRANSITION_MODE, (IMP)quad_route_transition_mode, @encode(void), mode, 1);
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "activeMode", QR_ACTIVE_MODE, (IMP)quad_route_active, @encode(void), NULL, 0);
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "deactiveMode", QR_DEACTIVE_MODE, (IMP)quad_route_deactive, @encode(void), NULL, 0);
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "interactiveMode", QR_INTERACTIVE_MODE, (IMP)quad_route_interactive, @encode(void), NULL, 0);
    installed += quad_route_install(vista_controller, false, "NUNIAstronomyVistaController", "supplementalMode", QR_SUPPLEMENTAL_MODE, (IMP)quad_route_supplemental, @encode(void), NULL, 0);
    quad_route_emit("SWEET QUAD ROUTE CONFIG pid=%d installed=%u expected=33 per_api=8 max=264 revision=4 original_behavior=1 no_extra_GPU_work=1", getpid(), installed);
    errno = incoming; return installed;
}
_Static_assert(sizeof(double) == 8 && sizeof(CGSize) == 16, "Original scalar/CGSize ABI differs");
_Static_assert(sizeof(NSInteger) == 8, "Original signed mode ABI differs");
