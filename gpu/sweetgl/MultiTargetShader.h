/* SPDX-License-Identifier: BSD-2-Clause
 * GPU-only writeback to the original XR integer image. The original code
 * snapshot preserves undrawn pixels and masked/unmodified components exactly. */
#pragma once
#include "XRColourShader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline char *sweet_xr_writeback_source(unsigned format)
{
    if(format<552 || format>555)return NULL;
    const char *update=format<554?
        "if((channel_mask&8u)!=0u && floatBitsToUint(value.r)!=floatBitsToUint(before.r))out_code.z=encoded.z;"
        "if((channel_mask&4u)!=0u && floatBitsToUint(value.g)!=floatBitsToUint(before.g))out_code.y=encoded.y;"
        "if((channel_mask&2u)!=0u && floatBitsToUint(value.b)!=floatBitsToUint(before.b))out_code.x=encoded.x;"
        "if((channel_mask&1u)!=0u && floatBitsToUint(value.a)!=floatBitsToUint(before.a))out_code.w=encoded.w;":
        "bool changed=false;"
        "if((channel_mask&8u)!=0u && floatBitsToUint(value.r)!=floatBitsToUint(before.r)){out_code.x=(out_code.x&~1072693248u)|(encoded.x&1072693248u);changed=true;}"
        "if((channel_mask&4u)!=0u && floatBitsToUint(value.g)!=floatBitsToUint(before.g)){out_code.x=(out_code.x&~1047552u)|(encoded.x&1047552u);changed=true;}"
        "if((channel_mask&2u)!=0u && floatBitsToUint(value.b)!=floatBitsToUint(before.b)){out_code.x=(out_code.x&~1023u)|(encoded.x&1023u);changed=true;}"
        "if(changed)out_code.x|=3221225472u;";
    size_t size=strlen(sweet_xr_shader)+strlen(update)+1024;char *source=(char *)malloc(size);if(!source)return NULL;
    snprintf(source,size,"#version 310 es\nprecision highp float;precision highp int;\n%s\n"
        "uniform highp sampler2D float_image;uniform highp usampler2D original_image;uniform uint channel_mask;"
        "layout(location=0) out highp uvec4 out_code;void main(){ivec2 p=ivec2(gl_FragCoord.xy);"
        "uvec4 old=texelFetch(original_image,p,0);vec4 before=_smtl_xr_unpack(old,%uu);"
        "vec4 value=texelFetch(float_image,p,0);uvec4 encoded=_smtl_xr_pack(value,%uu);out_code=old;%s}\n",
        sweet_xr_shader,format,format,update);return source;
}
