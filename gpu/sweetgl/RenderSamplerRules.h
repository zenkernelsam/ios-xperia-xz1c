/* SPDX-License-Identifier: BSD-2-Clause
 * Constant-sampler naming and state validation adapted from MC-XiaoXiao/
 * ios_sagit 673f85070fe9f7872793015eb230a5c3987d4a8c:
 * tools/gpu-compiler/Interface.h qgpu sampler remapping and
 * drivers/include/GPUShaderProtocol.h validSampler.
 * Original AIR values remain semantic state, never physical GL unit numbers.
 */
#pragma once
#include "MetalShaderAPI.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

typedef struct SweetRenderConstantSampler {
    uint32_t filters,min_lod,max_lod,address_modes,max_anisotropy,border_colour,normalized;
} SweetRenderConstantSampler;
typedef struct SweetRenderSamplerUse {
    unsigned texture,sampler,stage;
    bool literal,explicit_level;
    SweetRenderConstantSampler constant;
} SweetRenderSamplerUse;
typedef struct SweetRenderSamplerState {
    SweetShaderSamplerBinding binding;
    uint32_t max_anisotropy,border_colour;
} SweetRenderSamplerState;

static inline bool sweet_render_sampler_uint(const char **cursor,uint32_t *value)
{
    const char *p=*cursor;uint32_t n=0;
    if(*p<'0' || *p>'9')return false;
    do{uint32_t digit=(uint32_t)(*p-'0');if(n>(UINT32_MAX-digit)/10u)return false;
        n=n*10u+digit;++p;}while(*p>='0' && *p<='9');
    *cursor=p;*value=n;return true;
}
static inline bool sweet_render_constant_sampler_valid(const SweetRenderConstantSampler *s)
{
    // Plain sampled 2D AIR uses min/mag/mip fields, not shadow comparisons.
    if(!s || (s->filters&~15u) || ((s->filters>>2)&3u)>=3u || s->min_lod>s->max_lod || s->max_lod>4095u ||
        (s->address_modes&~511u) || !s->max_anisotropy || s->max_anisotropy>16u || s->border_colour>2u || s->normalized>1u)return false;
    for(unsigned axis=0;axis<3;++axis)if(((s->address_modes>>(axis*3))&7u)>5u)return false;
    if(!s->normalized && (s->address_modes || s->max_anisotropy!=1u || (s->filters&12u) ||
        (s->filters&1u)!=((s->filters>>1)&1u)))return false;
    unsigned a=s->address_modes&7u,b=(s->address_modes>>3)&7u;
    return !((a==4u || b==4u) && (a==5u || b==5u) && s->border_colour);
}
static inline bool sweet_render_sampler_parse(const char *name,SweetRenderSamplerUse *out)
{
    if(!name || !out)return false;
    SweetRenderSamplerUse use={0};const char *p=name;
    if(!strncmp(p,"_qsample",8)){use.stage=2;p+=8;}
    else if(!strncmp(p,"_qvertex_sample",15)){use.stage=1;p+=15;}
    else return false;
    uint32_t texture=0,sampler=0;
    if(!sweet_render_sampler_uint(&p,&texture) || *p++!='_' || !sweet_render_sampler_uint(&p,&sampler) || texture>=8u || sampler>=24u)return false;
    use.texture=texture;use.sampler=sampler;
    if(sampler>=8u){
        if(strncmp(p,"_c_",3))return false;p+=3;use.literal=true;
        uint32_t fields[7];for(unsigned i=0;i<7;++i){
            if(!sweet_render_sampler_uint(&p,&fields[i]))return false;
            if(i<6 && *p++!='_')return false;
        }
        use.constant=(SweetRenderConstantSampler){fields[0],fields[1],fields[2],fields[3],fields[4],fields[5],fields[6]};
        if(!sweet_render_constant_sampler_valid(&use.constant))return false;
    }
    if(*p){if(strcmp(p,"_level"))return false;use.explicit_level=true;}
    *out=use;return true;
}
static inline int sweet_render_sampler_resolve(const SweetRenderSamplerUse *use,
    const SweetShaderSamplerBinding *bindings,uint32_t count,SweetRenderSamplerState *out)
{
    if(!use || !out || count>16u || (count && !bindings))return -EINVAL;
    SweetRenderSamplerState state={0};state.max_anisotropy=1;
    if(use->literal){
        const SweetRenderConstantSampler *s=&use->constant;
        if(!sweet_render_constant_sampler_valid(s))return -EINVAL;
        state.binding=(SweetShaderSamplerBinding){use->sampler,use->stage,s->filters&1u,(s->filters>>1)&1u,(s->filters>>2)&3u,
            s->address_modes&7u,(s->address_modes>>3)&7u,s->normalized,(float)s->min_lod/256.0f,(float)s->max_lod/256.0f};
        state.max_anisotropy=s->max_anisotropy;state.border_colour=s->border_colour;
    }else{
        bool found=false;for(unsigned i=0;i<count;++i)if(bindings[i].index==use->sampler && bindings[i].stage==use->stage){
            if(found)return -EINVAL;state.binding=bindings[i];found=true;
        }
        if(!found)return -EINVAL;
    }
    const SweetShaderSamplerBinding *s=&state.binding;
    if(s->min_filter>1u || s->mag_filter>1u || s->mip_filter>2u || s->address_s>5u || s->address_t>5u || s->normalized>1u ||
        !isfinite(s->minimum_lod) || !isfinite(s->maximum_lod) || s->minimum_lod>s->maximum_lod)return -EINVAL;
    // GLES sampled 2D coordinates are normalized. Preserve the refusal until
    // an explicit unnormalized lowering exists, rather than changing AIR meaning.
    if(!s->normalized)return -ENOTSUP;
    *out=state;return 0;
}
static inline bool sweet_render_sampler_unit(unsigned ordinal,unsigned available,unsigned *unit)
{
    // 8..15 carry vertex-buffer words and 16 carries the XR lookup table.
    unsigned value=ordinal<8u?ordinal:ordinal+9u;
    if(!unit || ordinal>=23u || value>=available)return false;
    *unit=value;return true;
}
static inline GLenum sweet_render_sampler_address(uint32_t mode)
{
    if(mode<=1u)return GL_CLAMP_TO_EDGE;
    if(mode==2u)return GL_REPEAT;if(mode==3u)return GL_MIRRORED_REPEAT;
    if(mode<=5u)return GL_CLAMP_TO_BORDER_EXT;return 0;
}
static inline int sweet_render_sampler_parameters(const SweetRenderSamplerState *state,
    bool border_clamp,bool anisotropic,float maximum_anisotropy)
{
    if(!state)return -EINVAL;const SweetShaderSamplerBinding *s=&state->binding;
    bool border=s->address_s>=4u || s->address_t>=4u;
    if((border && !border_clamp) || (state->max_anisotropy>1u && (!anisotropic || state->max_anisotropy>maximum_anisotropy)))return -ENOTSUP;
    GLenum min=s->min_filter?GL_LINEAR:GL_NEAREST;
    if(s->mip_filter==1u)min=s->min_filter?GL_LINEAR_MIPMAP_NEAREST:GL_NEAREST_MIPMAP_NEAREST;
    if(s->mip_filter==2u)min=s->min_filter?GL_LINEAR_MIPMAP_LINEAR:GL_NEAREST_MIPMAP_LINEAR;
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,min);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,s->mag_filter?GL_LINEAR:GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,sweet_render_sampler_address(s->address_s));
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,sweet_render_sampler_address(s->address_t));
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_LOD,s->minimum_lod);glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_LOD,s->maximum_lod);
    if(border){unsigned colour=(s->address_s==4u || s->address_t==4u)?0u:state->border_colour;
        GLfloat rgba[4]={colour==2u?1.0f:0.0f,colour==2u?1.0f:0.0f,colour==2u?1.0f:0.0f,colour?1.0f:0.0f};
        glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR_EXT,rgba);}
    if(anisotropic)glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,(GLfloat)state->max_anisotropy);
    return glGetError()==GL_NO_ERROR?0:-EIO;
}
