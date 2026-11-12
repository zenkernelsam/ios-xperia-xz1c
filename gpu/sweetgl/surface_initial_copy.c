/* Preserve the initial bitmap supplied to a new IOSurface. RAM trial only.
 * This is a snapshot, not a caller-memory alias or a source for later refreshes. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <mach/mach.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "sweet_drm.h"
#include "SurfaceInitialCopy.h"

extern const CFStringRef kIOSurfaceClientAddress;
_Static_assert(sizeof(vm_offset_t) == sizeof(uintptr_t), "Native VM write data pointer must be full width");
_Static_assert(sizeof(vm_address_t) == 8, "Native VM address must be 64-bit");
_Static_assert(sizeof(vm_size_t) == sizeof(size_t), "Native VM size must be full width");
static pthread_mutex_t budget_lock = PTHREAD_MUTEX_INITIALIZER;
static size_t outstanding_bytes;
static atomic_uint logged;
static void *allocate(void *unused, size_t bytes)
{
    (void)unused; pthread_mutex_lock(&budget_lock);
    bool fits = bytes <= (32u << 20) - outstanding_bytes;
    if (fits) outstanding_bytes += bytes;
    pthread_mutex_unlock(&budget_lock);
    if (!fits) return NULL;
    void *p = malloc(bytes);
    if (!p) { pthread_mutex_lock(&budget_lock); outstanding_bytes -= bytes; pthread_mutex_unlock(&budget_lock); }
    return p;
}
static void release(void *unused, void *p, size_t bytes)
{
    (void)unused; free(p); pthread_mutex_lock(&budget_lock); outstanding_bytes -= bytes; pthread_mutex_unlock(&budget_lock);
}
static int read_self(void *unused, uint64_t address, void *out, size_t bytes, size_t *read)
{
    (void)unused; vm_size_t copied = 0;
    kern_return_t kr = vm_read_overwrite(mach_task_self(), (vm_address_t)address, (vm_size_t)bytes, (vm_address_t)(uintptr_t)out, &copied);
    *read = (size_t)copied; return (int)kr;
}
static int write_self(void *unused, uint64_t address, const void *data, size_t bytes)
{
    (void)unused;
    return (int)vm_write(mach_task_self(), (vm_address_t)address, (vm_offset_t)(uintptr_t)data, (mach_msg_type_number_t)bytes);
}
static int lock_surface(void *context) { return (int)IOSurfaceLock(context, 0, NULL); }
static int unlock_surface(void *context) { return (int)IOSurfaceUnlock(context, 0, NULL); }
static uint64_t number(CFDictionaryRef d, CFStringRef key, bool *present)
{
    CFTypeRef v = CFDictionaryGetValue(d, key); int64_t n = 0;
    *present = v != NULL;
    return v && CFGetTypeID(v) == CFNumberGetTypeID() &&
        CFNumberGetValue(v, kCFNumberSInt64Type, &n) ? (uint64_t)n : 0;
}
void sweetgl_surface_initial_copy(CFDictionaryRef d, IOSurfaceRef surface)
{
    if (!surface || !d || CFGetTypeID(d) != CFDictionaryGetTypeID()) return;
    bool present = false;
    uint64_t address = number(d, kIOSurfaceClientAddress, &present);
    if (!present || !address) return;
    size_t planes = IOSurfaceGetPlaneCount(surface), ew = IOSurfaceGetElementWidth(surface), eh = IOSurfaceGetElementHeight(surface);
    uint32_t seed = IOSurfaceGetSeed(surface);
    bool fresh = sc_new_surface(planes, ew, eh, seed);
    static atomic_uint gates;
    if (atomic_fetch_add_explicit(&gates, 1, memory_order_relaxed) < 32)
        sweet_drm_log("sweetgl: surface copy gate id=%u seed=%u planes=%zu elements=%zux%zu new=%u", IOSurfaceGetID(surface), seed, planes, ew, eh, fresh);
    if (!fresh) return;
    if (number(d, CFSTR("IOSurfaceOffset"), &present)) return;
    struct sc_shape shape = {.source = address, .destination = (uintptr_t)IOSurfaceGetBaseAddress(surface),
        .destination_bytes = IOSurfaceGetAllocSize(surface), .width = IOSurfaceGetWidth(surface),
        .height = IOSurfaceGetHeight(surface), .pitch = IOSurfaceGetBytesPerRow(surface),
        .bytes_per_element = IOSurfaceGetBytesPerElement(surface)};
    uint64_t width = number(d, CFSTR("IOSurfaceWidth"), &present); if (present && width != shape.width) return;
    uint64_t height = number(d, CFSTR("IOSurfaceHeight"), &present); if (present && height != shape.height) return;
    uint64_t pitch = number(d, CFSTR("IOSurfaceBytesPerRow"), &present); if (present && pitch != shape.pitch) return;
    uint64_t bpe = number(d, CFSTR("IOSurfaceBytesPerElement"), &present); if (present && bpe != shape.bytes_per_element) return;
    shape.source_bytes = number(d, CFSTR("IOSurfaceAllocSize"), &present);
    struct sc_ops ops = {.context = (void *)surface, .allocate = allocate, .release = release,
        .read = read_self, .write = write_self, .lock = lock_surface, .unlock = unlock_surface};
    CFRetain(surface);
    struct sc_result r = sc_snapshot(&ops, &shape);
    if (atomic_fetch_add_explicit(&logged, 1, memory_order_relaxed) < 128 || r.status < 0)
        sweet_drm_log("sweetgl: surface initial copy id=%u width=%zu height=%zu bpe=%zu pitch=%zu bytes=%zu source_nonzero=%zu read=%x write=%x lock=%x unlock=%x attempted=%x result=%d hash=%016llx snapshot=1",
            IOSurfaceGetID(surface), shape.width, shape.height, shape.bytes_per_element, shape.pitch, r.bytes,
            r.source_nonzero, r.read_status, r.write_status, r.lock_status, r.unlock_status, r.attempted, r.status,
            (unsigned long long)r.source_hash);
    CFRelease(surface);
}
