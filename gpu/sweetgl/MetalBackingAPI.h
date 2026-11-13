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
/* Native queue blits use real A5xx submission and retirement. DWORD aligned
 * regions are accepted; unsupported byte edges are returned as an error. */
SWEET_METAL_BACKING_API int sweetgl_metal_buffer_blit(uint32_t source,uint32_t destination,
    uint64_t source_offset,uint64_t destination_offset,uint64_t bytes,uint32_t fill_byte,
    uint32_t *completed_sequence);
/* Internal buffer views borrow the Mesa BO while Metal retains its owner. */
int sweet_drm_metal_buffer_view(uint32_t handle,SweetMetalBacking *backing);
int sweet_drm_metal_buffer_view_keep(uint32_t handle);
int sweet_drm_metal_buffer_sequence(uint32_t handle,uint32_t *sequence);
#ifdef __cplusplus
}
#endif
