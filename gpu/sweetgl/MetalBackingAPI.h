/* Metal resources share the existing per-process Mesa/SweetScatter device. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SweetMetalBacking {
    uint64_t logical_bytes,allocated_bytes,gpu_address;
    void *contents;
    uint32_t handle,reserved;
} SweetMetalBacking;
#define SWEET_METAL_BACKING_API __attribute__((visibility("default")))
SWEET_METAL_BACKING_API int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *backing);
SWEET_METAL_BACKING_API int sweetgl_metal_buffer_release(uint32_t handle);
SWEET_METAL_BACKING_API int sweetgl_metal_buffer_synchronize(uint32_t handle);
#ifdef __cplusplus
}
#endif
