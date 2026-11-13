/* Actual Metal objects with the existing kernel allocator; no GPU submissions. */
#pragma once
#import "../metal/MetalResources.h"
#include <stdint.h>
static bool metal_resource_probe(MetalABIEmit emit)
{
    emit("SWEET METAL RESOURCE begin custom_factory=1 typed_objects=1 kernel_backing=SweetScatter gpu_submit=0");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error),held=nil;
    id<MTLBuffer> buffers[3]={nil,nil,nil};bool valid=device!=nil;
    const NSUInteger lengths[3]={432,16384,16385},spans[3]={16384,16384,32768};
    if(valid)valid=device.currentAllocatedSize==0;
    for(unsigned index=0;index<3 && valid;++index) {
        buffers[index]=[device newBufferWithLength:lengths[index] options:1];
        id<MTLBuffer> buffer=buffers[index];void *contents=buffer?buffer.contents:NULL;
        uint64_t address=buffer?buffer.gpuAddress:0;
        valid=buffer && contents && buffer.device==device && buffer.length==lengths[index] &&
            buffer.allocatedSize==spans[index] && !((uintptr_t)contents&16383) && !(address&16383) &&
            address>=UINT64_C(0x40000000) && address<UINT64_C(0x80000000) && spans[index]<=UINT64_C(0x80000000)-address &&
            buffer.storageMode==MTLStorageModeShared && buffer.cpuCacheMode==MTLCPUCacheModeDefaultCache;
        uint64_t hash=UINT64_C(14695981039346656037);NSUInteger mismatches=0;
        if(valid) {
            unsigned char *bytes=(unsigned char *)contents;
            for(NSUInteger i=0;i<lengths[index];++i)bytes[i]=(unsigned char)(i*13+index*71+3);
            valid=buffer.contents==contents;
            if(valid)for(NSUInteger i=0;i<lengths[index];++i){unsigned char expected=(unsigned char)(i*13+index*71+3);
                mismatches+=bytes[i]!=expected;hash=(hash^bytes[i])*UINT64_C(1099511628211);}
            valid=valid && !mismatches;
        }
        emit("SWEET METAL RESOURCE view index=%u length=%lu allocated=%lu gpu=0x%llx cpu=%p cache=%lu storage=%lu checked=%lu mismatches=%lu hash=%016llx owner_match=%u valid=%u",
            index,(unsigned long)(buffer?buffer.length:0),(unsigned long)(buffer?buffer.allocatedSize:0),(unsigned long long)address,contents,
            (unsigned long)(buffer?buffer.cpuCacheMode:0),(unsigned long)(buffer?buffer.storageMode:0),
            (unsigned long)(valid?lengths[index]:0),(unsigned long)mismatches,(unsigned long long)hash,buffer && buffer.device==device,valid);
    }
    if(valid){held=[buffers[0].device retain];valid=device.currentAllocatedSize==65536;}
    [device release];device=nil;
    if(valid){valid=buffers[0].device==held && held.currentAllocatedSize==65536;
        emit("SWEET METAL RESOURCE lifetime owner_released=1 held_owner=1 current=%lu valid=%u",(unsigned long)held.currentAllocatedSize,valid);}
    for(unsigned i=0;i<3;++i){[buffers[i] release];buffers[i]=nil;}
    NSUInteger final=held?held.currentAllocatedSize:0;if(held)valid=valid && final==0;
    emit("SWEET METAL RESOURCE END result=%u buffers=3 bytes=33201 span=65536 final_allocated=%lu gpu_submit=0",valid?0:1,(unsigned long)final);
    [held release];return valid;
}
