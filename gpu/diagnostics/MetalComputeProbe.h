/* Standard Metal AIR library/pipeline/encoder, original backing and guard bytes. */
#pragma once
#include <dispatch/dispatch.h>
#include <stdatomic.h>
#include <CommonCrypto/CommonDigest.h>
static bool metal_compute_probe(MetalABIEmit emit)
{
    emit("SWEET AIR COMPUTE begin library_bytes=%lu standard_MTLB=1 standard_compute_encoder=1 original_buffer=1",(unsigned long)sizeof(sweet_air_fixture));
    unsigned char digest[32];CC_SHA256(sweet_air_fixture,(CC_LONG)sizeof(sweet_air_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);
    emit("SWEET AIR COMPUTE input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_fixture,sizeof(sweet_air_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=device?[device newLibraryWithData:data error:&error]:nil;dispatch_release(data);
    id<MTLFunction> function=library?[library newFunctionWithName:@"add_seven"]:nil;
    id<MTLComputePipelineState> pipeline=function?[device newComputePipelineStateWithFunction:function error:&error]:nil;
    id<MTLCommandQueue> queue=device?[device newCommandQueueWithMaxCommandBufferCount:8]:nil;
    id<MTLBuffer> buffer=device?[device newBufferWithLength:320 options:0]:nil;
    uint32_t *original=buffer?(uint32_t *)buffer.contents:NULL;bool valid=device && library && function && pipeline && queue && buffer && original;
    dispatch_semaphore_t callbacks=dispatch_semaphore_create(0);_Atomic unsigned completed=0;_Atomic unsigned *count=&completed;
    uint32_t sequences[2]={0};uint64_t hashes[2]={0};
    if(valid){for(unsigned i=0;i<80;++i)original[i]=(i*0x9e3779b1u)^0x54544149u;}
    for(unsigned phase=0;phase<2 && valid;++phase){
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];
        valid=commands && encoder;
        if(valid){[encoder setComputePipelineState:pipeline];[encoder setBuffer:buffer offset:0 atIndex:0];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];
            [commands addCompletedHandler:^(id<MTLCommandBuffer> result){(void)result;atomic_fetch_add_explicit(count,1,memory_order_relaxed);dispatch_semaphore_signal(callbacks);}];
            [commands commit];[commands waitUntilCompleted];
            valid=!dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC));}
        uint32_t operations=0;if(valid)valid=SweetMTLCommandResult(commands,&sequences[phase],&operations) && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && buffer.contents==original;
        unsigned mismatches=0;uint64_t hash=UINT64_C(14695981039346656037);
        if(valid){for(unsigned i=0;i<80;++i){uint32_t expected=(i*0x9e3779b1u)^0x54544149u;
                if(i<64)expected+=7u*(phase+1);mismatches+=original[i]!=expected;}
            const unsigned char *bytes=(const unsigned char *)original;for(unsigned i=0;i<320;++i)hash=(hash^bytes[i])*UINT64_C(1099511628211);}
        valid=valid && !mismatches;hashes[phase]=hash;
        emit("SWEET AIR COMPUTE phase=%u status=%lu error=%u sequence=%u operations=%u callback_count=%u checked_words=%u guard_words=16 mismatches=%u hash=%016llx same_original_pages=%u valid=%u",
            phase,(unsigned long)(commands?commands.status:0),commands.error!=nil,sequences[phase],operations,
            atomic_load_explicit(count,memory_order_relaxed),valid?80:0,mismatches,(unsigned long long)hash,buffer && buffer.contents==original,valid);
        if(!valid && commands.error)emit("SWEET AIR COMPUTE failure phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);
        [commands release];
    }
    valid=valid && sequences[0] && sequences[1]>sequences[0] && hashes[0]!=hashes[1] && atomic_load_explicit(count,memory_order_relaxed)==2;
    [queue release];[pipeline release];[function release];[library release];[buffer release];
    NSUInteger final=device?device.currentAllocatedSize:0;valid=valid && final==0;
    emit("SWEET AIR COMPUTE END result=%u phases=2 kernel_invocations=128 checked_words=160 guard_checks=32 final_allocated=%lu standard_Metal=1",
        valid?0:1,(unsigned long)final);
    [device release];if(callbacks)dispatch_release(callbacks);return valid;
}
