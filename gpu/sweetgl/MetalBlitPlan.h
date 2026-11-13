/* A5xx packet fields follow Mesa adreno_pm4.xml; no register programming. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define SWEET_METAL_BLIT_CHUNK 16384u
static inline uint32_t smb_packet7(uint32_t opcode,uint32_t count)
{
    uint32_t a=opcode,b=count;
    a^=a>>16;a^=a>>8;a^=a>>4;a^=a>>2;a^=a>>1;
    b^=b>>16;b^=b>>8;b^=b>>4;b^=b>>2;b^=b>>1;
    return 0x70000000u|(opcode<<16)|count|((1u^(a&1u))<<23)|((1u^(b&1u))<<15);
}
static inline bool smb_range(uint64_t capacity,uint64_t offset,uint64_t bytes)
{return bytes && offset<=capacity && bytes<=capacity-offset && !(offset&3) && !(bytes&3);}
static inline size_t smb_words(uint64_t source,uint64_t bytes)
{
    if(!bytes || bytes>SWEET_METAL_BLIT_CHUNK || (bytes&3))return 0;
    // One wait before and after. Fill chunks use one address and values;
    // copying emits one positive source DWORD per MEM_TO_MEM packet.
    return source?2+(size_t)(bytes/4)*6:5+(size_t)(bytes/4);
}
static inline bool smb_emit(uint32_t *words,size_t capacity,uint64_t source,uint64_t destination,
    uint64_t bytes,uint32_t fill_byte,size_t *written)
{
    size_t need=smb_words(source,bytes);
    if(!words || !written || !need || capacity<need || fill_byte>255 ||
        destination<UINT64_C(0x40000000) || destination>=UINT64_C(0x80000000) ||
        bytes>UINT64_C(0x80000000)-destination || (destination&3) ||
        (source && (source<UINT64_C(0x40000000) || source>=UINT64_C(0x80000000) ||
        bytes>UINT64_C(0x80000000)-source || (source&3))))return false;
    size_t at=0;words[at++]=smb_packet7(0x26,0); /* WAIT_FOR_IDLE */
    if(source)for(uint64_t offset=0;offset<bytes;offset+=4){
        uint64_t from=source+offset,to=destination+offset;
        words[at++]=smb_packet7(0x73,5);words[at++]=1u<<30;
        words[at++]=(uint32_t)to;words[at++]=(uint32_t)(to>>32);
        words[at++]=(uint32_t)from;words[at++]=(uint32_t)(from>>32);
    }else{
        words[at++]=smb_packet7(0x3d,2+(uint32_t)(bytes/4));
        words[at++]=(uint32_t)destination;words[at++]=(uint32_t)(destination>>32);
        uint32_t value=fill_byte*0x01010101u;
        for(uint64_t offset=0;offset<bytes;offset+=4)words[at++]=value;
    }
    words[at++]=smb_packet7(0x12,0); /* WAIT_MEM_WRITES before final kernel fence */
    if(at!=need)return false;*written=at;return true;
}
