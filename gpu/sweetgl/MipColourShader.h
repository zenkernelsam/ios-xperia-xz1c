/* SPDX-License-Identifier: BSD-2-Clause
 * Pixel-centre linear resampling. sRGB/XR inputs are decoded before filtering. */
#pragma once
static const char sweet_mip_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_mip_fragment[]="#version 310 es\nprecision highp float;uniform highp sampler2D source_image;uniform vec2 destination_extent;layout(location=0) out vec4 colour;void main(){colour=textureLod(source_image,gl_FragCoord.xy/destination_extent,0.0);}";
