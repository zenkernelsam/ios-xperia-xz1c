/* SPDX-License-Identifier: BSD-2-Clause
 * Independent backend ABI for real 2x/4x pages and GPU resolve.
 * Existing color/depth/stencil ABI and native single-sample APIs stay stable. */
#pragma once
#include "MetalStencilAttachment.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SweetMultisampleResolve {
    SweetShaderTextureBinding source,target;
    uint32_t samples,aspect,filter,reserved;
} SweetMultisampleResolve;
SWEET_METAL_SHADER_API int sweetgl_metal_multisample_supported(uint32_t format,uint32_t samples);
SWEET_METAL_SHADER_API SweetMetalProgram *sweetgl_metal_render_create_multisample(const char *,const char *,
    const SweetRenderPipelineConfig *,uint32_t samples,char *,size_t);
SWEET_METAL_SHADER_API int sweetgl_metal_render_draw_multisample(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,
    const SweetRenderStencilPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,
    const SweetShaderSamplerBinding *,uint32_t,uint32_t samples,uint32_t sample_mask,uint32_t *sequence);
SWEET_METAL_SHADER_API int sweetgl_metal_render_clear_multisample(const SweetRenderPass *,const SweetRenderDepthPass *,
    const SweetRenderStencilPass *,uint32_t samples,uint32_t *sequence);
/* aspect0=color average; aspect1=depth with filter0=sample0,1=min,2=max.
 * Stencil resolve and XR average retain an explicit unsupported result. */
SWEET_METAL_SHADER_API int sweetgl_metal_multisample_resolve(const SweetMultisampleResolve *,uint32_t *sequence);
#ifdef __cplusplus
}
#endif
