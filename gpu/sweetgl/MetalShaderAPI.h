/* Stable C boundary: only ESSL, owned buffer handles and bounded dispatch. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SweetMetalProgram SweetMetalProgram;
typedef struct SweetShaderBufferBinding {uint32_t handle,index;uint64_t offset,bytes;} SweetShaderBufferBinding;
typedef struct SweetShaderTextureBinding {
    uint32_t handle,index,stage,width,height,pitch,format,usage;
    uint64_t offset;
} SweetShaderTextureBinding;
typedef struct SweetShaderSamplerBinding {
    uint32_t index,stage,min_filter,mag_filter,mip_filter,address_s,address_t,normalized;
    float minimum_lod,maximum_lod;
} SweetShaderSamplerBinding;
typedef struct SweetRenderPipelineConfig {
    uint32_t format,vertex_buffers,fragment_buffers,fragment_writes;
    uint32_t vertex_texture_slots,fragment_texture_slots;
    uint32_t blend_enabled,source_rgb,destination_rgb,operation_rgb,source_alpha,destination_alpha,operation_alpha,write_mask;
} SweetRenderPipelineConfig;
typedef struct SweetRenderDraw {
    uint32_t primitive,first,count,instances,cull,winding;
    double x,y,width,height,znear,zfar;
    uint32_t scissor_x,scissor_y,scissor_width,scissor_height;
} SweetRenderDraw;
typedef struct SweetRenderPass {
    SweetShaderTextureBinding target;
    uint32_t load_action,store_action;
    double clear[4];
} SweetRenderPass;
#define SWEET_METAL_SHADER_API __attribute__((visibility("default")))
SWEET_METAL_SHADER_API SweetMetalProgram *sweetgl_metal_compute_create(const char *source,uint32_t buffer_mask,
    uint32_t write_mask,char *error,size_t error_bytes);
SWEET_METAL_SHADER_API int sweetgl_metal_compute_dispatch(SweetMetalProgram *program,const SweetShaderBufferBinding *bindings,
    uint32_t count,const uint32_t groups[3],const uint32_t local[3],const uint32_t grid[3],uint32_t *sequence);
SWEET_METAL_SHADER_API void sweetgl_metal_program_destroy(SweetMetalProgram *program);
SWEET_METAL_SHADER_API SweetMetalProgram *sweetgl_metal_render_create(const char *vertex,const char *fragment,
    const SweetRenderPipelineConfig *config,char *error,size_t error_bytes);
SWEET_METAL_SHADER_API int sweetgl_metal_render_draw(SweetMetalProgram *program,const SweetRenderPass *pass,
    const SweetRenderDraw *draw,const SweetShaderBufferBinding *buffers,uint32_t buffer_count,
    const SweetShaderTextureBinding *textures,uint32_t texture_count,const SweetShaderSamplerBinding *samplers,
    uint32_t sampler_count,uint32_t *sequence);
SWEET_METAL_SHADER_API int sweetgl_metal_render_clear(const SweetRenderPass *pass,uint32_t *sequence);
#ifdef __cplusplus
}
#endif
