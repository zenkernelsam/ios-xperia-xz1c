/* Internal only: shared private contexts over the one tested Mesa screen. */
#pragma once
#include "MetalShaderAPI.h"
#include <GLES3/gl31.h>
#include <pthread.h>
struct sweetgl_ctx;
struct st_context;
struct SweetMetalProgram {
    struct sweetgl_ctx *context;
    GLuint program;
    uint32_t buffer_mask,write_mask;
    SweetRenderPipelineConfig render;
};
extern pthread_mutex_t sweet_metal_shader_gate;
struct sweetgl_ctx *sweetgl_create_metal(void);
struct st_context *sweetgl_internal_context(struct sweetgl_ctx *);
void sweetgl_internal_restore(struct st_context *);
int sweet_metal_bind_buffer(struct st_context *,const SweetShaderBufferBinding *,GLuint *);
