// SPDX-License-Identifier: BSD-2-Clause
#pragma once
#import <Metal/Metal.h>
#ifdef __cplusplus
extern "C" {
#endif
// Explicit development factory. It does not replace MTLCreateSystemDefaultDevice
// or advertise a MetalPluginName before the command/shader/render implementation.
__attribute__((visibility("default"))) id<MTLDevice> SweetMTLCreateDevice(NSError **error) NS_RETURNS_RETAINED;
/* Read the completed custom queue's actual fence/operation metadata. */
__attribute__((visibility("default"))) BOOL SweetMTLCommandResult(id<MTLCommandBuffer> commands,
    uint32_t *sequence,uint32_t *operations);
__attribute__((visibility("default"))) int SweetMTLRenderPipelineCacheState(id<MTLRenderPipelineState> pipeline);
/* Author-diagnostic lease of an already materialized transient attachment.
 * It creates no storage and does not expose the public Memoryless texture's
 * contents. The retained original buffer permits independent guard readback. */
typedef struct SweetMTLMemorylessPassInfo {
    uint64_t pass_id,public_texture,gpu_address,buffer_bytes;
    uint32_t slot,handle,width,height,pitch,offset,format,reserved;
} SweetMTLMemorylessPassInfo;
#ifdef __cplusplus
static_assert(sizeof(SweetMTLMemorylessPassInfo)==64,"Transient diagnostic ABI differs");
#else
_Static_assert(sizeof(SweetMTLMemorylessPassInfo)==64,"Transient diagnostic ABI differs");
#endif
__attribute__((visibility("default"))) id<MTLBuffer> SweetMTLRetainMemorylessPassBuffer(id<MTLRenderCommandEncoder> encoder,
    uint32_t slot,SweetMTLMemorylessPassInfo *info) NS_RETURNS_RETAINED;
/* Diagnostic registration through original Metal.framework entry points.
 * No registry properties or framework functions are replaced. */
__attribute__((visibility("default"))) id<MTLDevice> SweetMTLCreateRegisteredDevice(NSError **error) NS_RETURNS_RETAINED;
#ifdef __cplusplus
}
#endif
