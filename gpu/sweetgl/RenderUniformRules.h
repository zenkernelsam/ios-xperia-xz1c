/* SPDX-License-Identifier: BSD-2-Clause
 * The render sampler binder must distinguish AIR sampled textures from its own
 * typed uniforms/images. Unknown user uniforms are still rejected. Shared by
 * production and the real host GLES uniform-introspection regression test. */
#pragma once
#include "MetalShaderAPI.h"
#include <GLES3/gl31.h>
#include <stdio.h>
#include <string.h>
typedef enum SweetRenderUniformRole {
    SWEET_UNIFORM_REJECT=0,SWEET_UNIFORM_AIR_SAMPLER=1,SWEET_UNIFORM_OWNED=2,SWEET_UNIFORM_XR_IMAGE=3
} SweetRenderUniformRole;
static inline SweetRenderUniformRole sweet_render_uniform_role(const char *name,GLenum type,GLint elements,
    const SweetRenderPipelineConfig *config,uint32_t ordered_mask,unsigned *image_binding)
{
    if(image_binding)*image_binding=~0u;
    if(!name || !config || elements<1)return SWEET_UNIFORM_REJECT;
    if(!strcmp(name,"_smtl_xr_lut"))return type==GL_UNSIGNED_INT_SAMPLER_2D && elements==1?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;
    if(!strcmp(name,"_smtl_instance_base"))return ordered_mask && type==GL_INT && elements==1?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;
    if(!strcmp(name,"_smtl_blend_constant"))return ordered_mask && type==GL_FLOAT_VEC4 && elements==1?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;
    unsigned index=0;int used=0;
    if(sscanf(name,"_smtl_xr_destination%u%n",&index,&used)==1 && !name[used]){
        if(index>=8 || !(ordered_mask&(1u<<index)) || type!=GL_IMAGE_2D || elements!=1)return SWEET_UNIFORM_REJECT;
        if(image_binding)*image_binding=index;return SWEET_UNIFORM_XR_IMAGE;
    }
    used=0;
    if(sscanf(name,"_smtl_xr_written%u%n",&index,&used)==1 && !name[used]){
        if(index>=8 || !(ordered_mask&(1u<<index)) || type!=GL_IMAGE_2D || elements!=1)return SWEET_UNIFORM_REJECT;
        if(image_binding)*image_binding=8+index;return SWEET_UNIFORM_XR_IMAGE;
    }
    if(!strcmp(name,"_smtl_vertex_buffer_info[0]") || !strcmp(name,"_smtl_vertex_buffer_info"))
        return type==GL_UNSIGNED_INT_VEC2 && elements<=8?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;
    used=0;
    if(sscanf(name,"_smtl_vertex_buffer%u%n",&index,&used)==1 && !name[used])
        return index<8 && (config->vertex_buffers&(1u<<index)) && type==GL_UNSIGNED_INT_SAMPLER_2D && elements==1?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;
    const char *info[]={"_qtexture2d_info","_qvertex_texture2d_info","_qsampler_info","_qvertex_sampler_info"};
    for(unsigned i=0;i<4;++i){size_t n=strlen(info[i]);if(!strncmp(name,info[i],n) && (!name[n] || !strcmp(name+n,"[0]")))
        return type==(i<2?GL_UNSIGNED_INT_VEC4:GL_UNSIGNED_INT) && elements<=8?SWEET_UNIFORM_OWNED:SWEET_UNIFORM_REJECT;}
    return type==GL_SAMPLER_2D?SWEET_UNIFORM_AIR_SAMPLER:SWEET_UNIFORM_REJECT;
}
