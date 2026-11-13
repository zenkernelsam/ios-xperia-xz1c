/* Object/state tests use guarded host backing; hardware proof is separate. */
#import "../../gpu/metal/MetalResources.mm"
#include <assert.h>
#include <map>
#include <atomic>
static std::map<uint32_t,SweetMetalBacking> hostResources;
static uint32_t nextResource=1;
static unsigned allocatedCount,releasedCount,submittedCount;
static bool injectFailure;
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{
    uint64_t span=sweet_metal_backing_span(bytes);void *memory=NULL;
    if(!span || posix_memalign(&memory,16384,span))return -ENOMEM;
    *out={bytes,span,UINT64_C(0x40000000)+uint64_t(nextResource)*32768,memory,nextResource++,0};
    memset(memory,0xa5,span);hostResources[out->handle]=*out;++allocatedCount;return 0;
}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle)
{auto it=hostResources.find(handle);assert(it!=hostResources.end());free(it->second.contents);hostResources.erase(it);++releasedCount;return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){return hostResources.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t source,uint32_t destination,uint64_t sourceOffset,uint64_t destinationOffset,uint64_t bytes,uint32_t fill,uint32_t *sequence)
{
    assert(sequence && hostResources.count(destination));*sequence=0;++submittedCount;if(injectFailure)return -ETIMEDOUT;
    auto &dst=hostResources[destination];assert(smb_range(dst.logical_bytes,destinationOffset,bytes));
    auto *target=(unsigned char *)dst.contents+destinationOffset;
    if(source){assert(hostResources.count(source));auto &src=hostResources[source];assert(smb_range(src.logical_bytes,sourceOffset,bytes));
        memcpy(target,(unsigned char *)src.contents+sourceOffset,bytes);}
    else memset(target,fill,bytes);
    *sequence=submittedCount;return 0;
}
static void expectException(void(^action)(void))
{bool caught=false;@try{action();}@catch(NSException *exception){(void)exception;caught=true;}assert(caught);}
int main(void){@autoreleasepool{
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);
    id<MTLCommandQueue> queue=[device newCommandQueue];assert(queue && queue.device==device);
    id<MTLBuffer> src=[device newBufferWithLength:256 options:0],dst=[device newBufferWithLength:256 options:0];
    assert(src && dst);unsigned char *input=(unsigned char *)src.contents,*output=(unsigned char *)dst.contents;
    for(unsigned i=0;i<256;++i)input[i]=(unsigned char)(i*7+1);
    std::atomic_uint scheduled{0},completed{0};auto *scheduledCounter=&scheduled,*completedCounter=&completed;
    dispatch_semaphore_t callbacks=dispatch_semaphore_create(0);
    id<MTLCommandBuffer> commands=[queue commandBuffer];[commands retain];
    expectException(^{[commands waitUntilCompleted];});
    id<MTLBlitCommandEncoder> encoder=[commands blitCommandEncoder];assert(encoder && ![commands blitCommandEncoder]);
    expectException(^{[commands commit];});
    expectException(^{[encoder copyFromBuffer:src sourceOffset:0 toBuffer:dst destinationOffset:254 size:4];});
    expectException(^{[encoder copyFromBuffer:src sourceOffset:1 toBuffer:dst destinationOffset:0 size:4];});
    expectException(^{[encoder copyFromBuffer:src sourceOffset:0 toBuffer:src destinationOffset:4 size:8];});
    [encoder fillBuffer:dst range:NSMakeRange(16,64) value:0x3c];
    [encoder copyFromBuffer:src sourceOffset:40 toBuffer:dst destinationOffset:96 size:64];
    [encoder endEncoding];expectException(^{[encoder endEncoding];});
    [commands addScheduledHandler:^(id<MTLCommandBuffer> callback){assert(callback==commands && callback.status>=MTLCommandBufferStatusScheduled);++*scheduledCounter;}];
    [commands addCompletedHandler:^(id<MTLCommandBuffer> callback){assert(callback==commands && callback.status==MTLCommandBufferStatusCompleted && !callback.error);++*completedCounter;dispatch_semaphore_signal(callbacks);}];
    [commands enqueue];[commands commit];expectException(^{[commands commit];});[commands waitUntilScheduled];[commands waitUntilCompleted];
    assert(!dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC)));
    assert(scheduled==1 && completed==1 && submittedCount==2);
    for(unsigned i=0;i<256;++i){unsigned char expected=i>=16&&i<80?0x3c:i>=96&&i<160?input[40+i-96]:0xa5;assert(output[i]==expected);}
    assert([(SweetMTLCommandBuffer *)commands sweetGPUOperations]==2 && [(SweetMTLCommandBuffer *)commands sweetCompletedSequence]==2);
    uint32_t resultSequence=0,resultOperations=0;assert(SweetMTLCommandResult(commands,&resultSequence,&resultOperations));
    assert(resultSequence==2 && resultOperations==2 && !SweetMTLCommandResult(commands,NULL,&resultOperations));
    [commands release];[src release];[dst release];
    id<MTLCommandBuffer> failed=[queue commandBuffer];[failed retain];id<MTLBuffer> retained=[device newBufferWithLength:64 options:0];
    encoder=[failed blitCommandEncoder];[encoder fillBuffer:retained range:NSMakeRange(0,64) value:0];[encoder endEncoding];[retained release];
    assert(device.currentAllocatedSize==16384);injectFailure=true;[failed commit];[failed waitUntilCompleted];
    assert(failed.status==MTLCommandBufferStatusError && failed.error.code==MTLCommandBufferErrorTimeout && device.currentAllocatedSize==0);
    assert(!SweetMTLCommandResult(failed,&resultSequence,&resultOperations) && !resultSequence && !resultOperations);
    assert([(SweetMTLCommandBuffer *)failed sweetGPUOperations]==0);[failed release];injectFailure=false;
    id<MTLBuffer> fifo=[device newBufferWithLength:64 options:0];assert(fifo);
    id<MTLCommandBuffer> first=[[queue commandBuffer] retain],second=[[queue commandBuffer] retain];
    encoder=[first blitCommandEncoder];[encoder fillBuffer:fifo range:NSMakeRange(0,64) value:0x11];[encoder endEncoding];
    encoder=[second blitCommandEncoder];[encoder fillBuffer:fifo range:NSMakeRange(0,64) value:0x22];[encoder endEncoding];
    [first enqueue];[second commit];assert(first.status==MTLCommandBufferStatusEnqueued);
    [first commit];[second waitUntilCompleted];assert(first.status==MTLCommandBufferStatusCompleted && second.status==MTLCommandBufferStatusCompleted);
    for(unsigned i=0;i<64;++i)assert(((unsigned char *)fifo.contents)[i]==0x22);
    assert([(SweetMTLCommandBuffer *)first sweetCompletedSequence]<[(SweetMTLCommandBuffer *)second sweetCompletedSequence]);
    [first release];[second release];[fifo release];
    assert(![device newCommandQueueWithMaxCommandBufferCount:0] && ![device newCommandQueueWithMaxCommandBufferCount:4097]);
    id<MTLCommandQueue> bounded=[device newCommandQueueWithMaxCommandBufferCount:1];
    id<MTLCommandBuffer> slot=[[bounded commandBuffer] retain];[slot commit];[slot waitUntilCompleted];
    id<MTLCommandBuffer> next=[[bounded commandBuffer] retain];assert(next);[next commit];[next waitUntilCompleted];
    [slot release];[next release];[bounded release];
    [queue release];[device release];dispatch_release(callbacks);
    assert(hostResources.empty() && allocatedCount==releasedCount);
    puts("native queue: enqueue/commit FIFO, capacity, callbacks, ranges, retained backing and hardware-error state PASS");
}return 0;}
