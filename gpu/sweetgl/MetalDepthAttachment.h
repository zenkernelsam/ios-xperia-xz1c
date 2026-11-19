/* SPDX-License-Identifier: BSD-2-Clause
 * Separate ABI for original Depth32Float attachments; existing color ABI stays. */
#pragma once
#include "MetalShaderAPI.h"
#include "MetalDepthStencilRules.h"

typedef struct SweetRenderDepthPass {
    SweetShaderTextureBinding target;
    uint32_t load_action, store_action;
    double clear_depth;
    SweetDepthStencilState state;
} SweetRenderDepthPass;

#ifdef __cplusplus
extern "C" {
#endif
SWEET_METAL_SHADER_API int sweetgl_metal_depth32_supported(void);
SWEET_METAL_SHADER_API int sweetgl_metal_render_draw_depth(SweetMetalProgram *program,const SweetRenderPass *pass,
    const SweetRenderDepthPass *depth,const SweetRenderDraw *draw,const SweetShaderBufferBinding *buffers,uint32_t buffer_count,
    const SweetShaderTextureBinding *textures,uint32_t texture_count,const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence);
SWEET_METAL_SHADER_API int sweetgl_metal_render_clear_depth(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,uint32_t *sequence);
#ifdef __cplusplus
}
#endif
