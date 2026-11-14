/* SPDX-License-Identifier: BSD-2-Clause
 * Independent CPU reference uses double arithmetic and nearest-even binary16. */
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
static double half_probe_value(uint16_t bits)
{unsigned e=(bits>>10)&31,m=bits&1023;double value=e==31?(m?NAN:INFINITY):e?ldexp(1+m/1024.0,(int)e-15):ldexp((double)m,-24);return bits&32768?-value:value;}
static uint16_t half_probe_pack(double value)
{
    unsigned sign=signbit(value)?32768:0;if(isnan(value))return sign|32256;if(isinf(value))return sign|31744;
    double a=fabs(value);if(!a)return sign;int exp;frexp(a,&exp);--exp;
    double scaled;unsigned base;if(exp<-14){scaled=a*16777216.0;base=0;}else{if(exp>15)return sign|31744;scaled=(ldexp(a,-exp)-1)*1024;base=(exp+15)*1024;}
    double whole=floor(scaled),part=scaled-whole;unsigned result=(unsigned)whole+(part>.5 || (part==.5 && ((unsigned)whole&1)));return sign|(base+result);
}
static uint16_t half_probe_input(unsigned i,unsigned c,unsigned phase)
{if(!i && !c)return 0x7e01;static const uint16_t values[]={0x0000,0x8000,0x0001,0x8001,0x03ff,0x0400,0x3c00,0x3c01,0x3555,0x7bfe,0x7bff,0x0800,0x8400,0x7a00,0xfa00,0x3bff};return values[(i*3+c*5+phase*7)&15];}
static float half_probe_float(unsigned i,unsigned phase)
{if(!i)return INFINITY;static const float values[]={INFINITY,-INFINITY,1.00048828125f,1.00146484375f,65519.0f,65520.0f,-1.00048828125f,0x1p-25f,0x1.8p-24f,-0x1p-25f,.3333f,-.3333f,0x1.ff8p-15f,2.001f,100.01f,-2.001f};return values[(i*5+phase*3)&15];}
static void half_probe_sources(unsigned char *halfbytes,unsigned char *full,unsigned phase)
{for(unsigned i=0;i<64;++i){for(unsigned c=0;c<4;++c){uint16_t h=half_probe_input(i,c,phase);memcpy(halfbytes+i*8+c*2,&h,2);}float f=half_probe_float(i,phase);memcpy(full+i*4,&f,4);}}
static void half_probe_result(unsigned i,unsigned phase,uint16_t result[12])
{
    const uint16_t bias[]={0x1000,0x9000,0x3555,0xb400},scale[]={0x3800,0x4000,0x3e00,0x3c00};
    for(unsigned c=0;c<4;++c){uint16_t h=half_probe_input(i,c,phase);result[c]=h;uint16_t sum=half_probe_pack(half_probe_value(h)+half_probe_value(bias[c]));result[4+c]=half_probe_pack(half_probe_value(sum)*half_probe_value(scale[c]));}
    uint16_t narrow=half_probe_pack(half_probe_float(i,phase));result[8]=narrow;
    result[9]=half_probe_pack(half_probe_value(narrow)+half_probe_value(result[0]));
    result[10]=half_probe_pack(half_probe_value(narrow)*half_probe_value(result[1]));result[11]=half_probe_pack(half_probe_value(result[2])/2.0);
}
static uint64_t half_probe_hash(const unsigned char *p,unsigned n)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<n;++i)h=(h^p[i])*UINT64_C(1099511628211);return h;}
static uint64_t half_probe_normalized_hash(const unsigned char *p,unsigned *nan_words)
{
    *nan_words=0;uint64_t hash=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<16384;++i){unsigned char value=p[i];
        if(i>=64 && i<1600){uint16_t word;memcpy(&word,p+(i&~1u),2);
            if((word&31744)==31744 && (word&1023)){value=(i&1)?0x7e:0;if(!(i&1))++*nan_words;}}
        hash=(hash^value)*UINT64_C(1099511628211);
    }return hash;
}
static unsigned half_probe_check(const unsigned char *output,const unsigned char *input,const unsigned char *full,unsigned phase)
{
    unsigned mismatch=0;unsigned char original[512],f32[256];half_probe_sources(original,f32,phase);
    for(unsigned i=0;i<16384;++i){unsigned char expected=0xa5;if(i>=64 && i<1600){uint16_t values[12];half_probe_result((i-64)/24,phase,values);expected=((unsigned char *)values)[(i-64)%24];}
        // Binary16 NaNs retain their class; payloads may be quieted by the GPU.
        if(i>=64 && i<1600){unsigned word=(i-64)/2;uint16_t expectedWords[12],actual;half_probe_result(word/12,phase,expectedWords);memcpy(&actual,output+64+word*2,2);
            if((expectedWords[word%12]&31744)==31744 && (expectedWords[word%12]&1023)){if(!(i&1))mismatch+=(actual&31744)!=31744 || !(actual&1023);}
            else mismatch+=output[i]!=expected;
        }else mismatch+=output[i]!=expected;
        mismatch+=input[i]!=(i>=64 && i<576?original[i-64]:0xb6);mismatch+=full[i]!=(i>=64 && i<320?f32[i-64]:0xc7);
    }return mismatch;
}
