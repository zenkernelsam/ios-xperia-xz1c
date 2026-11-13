/* Host lifecycle only; the real device separately validates actual backing. */
#import "../../gpu/metal/MetalResources.mm"
#include <assert.h>
#include <stdlib.h>
static unsigned allocated,released,syncs;static bool fail_sync;
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{
    uint64_t span=sweet_metal_backing_span(bytes);if(!span)return -EINVAL;
    void *pointer=NULL;if(posix_memalign(&pointer,16384,span))return -ENOMEM;
    *out={bytes,span,UINT64_C(0x40000000),pointer,++allocated,0};return 0;
}
static void *resource_pointer;
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(handle && resource_pointer);free(resource_pointer);resource_pointer=NULL;++released;return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){assert(handle);++syncs;return fail_sync?-EIO:0;}
int main(void){@autoreleasepool{
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error && device.currentAllocatedSize==0);
    const uint64_t invalid[]={0x20,0x10,0x400,2,UINT64_MAX};
    for(unsigned i=0;i<5;++i)assert(![device newBufferWithLength:432 options:(MTLResourceOptions)invalid[i]]);
    assert(![device newBufferWithLength:0 options:0] && ![device newBufferWithLength:((NSUInteger)256<<20)+1 options:0]);
    id<MTLBuffer> buffer=[device newBufferWithLength:432 options:1];assert(buffer && buffer.device==device && buffer.length==432);
    resource_pointer=buffer.contents;assert(resource_pointer && device.currentAllocatedSize==16384 && buffer.allocatedSize==16384);
    assert(buffer.cpuCacheMode==MTLCPUCacheModeDefaultCache && buffer.storageMode==MTLStorageModeShared);
    memset(resource_pointer,0x5a,432);assert(!memcmp(buffer.contents,resource_pointer,432));
    buffer.label=@"uniforms";assert([buffer.label isEqual:@"uniforms"]);
    fail_sync=true;assert(!buffer.contents);fail_sync=false;
    [device release];device=nil;assert(buffer.device && buffer.device.currentAllocatedSize==16384);
    [buffer release];assert(allocated==released && syncs>=3);
    puts("actual Metal classes: resource bounds, logical size, lifetime, accounting and sync failure PASS");
}return 0;}
