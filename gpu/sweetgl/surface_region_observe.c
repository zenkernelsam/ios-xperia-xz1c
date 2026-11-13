/* Metadata only, before the unchanged IOSurface initializer. */
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <mach/vm_region.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "sweet_drm.h"
#include "SurfaceRegionObserve.h"

static atomic_uint small_budget, poster_budget;
static uint64_t number(CFDictionaryRef d, CFStringRef key)
{
    CFTypeRef v = CFDictionaryGetValue(d, key); int64_t n = 0;
    return v && CFGetTypeID(v) == CFNumberGetTypeID() &&
        CFNumberGetValue(v, kCFNumberSInt64Type, &n) ? (uint64_t)n : 0;
}
void sweetgl_surface_region_observe(CFDictionaryRef d)
{
    if (!d || CFGetTypeID(d) != CFDictionaryGetTypeID()) return;
    uint64_t source = number(d, CFSTR("IOSurfaceAddress"));
    if (!source) source = number(d, CFSTR("IOSurfaceClientAddress"));
    uint64_t bytes = number(d, CFSTR("IOSurfaceAllocSize"));
    uint64_t width = number(d, CFSTR("IOSurfaceWidth")), height = number(d, CFSTR("IOSurfaceHeight"));
    struct sro_span span;
    if (!sro_plan(source, bytes, &span)) return;
    bool poster = width == 400 && height == 400;
    unsigned call = atomic_fetch_add_explicit(poster ? &poster_budget : &small_budget, 1, memory_order_relaxed) + 1;
    if (call > 12) return;
    sweet_drm_log("sweetgl: source region begin group=%u call=%u width=%llu height=%llu source=%#llx raw=%llu start=%#llx end=%#llx metadata_only=1",
        poster, call, width, height, source, bytes, span.start, span.end);
    CFIndex count = CFDictionaryGetCount(d);
    if (count <= 32) {
        const void *keys[32], *values[32]; CFDictionaryGetKeysAndValues(d, keys, values);
        for (CFIndex i = 0; i < count; ++i) {
            char key[96] = {0}; CFTypeID type = CFGetTypeID(values[i]);
            if (CFGetTypeID(keys[i]) != CFStringGetTypeID() ||
                !CFStringGetCString(keys[i], key, sizeof(key), kCFStringEncodingUTF8)) continue;
            int64_t n = 0; const char *kind = "other";
            if (type == CFNumberGetTypeID()) { kind = "number"; CFNumberGetValue(values[i], kCFNumberSInt64Type, &n); }
            else if (type == CFBooleanGetTypeID()) { kind = "bool"; n = CFBooleanGetValue(values[i]); }
            else if (type == CFDataGetTypeID()) { kind = "data-length"; n = CFDataGetLength(values[i]); }
            else if (type == CFStringGetTypeID()) kind = "string";
            sweet_drm_log("sweetgl: source property group=%u call=%u key=%s kind=%s number=%lld", poster, call, key, kind, n);
        }
    }
    uint64_t cursor = span.start; unsigned regions = 0; int result = 0;
    for (; cursor < span.end && regions < 8; ++regions) {
        vm_address_t address = cursor; vm_size_t size = 0; natural_t depth = 0;
        vm_region_submap_info_data_64_t info = {0}; kern_return_t kr = KERN_FAILURE;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            mach_msg_type_number_t words = VM_REGION_SUBMAP_INFO_COUNT_64;
            kr = vm_region_recurse_64(mach_task_self(), &address, &size, &depth,
                (vm_region_recurse_info_t)&info, &words);
            if (kr || words < VM_REGION_SUBMAP_INFO_COUNT_64 || !info.is_submap) break;
            ++depth;
        }
        uint64_t next = 0;
        bool covered = !kr && !info.is_submap && sro_advance(&span, cursor, address, size, &next);
        sweet_drm_log("sweetgl: source region item group=%u call=%u index=%u queried=%#llx address=%#llx size=%llu kr=%x prot=%x max=%x tag=%u share=%u submap=%u depth=%u covered=%u",
            poster, call, regions, cursor, (uint64_t)address, (uint64_t)size, kr,
            info.protection, info.max_protection, info.user_tag, info.share_mode, info.is_submap, depth, covered);
        if (!covered) { result = kr ? kr : -1; break; }
        cursor = next;
    }
    if (cursor < span.end && !result) result = -2;
    sweet_drm_log("sweetgl: source region end group=%u call=%u regions=%u result=%d complete=%u metadata_only=1",
        poster, call, regions, result, cursor == span.end);
}
