/* SPDX-License-Identifier: BSD-2-Clause
 * Independent byte contract: per-attachment Clear then a scissored MRT draw. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
static inline unsigned clear_cache_pixel_bytes(unsigned index,unsigned phase,unsigned x,unsigned y,unsigned channel)
{
    bool inside=x>=3 && x<14 && y>=2 && y<7,rgba=index&1;
    unsigned red=inside?((index*255+4)/8):((phase^(index&1))?255:0);
    unsigned green=inside?64:(phase?0:255),blue=inside?128:(phase?255:0);
    unsigned color[4]={rgba?red:blue,green,rgba?blue:red,255};return color[channel];
}
static inline uint64_t clear_cache_expected_hash(unsigned index,unsigned phase)
{
    uint64_t h=UINT64_C(14695981039346656037);
    for(unsigned at=0;at<16384;++at){unsigned value=0xa0+index;
        if(at>=64 && at<64+128*9){unsigned y=(at-64)/128,x=(at-64)%128;
            if(x<17*4)value=clear_cache_pixel_bytes(index,phase,x/4,y,x%4);}
        h=(h^value)*UINT64_C(1099511628211);
    }return h;
}
