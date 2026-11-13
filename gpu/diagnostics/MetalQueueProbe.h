/* Native Metal command lifecycle and real A5xx work. Full guards are checked. */
#pragma once
#include <dispatch/dispatch.h>
#include <stdatomic.h>
#include <string.h>
static bool metal_queue_probe(MetalABIEmit emit)
{
    enum {BYTES=33024};
    emit("SWEET METAL QUEUE begin bytes=33024 native_queue=1 enqueue_fifo=1 full_guards=1 gpu_commands=1");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    id<MTLCommandQueue> queue=device?[device newCommandQueueWithMaxCommandBufferCount:8]:nil;
    id<MTLBuffer> source=device?[device newBufferWithLength:BYTES options:0]:nil;
    id<MTLBuffer> destination=device?[device newBufferWithLength:BYTES options:0]:nil;
    id<MTLCommandBuffer> first=nil,second=nil;
    unsigned char *src=source?source.contents:NULL,*dst=destination?destination.contents:NULL;
    bool valid=device && queue && source && destination && src && dst && source.device==device && destination.device==device;
    dispatch_semaphore_t callbacks=dispatch_semaphore_create(0);
    _Atomic unsigned scheduled=0,completed=0;_Atomic unsigned *scheduled_count=&scheduled,*completed_count=&completed;
    uint32_t sequences[2]={0},operations[2]={0};
    uint64_t hash=UINT64_C(14695981039346656037);unsigned mismatches=0;
    if(valid){
        for(unsigned i=0;i<BYTES;++i){src[i]=(unsigned char)(i*7+1);dst[i]=0xa5;}
        first=[[queue commandBuffer] retain];second=[[queue commandBuffer] retain];valid=first && second;
    }
    if(valid){
        id<MTLBlitCommandEncoder> encoder=[first blitCommandEncoder];valid=encoder!=nil;
        if(valid){[encoder fillBuffer:destination range:NSMakeRange(16,64) value:0x3c];
            [encoder copyFromBuffer:source sourceOffset:40 toBuffer:destination destinationOffset:96 size:64];[encoder endEncoding];}
        encoder=[second blitCommandEncoder];valid=valid && encoder!=nil;
        if(valid){[encoder copyFromBuffer:source sourceOffset:512 toBuffer:destination destinationOffset:512 size:16396];[encoder endEncoding];}
    }
    if(valid){
        [first addScheduledHandler:^(id<MTLCommandBuffer> commands){(void)commands;atomic_fetch_add_explicit(scheduled_count,1,memory_order_relaxed);}];
        [second addScheduledHandler:^(id<MTLCommandBuffer> commands){(void)commands;atomic_fetch_add_explicit(scheduled_count,1,memory_order_relaxed);}];
        [first addCompletedHandler:^(id<MTLCommandBuffer> commands){(void)commands;atomic_fetch_add_explicit(completed_count,1,memory_order_relaxed);dispatch_semaphore_signal(callbacks);}];
        [second addCompletedHandler:^(id<MTLCommandBuffer> commands){(void)commands;atomic_fetch_add_explicit(completed_count,1,memory_order_relaxed);dispatch_semaphore_signal(callbacks);}];
        [first enqueue];[second commit];[first commit];[second waitUntilCompleted];[first waitUntilCompleted];
        valid=!dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC)) &&
              !dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC));
        valid=valid && SweetMTLCommandResult(first,&sequences[0],&operations[0]) && SweetMTLCommandResult(second,&sequences[1],&operations[1]) &&
            operations[0]==2 && operations[1]==1 && sequences[0]<sequences[1] &&
            atomic_load_explicit(scheduled_count,memory_order_relaxed)==2 && atomic_load_explicit(completed_count,memory_order_relaxed)==2;
        dst=destination.contents;valid=valid && dst!=NULL;
        if(valid)for(unsigned i=0;i<BYTES;++i){unsigned char expected=i>=16&&i<80?0x3c:
            i>=96&&i<160?(unsigned char)((40+i-96)*7+1):i>=512&&i<16908?(unsigned char)(i*7+1):0xa5;
            mismatches+=dst[i]!=expected;hash=(hash^dst[i])*UINT64_C(1099511628211);}
        valid=valid && !mismatches;
    }
    emit("SWEET METAL QUEUE completed first_status=%lu second_status=%lu first_seq=%u second_seq=%u first_ops=%u second_ops=%u scheduled=%u callbacks=%u checked=%u mismatches=%u hash=%016llx valid=%u",
        (unsigned long)(first?first.status:0),(unsigned long)(second?second.status:0),sequences[0],sequences[1],operations[0],operations[1],
        atomic_load_explicit(scheduled_count,memory_order_relaxed),atomic_load_explicit(completed_count,memory_order_relaxed),
        dst && valid?BYTES:0,mismatches,(unsigned long long)hash,valid);
    [first release];[second release];[source release];[destination release];[queue release];
    NSUInteger allocated=device?device.currentAllocatedSize:0;valid=valid && allocated==0;
    emit("SWEET METAL QUEUE END result=%u final_allocated=%lu copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1",
        valid?0:1,(unsigned long)allocated);
    [device release];if(callbacks)dispatch_release(callbacks);return valid;
}
