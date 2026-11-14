// SPDX-License-Identifier: BSD-2-Clause
// Linear 2D storage on the tested64B FD540 texture alignment.
#pragma once
#include <stdint.h>
#include <stddef.h>
struct SweetTextureLayout {uint32_t width=0,height=0,pitch=0,bytes_per_pixel=0;uint64_t offset=0,bytes=0;};
inline uint32_t sweet_texture_pixel_bytes(uint32_t format)
{
    switch(format){case 10:return 1;case 30:return 2;case 70:case 71:case 80:case 81:case 554:case 555:return 4;
        case 115:case 552:case 553:return 8;default:return 0;}
}
inline bool sweet_texture_layout(uint64_t width,uint64_t height,uint32_t format,uint64_t offset,uint64_t pitch,
    uint64_t capacity,SweetTextureLayout &result)
{
    result={};uint32_t pixel=sweet_texture_pixel_bytes(format);
    if(!pixel || !width || !height || width>16384 || height>16384 || (offset&63) || pitch<width*pixel ||
        (pitch&63) || pitch>UINT32_MAX || height>(UINT64_C(256)<<20)/pitch)return false;
    uint64_t bytes=pitch*height;if(offset>capacity || bytes>capacity-offset)return false;
    result={(uint32_t)width,(uint32_t)height,(uint32_t)pitch,pixel,offset,bytes};return true;
}
inline bool sweet_texture_region(const SweetTextureLayout &layout,uint64_t x,uint64_t y,uint64_t width,uint64_t height,
    uint64_t row_bytes,uint64_t &offset,uint64_t &source_bytes)
{
    offset=source_bytes=0;
    if(x>layout.width || width>layout.width-x || y>layout.height || height>layout.height-y)return false;
    if(!width || !height)return true;
    uint64_t bytes=width*layout.bytes_per_pixel;if(row_bytes<bytes || (height-1 && row_bytes>(UINT64_MAX-bytes)/(height-1)))return false;
    offset=layout.offset+y*layout.pitch+x*layout.bytes_per_pixel;source_bytes=(height-1)*row_bytes+bytes;return true;
}
