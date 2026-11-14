/* SPDX-License-Identifier: BSD-2-Clause
 * Authored packed integer source values and independent scalar expected results. */
#pragma once
#include <stdint.h>
#include <string.h>
static uint16_t narrow_short(unsigned index,unsigned component,unsigned phase)
{static const uint16_t values[]={0,1,32767,32768,65535,12345,50000,65280};return values[(index*3+component*5+phase*2)&7];}
static uint8_t narrow_byte(unsigned index,unsigned component,unsigned phase)
{static const uint8_t values[]={0,1,127,128,255,39,192,240};return values[(index*5+component*3+phase)&7];}
static void narrow_source(unsigned char *shorts,unsigned char *bytes,unsigned phase)
{for(unsigned i=0;i<64;++i)for(unsigned c=0;c<2;++c){uint16_t value=narrow_short(i,c,phase);memcpy(shorts+i*4+c*2,&value,2);bytes[i*2+c]=narrow_byte(i,c,phase);}}
static void narrow_result(unsigned index,unsigned phase,unsigned char result[48])
{
    uint16_t a=narrow_short(index,0,phase),b=narrow_short(index,1,phase);uint8_t c=narrow_byte(index,0,phase),d=narrow_byte(index,1,phase);
    int sa=a>=32768?(int)a-65536:a,sb=b>=32768?(int)b-65536:b,sc=c>=128?(int)c-256:c,sd=d>=128?(int)d-256:d;
    float expected[12]={(float)sa,(float)a,(float)sd,(float)d,(float)sa,(float)sb,(float)a,(float)b,(float)sc,(float)sd,(float)c,(float)d};
    memcpy(result,expected,48);
}
static uint64_t narrow_hash(const unsigned char *p,unsigned n)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<n;++i)h=(h^p[i])*UINT64_C(1099511628211);return h;}
static unsigned narrow_check(const unsigned char *output,const unsigned char *shorts,const unsigned char *bytes,unsigned phase)
{
    unsigned mismatch=0;unsigned char source16[256],source8[128];narrow_source(source16,source8,phase);
    for(unsigned i=0;i<16384;++i){unsigned char out=0xa5;if(i>=64 && i<3136){unsigned char expected[48];narrow_result((i-64)/48,phase,expected);out=expected[(i-64)%48];}
        mismatch+=output[i]!=out;mismatch+=shorts[i]!=(i>=64 && i<320?source16[i-64]:0xb6);mismatch+=bytes[i]!=(i>=64 && i<192?source8[i-64]:0xc7);
    }return mismatch;
}
