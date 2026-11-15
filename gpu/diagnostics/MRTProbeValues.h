/* SPDX-License-Identifier: BSD-2-Clause
 * Independent raw-byte contract for dual output, blend masks and XR codes. */
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../sweetgl/XRColourTables.h"
static unsigned mrt_code(float value,bool srgb)
{
    unsigned n;if(srgb){unsigned fixed=(unsigned)nearbyint((double)fabsf(value)*8192.0);n=0;while(n+1<640 && sweet_xr_srgb_encode_first[n+1]<=fixed)++n;}
    else{double exact=(double)fabsf(value)*510.0,whole=floor(exact),part=exact-whole;n=(unsigned)whole+(part>.5 || (part==.5 && ((unsigned)whole&1)));}
    return value<0?384-(n<384?n:384):(384+n<1023?384+n:1023);
}
static float mrt_decode(unsigned code,bool srgb)
{return srgb?(code<384?-1:1)*(float)sweet_xr_srgb_decode_steps[code<384?384-code:code-384]/8192.0f:(float)((int)code-384)/510.0f;}
static unsigned mrt_low(unsigned x,unsigned y,unsigned c){return (x*3+y*5+c*7)&63;}
static void mrt_xr_pixel(unsigned format,unsigned phase,unsigned x,unsigned y,unsigned char pixel[8])
{
    bool region=phase && x>=3 && x<14 && y>=2 && y<7,srgb=format&1;unsigned r=384,g=384,b=894;
    if(region){r=mrt_code(.125f,srgb);g=mrt_code(.25f,srgb);b=mrt_code(.875f,srgb);
        if(phase>=2){r=mrt_code(mrt_decode(r,srgb)+.25f,srgb);b=mrt_code(mrt_decode(b,srgb)+.0625f,srgb);}}
    if(format<554){uint16_t words[4]={(uint16_t)(b<<6),(uint16_t)(g<<6),(uint16_t)(r<<6),894<<6};
        if(phase){for(unsigned c=0;c<4;++c)if(!region || c==3)words[c]|=mrt_low(x,y,c);}memcpy(pixel,words,8);}
    else{uint32_t word=(r<<20)|(g<<10)|b;if(!phase || region)word|=0xc0000000u;memcpy(pixel,&word,4);}
}
static void mrt_bgra_pixel(unsigned phase,unsigned x,unsigned y,unsigned char pixel[4])
{bool region=phase && x>=3 && x<14 && y>=2 && y<7;pixel[0]=region && phase>=2?255:0;pixel[1]=region?(phase>=2?191:64):0;pixel[2]=255;pixel[3]=255;}
static void mrt_sample_pixel(unsigned format,bool rgba,unsigned char pixel[4])
{
    bool srgb=format&1;unsigned r=mrt_code(.125f,srgb),b=mrt_code(.875f,srgb),g=mrt_code(.25f,srgb);
    r=mrt_code(mrt_decode(r,srgb)+.25f,srgb);b=mrt_code(mrt_decode(b,srgb)+.0625f,srgb);
    unsigned rr=(unsigned)floorf(fminf(fmaxf(mrt_decode(r,srgb),0),1)*255+.5f),gg=(unsigned)floorf(fminf(fmaxf(mrt_decode(g,srgb),0),1)*255+.5f),bb=(unsigned)floorf(fminf(fmaxf(mrt_decode(b,srgb),0),1)*255+.5f);
    pixel[0]=rgba?rr:bb;pixel[1]=gg;pixel[2]=rgba?bb:rr;pixel[3]=255;
}
static uint64_t mrt_hash(const unsigned char *p,unsigned n)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<n;++i)h=(h^p[i])*UINT64_C(1099511628211);return h;}
static unsigned mrt_check(const unsigned char *raw,unsigned format,unsigned phase,unsigned index,bool sample)
{
    unsigned offset=index || sample?128:64,pitch=!index && !sample && format<554?192:128,pixel=!index && !sample && format<554?8:4,mismatch=0;
    for(unsigned i=0;i<16384;++i){unsigned char wanted=index || sample?0xb6:0xa5;
        if(i>=offset && i<offset+pitch*9){unsigned y=(i-offset)/pitch,x=(i-offset)%pitch;if(x<17*pixel){unsigned char color[8];
                if(sample)mrt_sample_pixel(format,index!=0,color);else if(index)mrt_bgra_pixel(phase,x/4,y,color);else mrt_xr_pixel(format,phase,x/pixel,y,color);wanted=color[x%pixel];}}
        mismatch+=raw[i]!=wanted;
    }return mismatch;
}
static void mrt_seed_old_codes(unsigned char *raw,unsigned format)
{for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){unsigned char pixel[8];mrt_xr_pixel(format,1,0,0,pixel);
        // Seed valid old blue pixels with noncanonical padding bits. Later GPU
        // draws must preserve those bits wherever color/channels do not change.
        if(format<554){uint16_t words[4]={894<<6,384<<6,384<<6,894<<6};for(unsigned c=0;c<4;++c)words[c]|=mrt_low(x,y,c);memcpy(pixel,words,8);}
        else{uint32_t word=(384u<<20)|(384u<<10)|894;memcpy(pixel,&word,4);}memcpy(raw+64+y*(format<554?192:128)+x*(format<554?8:4),pixel,format<554?8:4);}}
static unsigned mrt_eight_check(const unsigned char *raw,unsigned index)
{
    unsigned mismatch=0;bool rgba=index&1;unsigned r=(unsigned)floor(index*.125*255+.5);unsigned char color[4]={(unsigned char)(rgba?r:128),64,(unsigned char)(rgba?128:r),255};
    for(unsigned i=0;i<16384;++i){unsigned char wanted=(unsigned char)(0xa0+index);if(i>=64 && i<64+1152){unsigned c=(i-64)%128;if(c<68)wanted=color[c%4];}mismatch+=raw[i]!=wanted;}return mismatch;
}
static void mrt_order_codes(unsigned format,unsigned codes[3])
{
    bool srgb=format&1;codes[0]=384;codes[1]=384;codes[2]=894;
    const float reds[4]={.25f,.375f,.5f,.625f};
    for(unsigned draw=0;draw<4;++draw){
        codes[0]=mrt_code(reds[draw]*.5f+mrt_decode(codes[0],srgb)*.5f,srgb);
        codes[1]=mrt_code(-.125f+mrt_decode(codes[1],srgb)*.5f,srgb);
        codes[2]=mrt_code(.5625f+mrt_decode(codes[2],srgb)*.5f,srgb);
    }
}
static void mrt_order_pixel(unsigned format,unsigned x,unsigned y,unsigned char pixel[8])
{
    unsigned codes[3];mrt_order_codes(format,codes);bool active=x>=3 && x<14 && y>=2 && y<7;
    if(!active){codes[0]=384;codes[1]=384;codes[2]=894;}
    if(format<554){uint16_t words[4]={(uint16_t)(codes[2]<<6),(uint16_t)(codes[1]<<6),(uint16_t)(codes[0]<<6),894<<6};
        for(unsigned c=0;c<4;++c)if(!active || c==3)words[c]|=mrt_low(x,y,c);memcpy(pixel,words,8);}
    else{uint32_t word=(codes[0]<<20)|(codes[1]<<10)|codes[2]|(active?0xc0000000u:0);memcpy(pixel,&word,4);}
}
static unsigned mrt_order_check(const unsigned char *raw,unsigned format)
{
    unsigned mismatch=0,pixel=format<554?8:4,pitch=format<554?192:128;
    for(unsigned i=0;i<16384;++i){unsigned char wanted=0xa5;
        if(i>=64 && i<64+pitch*9){unsigned y=(i-64)/pitch,x=(i-64)%pitch;if(x<17*pixel){unsigned char stored[8];mrt_order_pixel(format,x/pixel,y,stored);wanted=stored[x%pixel];}}
        mismatch+=raw[i]!=wanted;
    }return mismatch;
}
