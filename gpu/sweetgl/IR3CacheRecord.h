/* SPDX-License-Identifier: BSD-2-Clause
 * Exact-build IR3 payloads. Existing records are never replaced or removed. */
#pragma once
#include "ProgramCacheCodec.h"
#define SWEET_IR3_HEADER 112u
#define SWEET_IR3_MAX (8u<<20)
static inline bool sweet_ir3_record(unsigned char header[SWEET_IR3_HEADER],const unsigned char build[32],
    const unsigned char key[32],const void *payload,size_t bytes)
{
    if(!payload || !bytes || bytes>SWEET_IR3_MAX)return false;
    memset(header,0,SWEET_IR3_HEADER);memcpy(header,"SGIV0001",8);sweet_cache_put32(header+8,1);sweet_cache_put32(header+12,(uint32_t)bytes);
    memcpy(header+16,build,32);memcpy(header+48,key,32);sweet_cache_digest(payload,bytes,header+80);return true;
}
static inline bool sweet_ir3_validate(const unsigned char *record,size_t bytes,const unsigned char build[32],const unsigned char key[32])
{
    if(!record || bytes<SWEET_IR3_HEADER || memcmp(record,"SGIV0001",8) || sweet_cache_get32(record+8)!=1 ||
        memcmp(record+16,build,32) || memcmp(record+48,key,32))return false;
    uint32_t size=sweet_cache_get32(record+12);if(!size || size>SWEET_IR3_MAX || bytes!=SWEET_IR3_HEADER+(size_t)size)return false;
    unsigned char digest[32];sweet_cache_digest(record+SWEET_IR3_HEADER,size,digest);return !memcmp(digest,record+80,32);
}
static inline void sweet_ir3_key(const unsigned char build[32],const void *data,size_t bytes,unsigned char key[32])
{
    const char domain[]="lilac.FD540.IR3.variant.v1";SweetCacheHash hash;sweet_cache_hash_init(&hash);
    sweet_cache_hash_add(&hash,domain,sizeof(domain));sweet_cache_hash_add(&hash,build,32);sweet_cache_hash_add(&hash,data,bytes);sweet_cache_hash_end(&hash,key);
}
