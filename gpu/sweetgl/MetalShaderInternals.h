/* Internal only: shared private contexts over the one tested Mesa screen. */
#pragma once
#include "MetalShaderAPI.h"
#include <GLES3/gl31.h>
#include <pthread.h>
struct sweetgl_ctx;
struct st_context;
typedef struct SweetMetalShaderCache {GLuint copy[5],decode[4],lut;} SweetMetalShaderCache;
static inline void sweet_metal_shader_cache_dispose(SweetMetalShaderCache *cache)
{
    for(unsigned i=0;i<5;++i){if(cache->copy[i])glDeleteProgram(cache->copy[i]);cache->copy[i]=0;}
    for(unsigned i=0;i<4;++i){if(cache->decode[i])glDeleteProgram(cache->decode[i]);cache->decode[i]=0;}
    if(cache->lut)glDeleteTextures(1,&cache->lut);cache->lut=0;
}
struct SweetMetalProgram {
    struct sweetgl_ctx *context;
    GLuint program;
    uint32_t buffer_mask,write_mask;
    SweetRenderPipelineConfig render;
    SweetMetalShaderCache cache;
};
extern pthread_mutex_t sweet_metal_shader_gate;
struct sweetgl_ctx *sweetgl_create_metal(void);
struct st_context *sweetgl_internal_context(struct sweetgl_ctx *);
void sweetgl_internal_restore(struct st_context *);
int sweet_metal_bind_buffer(struct st_context *,const SweetShaderBufferBinding *,GLuint *);
