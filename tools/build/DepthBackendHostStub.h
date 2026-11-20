/* Existing color/compute host tests do not exercise the new depth GPU backend. */
#pragma once
extern "C" int sweetgl_metal_depth32_supported(void){return 0;}
extern "C" int sweetgl_metal_render_draw_depth(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderDraw *,
    const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *)
{assert(!"This host fixture has no depth GPU backend");return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_depth(const SweetRenderPass *,const SweetRenderDepthPass *,uint32_t *)
{assert(!"This host fixture has no depth GPU backend");return -ENOTSUP;}
extern "C" int sweetgl_metal_stencil8_supported(void){return 0;}
extern "C" int sweetgl_metal_render_draw_stencil(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,const SweetRenderDraw *,
    const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *)
{assert(!"This host fixture has no stencil GPU backend");return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_stencil(const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t *)
{assert(!"This host fixture has no stencil GPU backend");return -ENOTSUP;}
