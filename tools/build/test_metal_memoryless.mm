/* Real native transient descriptors, per-pass pages, rollback and queue lifetime.
 * GPU transport is a host stub, never device pixel evidence. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <cassert>
#include <map>
#include <vector>
#include <mutex>
#include "MultisampleBackendHostStub.h"
static std::map<uint32_t,SweetMetalBacking> memoryPages;
static std::mutex memoryPagesLock;
static uint32_t memoryHandle=1,memoryFence;
static unsigned failAllocationAt,allocationCalls,memoryPrograms,memoryRetired;
static bool failGPU;
static std::vector<uint32_t> depthHandles,stencilHandles,colorHandles;
struct SweetMetalProgram {SweetRenderPipelineConfig config;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_depth32_supported(void){return 1;}
extern "C" int sweetgl_metal_stencil8_supported(void){return 1;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{
    std::lock_guard<std::mutex> lock(memoryPagesLock);if(++allocationCalls==failAllocationAt)return -ENOMEM;
    uint64_t span=sweet_metal_backing_span(bytes);void *p=nullptr;if(!span || posix_memalign(&p,16384,span))return -ENOMEM;
    memset(p,0xa5,span);*out={bytes,span,UINT64_C(0x40000000)+uint64_t(memoryHandle)*65536,p,memoryHandle++,0};memoryPages[out->handle]=*out;return 0;
}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle)
{std::lock_guard<std::mutex> lock(memoryPagesLock);assert(memoryPages.count(handle));free(memoryPages[handle].contents);memoryPages.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle)
{std::lock_guard<std::mutex> lock(memoryPagesLock);return memoryPages.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){assert(!"Transient test does not blit");return -ENOTSUP;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_program_cache_state(const SweetMetalProgram *){return 0;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,char *,size_t)
{assert(vertex && fragment && config && config->format==80);++memoryPrograms;return new SweetMetalProgram{*config};}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){if(program){delete program;++memoryRetired;}}
static int memoryless_draw(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,uint32_t *fence)
{
    std::lock_guard<std::mutex> lock(memoryPagesLock);assert(pass && memoryPages.count(pass->target.handle));
    if(depth){assert(memoryPages.count(depth->target.handle) && depth->target.format==252 && depth->target.offset==64);depthHandles.push_back(depth->target.handle);}
    if(stencil){assert(memoryPages.count(stencil->target.handle) && stencil->target.format==253 && stencil->target.offset==64);stencilHandles.push_back(stencil->target.handle);}
    colorHandles.push_back(pass->target.handle);if(failGPU)return -EIO;*fence=++memoryFence;return 0;
}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *,const SweetRenderPass *pass,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,
    const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *fence){return memoryless_draw(pass,nullptr,nullptr,fence);}
extern "C" int sweetgl_metal_render_draw_depth(SweetMetalProgram *,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,
    const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *fence){return memoryless_draw(pass,depth,nullptr,fence);}
extern "C" int sweetgl_metal_render_draw_stencil(SweetMetalProgram *,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,
    const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *fence){return memoryless_draw(pass,depth,stencil,fence);}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *pass,uint32_t *fence){return memoryless_draw(pass,nullptr,nullptr,fence);}
extern "C" int sweetgl_metal_render_clear_depth(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,uint32_t *fence){return memoryless_draw(pass,depth,nullptr,fence);}
extern "C" int sweetgl_metal_render_clear_stencil(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,uint32_t *fence){return memoryless_draw(pass,depth,stencil,fence);}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -ENOTSUP;}
static void memoryThrow(void(^action)(void)){bool thrown=false;@try{action();}@catch(id error){(void)error;thrown=true;}assert(thrown);}
static MTLTextureDescriptor *memoryDescriptor(MTLPixelFormat format)
{auto d=[MTLTextureDescriptor new];d.width=17;d.height=9;d.pixelFormat=format;d.usage=MTLTextureUsageRenderTarget;d.storageMode=MTLStorageModeMemoryless;return d;}
static MTLRenderPassDescriptor *memoryPass(id<MTLTexture> color,id<MTLTexture> depth,id<MTLTexture> stencil)
{
    auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=color;
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=color.storageMode==MTLStorageModeMemoryless?MTLStoreActionDontCare:MTLStoreActionStore;
    pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionDontCare;
    pass.stencilAttachment.texture=stencil;pass.stencilAttachment.loadAction=MTLLoadActionClear;pass.stencilAttachment.storeAction=MTLStoreActionDontCare;
    return pass;
}
static void publicMemorylessResources(id<MTLDevice> device)
{
    for(auto format:{MTLPixelFormatBGRA8Unorm,MTLPixelFormatDepth32Float,MTLPixelFormatStencil8}){
        auto d=memoryDescriptor(format);NSUInteger allocated=device.currentAllocatedSize;auto texture=[device newTextureWithDescriptor:d];
        assert(texture && texture.device==device && texture.storageMode==MTLStorageModeMemoryless && !texture.buffer && !texture.iosurface &&
            !texture.allocatedSize && !texture.bufferBytesPerRow && texture.framebufferOnly && device.currentAllocatedSize==allocated);
        auto native=(SweetMTLTexture *)texture;assert(![native sweetBacking].handle && ![native synchronizedContents] && ![native bindingForIndex:0 stage:0].handle);
        char bytes[68];void *output=bytes;const void *input=bytes;
        memoryThrow(^{[texture getBytes:output bytesPerRow:68 fromRegion:MTLRegionMake2D(0,0,17,1) mipmapLevel:0];});
        memoryThrow(^{[texture replaceRegion:MTLRegionMake2D(0,0,17,1) mipmapLevel:0 withBytes:input bytesPerRow:68];});
        d.width=999;assert(texture.width==17);d.width=17;d.usage|=MTLTextureUsageShaderRead;assert(![device newTextureWithDescriptor:d]);d.usage=MTLTextureUsageRenderTarget;
        d.sampleCount=4;assert(![device newTextureWithDescriptor:d]);d.sampleCount=1;d.mipmapLevelCount=2;assert(![device newTextureWithDescriptor:d]);
        [texture release];[d release];assert(device.currentAllocatedSize==allocated);
    }
}
int main(int argc,char **argv){@autoreleasepool{
    assert(argc==2);NSError *error=nil;auto device=SweetMTLCreateDevice(&error);assert(device && !error);publicMemorylessResources(device);
    auto library=[device newLibraryWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]] error:&error];
    auto vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];assert(vertex && fragment);
    auto pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;pd.stencilAttachmentPixelFormat=MTLPixelFormatStencil8;
    auto pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&error];assert(pipeline && !error);
    auto cd=memoryDescriptor(MTLPixelFormatBGRA8Unorm);cd.storageMode=MTLStorageModeShared;
    auto dd=memoryDescriptor(MTLPixelFormatDepth32Float),sd=memoryDescriptor(MTLPixelFormatStencil8);
    auto color=[device newTextureWithDescriptor:cd],depth=[device newTextureWithDescriptor:dd],stencil=[device newTextureWithDescriptor:sd];
    auto vertices=[device newBufferWithLength:48 options:0];auto queueA=[device newCommandQueue],queueB=[device newCommandQueue];
    auto pass=memoryPass(color,depth,stencil);NSUInteger base=device.currentAllocatedSize;
    auto command=(SweetMTLCommandBuffer *)[[queueA commandBuffer] retain];
    pass.depthAttachment.loadAction=MTLLoadActionLoad;assert(![command renderCommandEncoderWithDescriptor:pass] && device.currentAllocatedSize==base);pass.depthAttachment.loadAction=MTLLoadActionClear;
    pass.depthAttachment.storeAction=MTLStoreActionStore;assert(![command renderCommandEncoderWithDescriptor:pass] && device.currentAllocatedSize==base);pass.depthAttachment.storeAction=MTLStoreActionDontCare;
    // If the second transient plane cannot allocate, the first is rolled back.
    failAllocationAt=allocationCalls+2;assert(![command renderCommandEncoderWithDescriptor:pass] && device.currentAllocatedSize==base);failAllocationAt=0;
    id<MTLRenderCommandEncoder> encoders[2]={nil,nil};SweetMTLCommandBuffer *commands[2]={command,(SweetMTLCommandBuffer *)[[queueB commandBuffer] retain]};
    SweetMTLMemorylessPassInfo depthInfo[2]={},stencilInfo[2]={};id<MTLBuffer> retainedDepth[2]={nil,nil},retainedStencil[2]={nil,nil};
    for(unsigned i=0;i<2;++i){encoders[i]=[commands[i] renderCommandEncoderWithDescriptor:pass];assert(encoders[i]);
        retainedDepth[i]=SweetMTLRetainMemorylessPassBuffer(encoders[i],8,&depthInfo[i]);retainedStencil[i]=SweetMTLRetainMemorylessPassBuffer(encoders[i],9,&stencilInfo[i]);
        assert(retainedDepth[i] && retainedStencil[i] && depthInfo[i].pass_id==stencilInfo[i].pass_id && depthInfo[i].public_texture==(uintptr_t)depth);
        [encoders[i] setRenderPipelineState:pipeline];[encoders[i] setVertexBuffer:vertices offset:0 atIndex:0];
        [encoders[i] drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoders[i] drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoders[i] endEncoding];SweetMTLMemorylessPassInfo closed={};assert(!SweetMTLRetainMemorylessPassBuffer(encoders[i],8,&closed) && !closed.handle);
        assert(pass.depthAttachment.texture==depth && pass.stencilAttachment.texture==stencil);}
    assert(depthInfo[0].handle!=depthInfo[1].handle && depthInfo[0].gpu_address!=depthInfo[1].gpu_address && depthInfo[0].pass_id!=depthInfo[1].pass_id);
    // The snapshot verifier must reject replacement or duplicated backing.
    @autoreleasepool{NSMutableDictionary *changed=[commands[0]->_operations[0] mutableCopy];NSArray *bindings=changed[@"memoryless"];
        changed[@"memoryless"]=@[bindings[0],bindings[0]];assert(!memoryless_operation_valid(changed,device));[changed release];
        changed=[commands[0]->_operations[0] mutableCopy];NSData *saved=changed[@"depthPass"];SweetRenderDepthPass wrong={};memcpy(&wrong,saved.bytes,sizeof(wrong));
        wrong.store_action=MTLStoreActionStore;changed[@"depthPass"]=[NSData dataWithBytes:&wrong length:sizeof(wrong)];
        assert(!memoryless_operation_valid(changed,device));[changed release];}
    [pass release];[depth release];[stencil release];[color release];[vertices release];[pipeline release];[pd release];[dd release];[sd release];[cd release];
    [commands[0] commit];[commands[1] commit];
    for(unsigned i=0;i<2;++i){[commands[i] waitUntilCompleted];assert(commands[i].status==MTLCommandBufferStatusCompleted && commands[i].sweetGPUOperations==2);
        [commands[i] release];[retainedDepth[i] release];[retainedStencil[i] release];}
    assert(depthHandles.size()==4 && stencilHandles.size()==4 && memoryPages.empty() && !device.currentAllocatedSize);
    for(unsigned i=0;i<2;++i){unsigned depths=0,stencils=0;
        for(auto handle:depthHandles)depths+=handle==depthInfo[i].handle;
        for(auto handle:stencilHandles)stencils+=handle==stencilInfo[i].handle;
        assert(depths==2 && stencils==2);}
    // Public color is supported too. Failed commands retire every private page.
    cd=memoryDescriptor(MTLPixelFormatBGRA8Unorm);color=[device newTextureWithDescriptor:cd];pass=memoryPass(color,nil,nil);
    command=(SweetMTLCommandBuffer *)[[queueA commandBuffer] retain];auto encoder=[command renderCommandEncoderWithDescriptor:pass];assert(encoder);
    [encoder endEncoding];[pass release];[color release];[cd release];failGPU=true;[command commit];[command waitUntilCompleted];failGPU=false;
    assert(command.status==MTLCommandBufferStatusError && !device.currentAllocatedSize && memoryPages.empty());[command release];
    [queueA release];[queueB release];[vertex release];[fragment release];[library release];[device release];assert(memoryPrograms==memoryRetired);
    puts("Native Memoryless objects PASS: immutable public metadata, no CPU/sampler backing, exact per-pass pages, concurrent queue isolation, same-pass retention, allocation rollback, GPU error retirement; host GPU stub only");
}return 0;}
