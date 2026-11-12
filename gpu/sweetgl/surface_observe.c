/* Forward all arguments/results unchanged. Record bounded iOS16 creation metadata. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sweet_drm.h"

extern void *IOSurfaceClientCreate(CFDictionaryRef properties);
extern void *IOSurfaceClientCreateChild(void *parent, CFDictionaryRef properties);
extern uint32_t IOSurfaceClientGetID(void *client);
extern void *IOSurfaceClientGetBaseAddress(void *client);
extern const CFStringRef kIOSurfaceClientAddress;
void sweetgl_surface_initial_copy(CFDictionaryRef, IOSurfaceRef);
static atomic_uint observed;
static _Thread_local unsigned nesting;
static id (*original_init)(id, SEL, id);

static uint64_t value(CFDictionaryRef d, CFStringRef key)
{
    CFTypeRef v = CFDictionaryGetValue(d, key);
    int64_t n = 0;
    return v && CFGetTypeID(v) == CFNumberGetTypeID() &&
        CFNumberGetValue((CFNumberRef)v, kCFNumberSInt64Type, &n) ? (uint64_t)n : 0;
}
static void record(const char *entry, CFDictionaryRef d, uint32_t id, void *base)
{
    if (!d || CFGetTypeID(d) != CFDictionaryGetTypeID() ||
        atomic_fetch_add_explicit(&observed, 1, memory_order_relaxed) >= 128) return;
    sweet_drm_log("sweetgl: surface props via=%s id=%u width=%llu height=%llu bpe=%llu pitch=%llu alloc=%llu address=%#llx client_address=%#llx offset=%llu keys=%ld returned=%p forwarded=1",
        entry, id, (unsigned long long)value(d, CFSTR("IOSurfaceWidth")),
        (unsigned long long)value(d, CFSTR("IOSurfaceHeight")),
        (unsigned long long)value(d, CFSTR("IOSurfaceBytesPerElement")),
        (unsigned long long)value(d, CFSTR("IOSurfaceBytesPerRow")),
        (unsigned long long)value(d, CFSTR("IOSurfaceAllocSize")),
        (unsigned long long)value(d, CFSTR("IOSurfaceAddress")),
        (unsigned long long)value(d, kIOSurfaceClientAddress),
        (unsigned long long)value(d, CFSTR("IOSurfaceOffset")), (long)CFDictionaryGetCount(d), base);
}
static IOSurfaceRef observe_create(CFDictionaryRef d)
{
    unsigned previous = nesting++; IOSurfaceRef s = IOSurfaceCreate(d); --nesting;
    if (!previous) { sweetgl_surface_initial_copy(d, s); record("create", d, s ? IOSurfaceGetID(s) : 0, s ? IOSurfaceGetBaseAddress(s) : NULL); }
    return s;
}
static void *observe_client(CFDictionaryRef d)
{
    unsigned previous = nesting++; void *c = IOSurfaceClientCreate(d); --nesting;
    if (!previous) record("client", d, c ? IOSurfaceClientGetID(c) : 0, c ? IOSurfaceClientGetBaseAddress(c) : NULL);
    return c;
}
static void *observe_child(void *parent, CFDictionaryRef d)
{
    unsigned previous = nesting++; void *c = IOSurfaceClientCreateChild(parent, d); --nesting;
    if (!previous) record("child", d, c ? IOSurfaceClientGetID(c) : 0, c ? IOSurfaceClientGetBaseAddress(c) : NULL);
    return c;
}
static id observe_init(id self, SEL command, id d)
{
    unsigned previous = nesting++; id result = original_init(self, command, d); --nesting;
    if (!previous) {
        sweetgl_surface_initial_copy((CFDictionaryRef)d, (IOSurfaceRef)result);
        record("init", (CFDictionaryRef)d, result ? IOSurfaceGetID((IOSurfaceRef)result) : 0,
            result ? IOSurfaceGetBaseAddress((IOSurfaceRef)result) : NULL);
    }
    return result;
}
#define INTERPOSE(replacement, replacee) \
    __attribute__((used, section("__DATA,__interpose"))) \
    static const struct { const void *to; const void *from; } pair_##replacee = \
        {(const void *)(uintptr_t)&replacement, (const void *)(uintptr_t)&replacee}
INTERPOSE(observe_create, IOSurfaceCreate);
INTERPOSE(observe_client, IOSurfaceClientCreate);
INTERPOSE(observe_child, IOSurfaceClientCreateChild);

__attribute__((constructor)) static void install_observer(void)
{
    Method m = class_getInstanceMethod(objc_getClass("IOSurface"), sel_registerName("initWithProperties:"));
    char *returns = m ? method_copyReturnType(m) : NULL;
    char *argument = m ? method_copyArgumentType(m, 2) : NULL;
    bool signature = m && returns && returns[0] == '@' && argument && argument[0] == '@' &&
        method_getNumberOfArguments(m) == 3;
    if (signature) {
        original_init = (id (*)(id, SEL, id))method_getImplementation(m);
        method_setImplementation(m, (IMP)observe_init);
    }
    sweet_drm_log("sweetgl: surface observer iOS16 init=%u encoding=%s initial_copy=1 snapshot_only=1 limit=128", signature,
        m ? method_getTypeEncoding(m) : "missing");
    char key[96] = {0};
    if (kIOSurfaceClientAddress && CFStringGetCString(kIOSurfaceClientAddress, key, sizeof(key), kCFStringEncodingUTF8))
        sweet_drm_log("sweetgl: surface observer native_client_key=%s", key);
    free(returns); free(argument);
}
