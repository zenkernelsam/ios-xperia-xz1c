/* SPDX-License-Identifier: BSD-2-Clause
 * Bounded immutable program/request records; no process pointers are saved.
 */
#pragma once
#include "MetalShaderAPI.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
typedef CC_SHA256_CTX SweetCacheHash;
static inline void sweet_cache_hash_init(SweetCacheHash *h){CC_SHA256_Init(h);}
static inline void sweet_cache_hash_add(SweetCacheHash *h,const void *p,size_t n){CC_SHA256_Update(h,p,(CC_LONG)n);}
static inline void sweet_cache_hash_end(SweetCacheHash *h,unsigned char out[32]){CC_SHA256_Final(out,h);}
#else
#include <openssl/sha.h>
typedef SHA256_CTX SweetCacheHash;
static inline void sweet_cache_hash_init(SweetCacheHash *h){SHA256_Init(h);}
static inline void sweet_cache_hash_add(SweetCacheHash *h,const void *p,size_t n){SHA256_Update(h,p,n);}
static inline void sweet_cache_hash_end(SweetCacheHash *h,unsigned char out[32]){SHA256_Final(out,h);}
#endif
#define SWEET_PROGRAM_CACHE_HEADER 136u
#define SWEET_PROGRAM_CACHE_MAX (8u<<20)
#define SWEET_PROGRAM_CACHE_REQUEST 1u
#define SWEET_PROGRAM_CACHE_BINARY 2u
static inline void sweet_cache_put32(unsigned char *p,uint32_t n)
{p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static inline uint32_t sweet_cache_get32(const unsigned char *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static inline void sweet_cache_digest(const void *p,size_t n,unsigned char out[32])
{SweetCacheHash h;sweet_cache_hash_init(&h);sweet_cache_hash_add(&h,p,n);sweet_cache_hash_end(&h,out);}
static inline void sweet_cache_key(const unsigned char identity[32],const SweetRenderPipelineConfig *config,
    const char *vertex,size_t vs,const char *fragment,size_t fs,unsigned char out[32])
{
    unsigned char lengths[12];sweet_cache_put32(lengths,(uint32_t)vs);sweet_cache_put32(lengths+4,(uint32_t)fs);
    sweet_cache_put32(lengths+8,(uint32_t)sizeof(*config));SweetCacheHash h;sweet_cache_hash_init(&h);
    sweet_cache_hash_add(&h,identity,32);sweet_cache_hash_add(&h,lengths,sizeof(lengths));
    sweet_cache_hash_add(&h,config,sizeof(*config));sweet_cache_hash_add(&h,vertex,vs);sweet_cache_hash_add(&h,fragment,fs);sweet_cache_hash_end(&h,out);
}
static inline int sweet_cache_header(unsigned char out[SWEET_PROGRAM_CACHE_HEADER],unsigned kind,unsigned format,
    const unsigned char identity[32],const unsigned char key[32],const void *payload,size_t bytes,size_t vs,size_t fs)
{
    if(!out || !identity || !key || !payload || !bytes || bytes>SWEET_PROGRAM_CACHE_MAX || !vs || !fs || vs>1u<<20 || fs>1u<<20 ||
        (kind!=SWEET_PROGRAM_CACHE_REQUEST && kind!=SWEET_PROGRAM_CACHE_BINARY))return 0;
    if(kind==SWEET_PROGRAM_CACHE_REQUEST && (format || bytes!=sizeof(SweetRenderPipelineConfig)+vs+fs))return 0;
    memset(out,0,SWEET_PROGRAM_CACHE_HEADER);memcpy(out,"SGPB0001",8);sweet_cache_put32(out+8,1);sweet_cache_put32(out+12,kind);
    sweet_cache_put32(out+16,format);sweet_cache_put32(out+20,(uint32_t)bytes);sweet_cache_put32(out+24,(uint32_t)vs);sweet_cache_put32(out+28,(uint32_t)fs);
    sweet_cache_put32(out+32,(uint32_t)sizeof(SweetRenderPipelineConfig));memcpy(out+40,identity,32);memcpy(out+72,key,32);
    sweet_cache_digest(payload,bytes,out+104);return 1;
}
static inline int sweet_cache_validate(const unsigned char *record,size_t bytes,unsigned kind,
    const unsigned char identity[32],const unsigned char key[32])
{
    if(!record || bytes<SWEET_PROGRAM_CACHE_HEADER || memcmp(record,"SGPB0001",8) || sweet_cache_get32(record+8)!=1 ||
        sweet_cache_get32(record+12)!=kind || sweet_cache_get32(record+36) || sweet_cache_get32(record+32)!=sizeof(SweetRenderPipelineConfig) ||
        (identity && memcmp(record+40,identity,32)) || (key && memcmp(record+72,key,32)))return 0;
    uint32_t payload=sweet_cache_get32(record+20),vs=sweet_cache_get32(record+24),fs=sweet_cache_get32(record+28);
    if(!payload || payload>SWEET_PROGRAM_CACHE_MAX || bytes!=SWEET_PROGRAM_CACHE_HEADER+(size_t)payload || !vs || !fs || vs>1u<<20 || fs>1u<<20)return 0;
    const unsigned char *data=record+SWEET_PROGRAM_CACHE_HEADER;unsigned char digest[32];sweet_cache_digest(data,payload,digest);
    if(memcmp(digest,record+104,32))return 0;
    if(kind==SWEET_PROGRAM_CACHE_REQUEST){
        if(sweet_cache_get32(record+16) || payload!=sizeof(SweetRenderPipelineConfig)+(size_t)vs+fs ||
            memchr(data+sizeof(SweetRenderPipelineConfig),0,(size_t)vs+fs))return 0;
    }else if(kind!=SWEET_PROGRAM_CACHE_BINARY || !sweet_cache_get32(record+16))return 0;
    return 1;
}
