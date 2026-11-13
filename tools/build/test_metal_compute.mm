/* Real library/function/pipeline/queue bodies, AIR lowering; GPU is host-stubbed. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <assert.h>
#include <map>
#include <string>
#include <atomic>
static std::map<uint32_t,SweetMetalBacking> backing;
static uint32_t nextHandle=1;static unsigned programs,dispatched,destroyed,released;
struct SweetMetalProgram {std::string source;unsigned buffers,writes;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *result)
{
    uint64_t span=sweet_metal_backing_span(bytes);void *memory=nullptr;if(!span || posix_memalign(&memory,16384,span))return -ENOMEM;
    *result={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*65536,memory,nextHandle++,0};backing[result->handle]=*result;return 0;
}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(backing.count(handle));free(backing[handle].contents);backing.erase(handle);++released;return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){return backing.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){assert(!"compute host test does not blit");return -EINVAL;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *source,uint32_t mask,uint32_t writes,char *error,size_t errorBytes)
{
    if(error && errorBytes)error[0]=0;
    assert(strstr(source,"#version 310 es\n") && strstr(source,"layout(local_size_x=64,local_size_y=1,local_size_z=1)") &&
        !strstr(source,"local_size_variable") && strstr(source,"_qbuffer0.data") && strstr(source,"+7u"));
    ++programs;return new SweetMetalProgram{source,mask,writes};
}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *program,const SweetShaderBufferBinding *bindings,uint32_t count,
    const uint32_t groups[3],const uint32_t local[3],const uint32_t grid[3],uint32_t *sequence)
{
    assert(program && program->buffers==1 && program->writes==1 && count==1 && bindings[0].index==0 && backing.count(bindings[0].handle));
    assert(groups[0]==1 && groups[1]==1 && groups[2]==1 && local[0]==64 && local[1]==1 && local[2]==1 && grid[0]==64);
    assert(bindings[0].offset==0 && bindings[0].bytes==256);++dispatched;*sequence=dispatched;return 0;
}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){delete program;++destroyed;}
static void exception(void(^action)(void)){bool raised=false;@try{action();}@catch(NSException *e){(void)e;raised=true;}assert(raised);}
int main(int argc,char **argv){assert(argc==2);@autoreleasepool{
    NSData *bytes=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];assert(bytes);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);
    dispatch_data_t data=dispatch_data_create(bytes.bytes,bytes.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);assert(library && !error && library.functionNames.count==3);
    if(bytes.length>88 && !memcmp(bytes.bytes,"MTLB",4)){
        NSMutableData *changed=[bytes mutableCopy];unsigned char *raw=(unsigned char *)changed.mutableBytes;
        raw[changed.length-1]^=1;dispatch_data_t wrong=dispatch_data_create(raw,changed.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        NSError *failure=nil;assert(![device newLibraryWithData:wrong error:&failure] && failure);dispatch_release(wrong);[changed release];
    }
    assert(![library newFunctionWithName:@"absent"] && ![library newFunctionWithName:nil]);
    id<MTLFunction> function=[library newFunctionWithName:@"add_seven"];assert(function && function.functionType==MTLFunctionTypeKernel);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"];assert(vertex && vertex.functionType==MTLFunctionTypeVertex);
    assert(![device newComputePipelineStateWithFunction:vertex error:&error]);[vertex release];error=nil;
    id<MTLComputePipelineState> pipeline=[device newComputePipelineStateWithFunction:function error:&error];assert(pipeline && !error);
    id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLBuffer> buffer=[device newBufferWithLength:256 options:0];assert(queue && buffer);
    id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[[commands computeCommandEncoder] retain];assert(encoder);
    [encoder setComputePipelineState:pipeline];[encoder setBuffer:buffer offset:0 atIndex:0];
    exception(^{[encoder setBuffer:buffer offset:1 atIndex:0];});
    exception(^{[encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(1025,1,1)];});
    [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];
    [buffer release];[encoder release];[pipeline release];[function release];[library release];
    assert(device.currentAllocatedSize==16384);[commands commit];[commands waitUntilCompleted];
    fprintf(stderr,"compute-check status=%lu error=%s dispatched=%u programs=%u allocated=%lu\n",
        (unsigned long)commands.status,commands.error?commands.error.localizedDescription.UTF8String:"none",dispatched,programs,(unsigned long)device.currentAllocatedSize);
    assert(commands.status==MTLCommandBufferStatusCompleted && !commands.error && dispatched==1 && programs==1 && device.currentAllocatedSize==0);
    [commands release];[queue release];[device release];
    assert(backing.empty() && destroyed==1 && released==1);
    puts("actual AIR Metal objects: typed library, entry, pipeline, retained dispatch, ESSL target and terminal state PASS");
}return 0;}
