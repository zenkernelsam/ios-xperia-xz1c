/* SPDX-License-Identifier: BSD-2-Clause
 * The fixed Mesa A5xx linear layout stores samples per pixel (cpp*=samples).
 * Sysmem rendering keeps caller stride/offset and logical sample bytes. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
static inline bool sweet_multisample_count(uint32_t samples)
{return samples==1 || samples==2 || samples==4;}
static inline bool sweet_multisample_layout(uint32_t width,uint32_t height,uint32_t pixel,uint32_t samples,
    uint32_t pitch,uint64_t offset,uint64_t capacity)
{
    if(!sweet_multisample_count(samples) || !width || !height || width>16384 || height>16384 ||
        (pixel!=1 && pixel!=2 && pixel!=4 && pixel!=8) || (pitch&63) || (offset&63) ||
        (uint64_t)width*pixel*samples>pitch || offset>capacity)return false;
    return (uint64_t)pitch*height<=capacity-offset;
}
static inline bool sweet_multisample_footprint(uint32_t width,uint32_t height,uint32_t pixel,uint32_t samples,
    uint64_t offset,uint64_t capacity,uint32_t *pitch,uint64_t *bytes)
{
    if(!pitch || !bytes || !sweet_multisample_count(samples) || (samples!=2 && samples!=4) || width>16384)return false;
    uint64_t row=((uint64_t)width*pixel*samples+63)&~UINT64_C(63);
    if(row>UINT32_MAX || !sweet_multisample_layout(width,height,pixel,samples,(uint32_t)row,offset,capacity))return false;
    *pitch=(uint32_t)row;*bytes=row*height;return true;
}
