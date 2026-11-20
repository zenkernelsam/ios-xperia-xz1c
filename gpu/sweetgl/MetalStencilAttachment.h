/* SPDX-License-Identifier: BSD-2-Clause
 * Original Stencil8 plane paired with optional original Depth32Float storage. */
#pragma once
#include "MetalDepthAttachment.h"
typedef struct SweetRenderStencilPass {
    SweetShaderTextureBinding target;
    uint32_t load_action,store_action,clear_stencil,reserved;
    SweetDepthStencilState state;
} SweetRenderStencilPass;
#ifdef __cplusplus
extern "C" {
#endif
SWEET_METAL_SHADER_API int sweetgl_metal_stencil8_supported(void);
SWEET_METAL_SHADER_API int sweetgl_metal_render_draw_stencil(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,
    const SweetRenderStencilPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,
    const SweetShaderSamplerBinding *,uint32_t,uint32_t *);
SWEET_METAL_SHADER_API int sweetgl_metal_render_clear_stencil(const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t *);
#ifdef __cplusplus
}
#endif
