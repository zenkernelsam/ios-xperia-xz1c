/* SPDX-License-Identifier: BSD-2-Clause
 * Read only retired, authored attachment pages after their byte check fails.
 * No UI page, allocation, submission, CPU repair or success substitution. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { ATTACHMENT_SNAPSHOT_MAX_BYTES=4096, ATTACHMENT_SNAPSHOT_CHUNK=24 };
static uint64_t attachment_snapshot_fnv(const unsigned char *bytes,unsigned count)
{uint64_t value=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)value=(value^bytes[i])*UINT64_C(1099511628211);return value;}
static void attachment_page_snapshot(MetalABIEmit emit,const char *suite,unsigned phase,unsigned slot,
    const unsigned char *bytes,unsigned count,uint64_t GPU,uint32_t sequence)
{
    if(!emit || !suite || (strcmp(suite,"depth32") && strcmp(suite,"stencil8") && strcmp(suite,"memoryless") && strcmp(suite,"msaa")) ||
        !bytes || !count || count>ATTACHMENT_SNAPSHOT_MAX_BYTES || !sequence || GPU<UINT64_C(0x40000000) || GPU>=UINT64_C(0x80000000) || (GPU&16383))return;
    uint64_t hash=attachment_snapshot_fnv(bytes,count);unsigned blocks=(count+ATTACHMENT_SNAPSHOT_CHUNK-1)/ATTACHMENT_SNAPSHOT_CHUNK;
    emit("SWEET ATTACH SNAP BEGIN suite=%s phase=%u slot=%u bytes=%u GPU=%llx sequence=%u fnv=%016llx blocks=%u",suite,phase,slot,count,
        (unsigned long long)GPU,sequence,(unsigned long long)hash,blocks);
    for(unsigned block=0,at=0;at<count;++block,at+=ATTACHMENT_SNAPSHOT_CHUNK){char hex[ATTACHMENT_SNAPSHOT_CHUNK*2+1]={0};
        unsigned size=count-at<ATTACHMENT_SNAPSHOT_CHUNK?count-at:ATTACHMENT_SNAPSHOT_CHUNK;
        for(unsigned i=0;i<size;++i)snprintf(hex+2*i,3,"%02x",bytes[at+i]);
        emit("SWEET ATTACH SNAP DATA suite=%s phase=%u slot=%u block=%u offset=%u n=%u hex=%s",suite,phase,slot,block,at,size,hex);
    }
    uint64_t final=attachment_snapshot_fnv(bytes,count);
    emit("SWEET ATTACH SNAP END suite=%s phase=%u slot=%u bytes=%u fnv=%016llx stable=%u complete=1",suite,phase,slot,count,
        (unsigned long long)final,hash==final);
}
