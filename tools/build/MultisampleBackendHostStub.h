/* Existing single-sample host fixtures must never execute MSAA transport. */
#pragma once
extern "C" int sweetgl_metal_multisample_supported(uint32_t,uint32_t){return 0;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create_multisample(const char *,const char *,const SweetRenderPipelineConfig *,uint32_t,char *,size_t)
{assert(!"Single-sample fixture has no multisample backend");return nullptr;}
extern "C" int sweetgl_metal_render_draw_multisample(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,const SweetRenderDraw *,
    const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t,uint32_t,uint32_t *)
{assert(!"Single-sample fixture has no multisample backend");return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_multisample(const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t,uint32_t *)
{assert(!"Single-sample fixture has no multisample backend");return -ENOTSUP;}
extern "C" int sweetgl_metal_multisample_resolve(const SweetMultisampleResolve *,uint32_t *)
{assert(!"Single-sample fixture has no resolve backend");return -ENOTSUP;}
