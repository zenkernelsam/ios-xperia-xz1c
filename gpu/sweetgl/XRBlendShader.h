/* SPDX-License-Identifier: BSD-2-Clause
 * A5xx has float32 render/image support but the pinned screen does not advertise
 * PIPE_BIND_BLENDABLE. Keep the XR image in float32 and blend in the fragment
 * shader. The caller submits ONE primitive and ONE instance, with an image
 * barrier before the next primitive; no framebuffer fetch/interlock is assumed.
 * XR image slots are detached from the framebuffer to avoid a feedback loop.
 */
#pragma once
#include "MetalShaderAPI.h"
#include "XRColourShader.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint32_t sweet_xr_ordered_mask(const SweetRenderPipelineConfig *config)
{
    uint32_t mask=0;
    for(unsigned i=0;i<8;++i)if(sweet_render_color_mask(config)&(1u<<i)){
        SweetRenderColorConfig c=sweet_render_color(config,i);
        if(c.format>=552 && c.format<=555 && c.blend_enabled)mask|=1u<<i;
    }
    return mask;
}
static inline bool sweet_shader_replace(char *source,size_t capacity,const char *before,const char *after)
{
    char *at=strstr(source,before);if(!at || strstr(at+strlen(before),before))return false;
    size_t old=strlen(before),next=strlen(after),length=strlen(source);
    if(length-old+next>=capacity)return false;
    memmove(at+next,at+old,strlen(at+old)+1);memcpy(at,after,next);return true;
}
static inline char *sweet_xr_ordered_vertex(const char *source)
{
    const char *id="gl_InstanceID",*replacement="(gl_InstanceID+_smtl_instance_base)";
    size_t occurrences=0;for(const char *p=source;(p=strstr(p,id));p+=strlen(id))++occurrences;
    size_t bytes=strlen(source)+occurrences*(strlen(replacement)-strlen(id))+80;
    char *result=(char *)malloc(bytes);if(!result)return NULL;
    const char *line=strchr(source,'\n');if(!line){free(result);return NULL;}
    size_t head=(size_t)(line+1-source);memcpy(result,source,head);result[head]=0;
    strcat(result,"uniform highp int _smtl_instance_base;\n");
    const char *start=line+1;char *out=result+strlen(result);
    for(const char *p;(p=strstr(start,id));start=p+strlen(id)){
        size_t n=(size_t)(p-start);memcpy(out,start,n);out+=n;memcpy(out,replacement,strlen(replacement));out+=strlen(replacement);
    }
    strcpy(out,start);return result;
}
static const char sweet_xr_blend_functions[]=
    "vec4 _smtl_blend_factor(uint f,vec4 s,vec4 d,vec4 k){"
    "if(f==0u)return vec4(0);if(f==1u)return vec4(1);if(f==2u)return s;if(f==3u)return vec4(1)-s;"
    "if(f==4u)return vec4(s.a);if(f==5u)return vec4(1.0-s.a);if(f==6u)return d;if(f==7u)return vec4(1)-d;"
    "if(f==8u)return vec4(d.a);if(f==9u)return vec4(1.0-d.a);if(f==10u)return vec4(vec3(min(s.a,1.0-d.a)),1);"
    "if(f==11u)return k;if(f==12u)return vec4(1)-k;if(f==13u)return vec4(k.a);return vec4(1.0-k.a);}\n"
    "vec4 _smtl_blend_op(uint op,vec4 s,vec4 d,vec4 sf,vec4 df){"
    "if(op==3u)return min(s,d);if(op==4u)return max(s,d);"
    "vec4 a=s*sf,b=d*df;return op==1u?a-b:op==2u?b-a:a+b;}\n";
static inline char *sweet_xr_ordered_fragment(const char *source,const SweetRenderPipelineConfig *config,uint32_t mask)
{
    size_t bytes=strlen(source)+strlen(sweet_xr_shader)+strlen(sweet_xr_blend_functions)+16384;
    char *result=(char *)malloc(bytes);if(!result)return NULL;strcpy(result,source);
    if(!sweet_shader_replace(result,bytes,"void main()","void _smtl_original_main()"))goto failure;
    for(unsigned i=0;i<8;++i)if(mask&(1u<<i)){
        char before[128],after[64],name[32];snprintf(name,sizeof(name),i?"_qcolor%u":"_qcolor",i);
        snprintf(before,sizeof(before),"layout(location=%u) out vec4 %s;",i,name);snprintf(after,sizeof(after),"vec4 %s;",name);
        if(!sweet_shader_replace(result,bytes,before,after))goto failure;
    }
    strcat(result,"\nuniform highp vec4 _smtl_blend_constant;\n");strcat(result,sweet_xr_shader);strcat(result,sweet_xr_blend_functions);
    for(unsigned i=0;i<8;++i)if(mask&(1u<<i)){
        char declaration[320];snprintf(declaration,sizeof(declaration),
            "layout(rgba32f,binding=%u) uniform readonly coherent highp image2D _smtl_xr_destination%u;\n"
            "layout(rgba32f,binding=%u) uniform writeonly coherent highp image2D _smtl_xr_written%u;\n",i,i,8+i,i);strcat(result,declaration);
    }
    strcat(result,"void main(){_smtl_original_main();ivec2 p=ivec2(gl_FragCoord.xy);\n");
    for(unsigned i=0;i<8;++i)if(mask&(1u<<i)){
        SweetRenderColorConfig c=sweet_render_color(config,i);char name[32],body[2048];snprintf(name,sizeof(name),i?"_qcolor%u":"_qcolor",i);
        snprintf(body,sizeof(body),"{vec4 s=%s,d=imageLoad(_smtl_xr_destination%u,p),k=_smtl_blend_constant;"
            "vec4 rgb=_smtl_blend_op(%uu,s,d,_smtl_blend_factor(%uu,s,d,k),_smtl_blend_factor(%uu,s,d,k));"
            "vec4 alpha=_smtl_blend_op(%uu,s,d,_smtl_blend_factor(%uu,s,d,k),_smtl_blend_factor(%uu,s,d,k));"
            "vec4 value=vec4(rgb.rgb,alpha.a);"
            "vec4 rounded=_smtl_xr_unpack(_smtl_xr_pack(value,%uu),%uu);"
            "value=mix(d,rounded,bvec4(%s,%s,%s,%s));imageStore(_smtl_xr_written%u,p,value);}\n",
            name,i,c.operation_rgb,c.source_rgb,c.destination_rgb,c.operation_alpha,c.source_alpha,c.destination_alpha,c.format,c.format,
            c.write_mask&8?"true":"false",c.write_mask&4?"true":"false",c.write_mask&2?"true":"false",c.write_mask&1?"true":"false",i);
        strcat(result,body);
    }
    strcat(result,"}\n");return result;
failure:free(result);return NULL;
}
typedef struct SweetOrderedPrimitive {uint32_t first,count,mode;bool reverse_winding;} SweetOrderedPrimitive;
static inline uint32_t sweet_ordered_primitive_count(uint32_t mode,uint32_t vertices)
{return mode==0?vertices:mode==1?vertices/2:mode==2?(vertices>1?vertices-1:0):mode==3?vertices/3:mode==4?(vertices>2?vertices-2:0):0;}
static inline SweetOrderedPrimitive sweet_ordered_primitive(uint32_t mode,uint32_t first,uint32_t index)
{
    SweetOrderedPrimitive p={first,1,mode,false};
    if(mode==0)p.first+=index;
    else if(mode==1){p.first+=index*2;p.count=2;}
    else if(mode==2){p.first+=index;p.count=2;p.mode=1;}
    else if(mode==3){p.first+=index*3;p.count=3;}
    else if(mode==4){p.first+=index;p.count=3;p.reverse_winding=(index&1)!=0;}
    return p;
}
