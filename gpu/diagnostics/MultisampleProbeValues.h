/* SPDX-License-Identifier: BSD-2-Clause; authored exact original sample bytes. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
enum { MULTISAMPLE_PHASES=18, MULTISAMPLE_CHECKED_BYTES=151488, MULTISAMPLE_GUARD_BYTES=55098 };
static unsigned multisample_probe_samples(unsigned phase){return phase<7 || phase==14 || phase==15?2:4;}
static unsigned multisample_probe_state(unsigned phase)
{return phase<14?phase%7==0?0:phase%7==1?1:phase%7==6?3:2:phase&1?2:1;}
static unsigned multisample_probe_pitch(unsigned samples,unsigned slot)
{return slot>=10?128:slot==9?samples==2?64:128:samples==2?192:320;}
static unsigned multisample_probe_bytes(unsigned samples,unsigned slot)
{return 128+multisample_probe_pitch(samples,slot)*9;}
static unsigned multisample_probe_operations(unsigned phase)
{unsigned samples=multisample_probe_samples(phase);return phase>=14?samples+2+(phase&1):phase%7==1?samples:phase%7==3 || phase%7==4 || phase%7==5?2:phase%7==6?3:1;}
static void multisample_probe_rgba(unsigned sample,unsigned state,unsigned char value[4])
{
    static const unsigned char colors[4][4]={{32,64,96,255},{96,128,160,255},{160,192,224,255},{224,64,32,255}};
    const unsigned char changed[]={0,224,224,255},all[]={64,128,191,255},clear[]={255,0,255,255};
    memcpy(value,state==0?clear:state==3?all:state==2 && sample==0?changed:colors[sample],4);
}
static float multisample_probe_depth(unsigned sample,unsigned state)
{static const float values[4]={0.5f,0.125f,0.75f,0.25f};return state==0?0.75f:state==3?0.25f:state==2 && sample==0?0.375f:values[sample];}
static unsigned multisample_probe_stencil(unsigned sample,unsigned state)
{return state==0?17:state==3?77:state==2 && sample==0?90:40+sample;}
static unsigned multisample_probe_resolve_state(unsigned phase)
{return phase>=14?multisample_probe_state(phase):phase%7>=3?multisample_probe_state(phase):0;}
static unsigned multisample_probe_filter(unsigned phase)
{return phase<14 && phase%7==4?1:phase<14 && phase%7==5?2:0;}
static void multisample_probe_expected(unsigned phase,unsigned slot,unsigned char *bytes)
{
    unsigned samples=multisample_probe_samples(phase),pitch=multisample_probe_pitch(samples,slot),size=multisample_probe_bytes(samples,slot);
    unsigned marker=slot==0?0xa5:slot==8?0xb6:slot==9?0xc7:slot==10?0xd8:0xe9;
    memset(bytes,marker,size);unsigned state=multisample_probe_state(phase),resolved=multisample_probe_resolve_state(phase);
    if(slot>=10 && !resolved)return;
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){bool box=x>=3 && x<14 && y>=2 && y<7;
        if(slot<10){for(unsigned sample=0;sample<samples;++sample){unsigned at=64+y*pitch+x*(slot==9?1:4)*samples+sample*(slot==9?1:4);
            unsigned selected=box?state:0;
            if(slot==0){unsigned char c[4];multisample_probe_rgba(sample,selected,c);bytes[at]=c[2];bytes[at+1]=c[1];bytes[at+2]=c[0];bytes[at+3]=c[3];}
            else if(slot==8){float z=multisample_probe_depth(sample,selected);memcpy(bytes+at,&z,4);}
            else bytes[at]=(unsigned char)multisample_probe_stencil(sample,selected);}}
        else{unsigned at=64+y*pitch+x*4;unsigned selected=box?resolved:0;
            if(slot==10){unsigned total[4]={0,0,0,0};for(unsigned sample=0;sample<samples;++sample){unsigned char c[4];multisample_probe_rgba(sample,selected,c);for(unsigned i=0;i<4;++i)total[i]+=c[i];}
                bytes[at]=total[2]/samples;bytes[at+1]=total[1]/samples;bytes[at+2]=total[0]/samples;bytes[at+3]=total[3]/samples;}
            else{unsigned filter=multisample_probe_filter(phase);float z=multisample_probe_depth(0,selected);
                for(unsigned sample=1;sample<samples && filter;++sample){float value=multisample_probe_depth(sample,selected);z=filter==1?(z<value?z:value):(z>value?z:value);}
                memcpy(bytes+at,&z,4);}}
    }
}
static uint64_t multisample_probe_hash(const unsigned char *bytes,unsigned count)
{uint64_t hash=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)hash=(hash^bytes[i])*UINT64_C(1099511628211);return hash;}
