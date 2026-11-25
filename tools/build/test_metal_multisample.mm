/* Actual native 2x/4x descriptors, original pages, resolve order and lifetime.
 * GPU transport is stubbed here; host GLES and device proofs are separate. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <cassert>
#include <map>
#include <vector>
#include <mutex>
static std::recursive_mutex hostPageGate;
static std::map<uint32_t,SweetMetalBacking> multisamplePages;
static uint32_t nextHandle=1,fence;
static unsigned createdPrograms,retiredPrograms,resolveCalls,drawCalls,clearCalls;
static bool failGPU,failResolve;
static std::vector<uint32_t> sampleMasks;
struct SweetMetalProgram {unsigned samples;SweetRenderPipelineConfig config;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_depth32_supported(void){return 1;}
extern "C" int sweetgl_metal_stencil8_supported(void){return 1;}
extern "C" int sweetgl_metal_multisample_supported(uint32_t format,uint32_t samples)
{return (samples==2 || samples==4) && (format==80 || format==252 || format==253);}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{std::lock_guard<std::recursive_mutex> lock(hostPageGate);uint64_t span=sweet_metal_backing_span(bytes);void *p=nullptr;if(!span || posix_memalign(&p,16384,span))return -ENOMEM;memset(p,0xa5,span);
    *out={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*65536,p,nextHandle++,0};multisamplePages[out->handle]=*out;return 0;}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){std::lock_guard<std::recursive_mutex> lock(hostPageGate);assert(multisamplePages.count(handle));free(multisamplePages[handle].contents);multisamplePages.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){std::lock_guard<std::recursive_mutex> lock(hostPageGate);return multisamplePages.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_program_cache_state(const SweetMetalProgram *){return 0;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *,const char *,const SweetRenderPipelineConfig *config,char *,size_t)
{++createdPrograms;return new SweetMetalProgram{1,*config};}
extern "C" SweetMetalProgram *sweetgl_metal_render_create_multisample(const char *v,const char *f,const SweetRenderPipelineConfig *config,uint32_t samples,char *,size_t)
{assert(v && f && config && (samples==2 || samples==4));++createdPrograms;return new SweetMetalProgram{samples,*config};}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){if(program){delete program;++retiredPrograms;}}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_draw_depth(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_depth(const SweetRenderPass *,const SweetRenderDepthPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_draw_stencil(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_stencil(const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_draw_multisample(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *,uint32_t textures,const SweetShaderSamplerBinding *,uint32_t samplers,uint32_t samples,uint32_t mask,uint32_t *sequence)
{
    std::lock_guard<std::recursive_mutex> lock(hostPageGate);
    assert(program && program->samples==samples && pass && draw && count==1 && buffers && !textures && !samplers);
    assert(multisamplePages.count(pass->target.handle) && pass->target.pitch>=17*4*samples && pass->target.offset==64);
    assert(depth && depth->target.format==252 && depth->target.pitch>=17*4*samples && depth->target.offset==64);
    assert(stencil && stencil->target.format==253 && stencil->target.pitch>=17*samples && stencil->target.offset==64);
    assert(multisamplePages.count(depth->target.handle) && multisamplePages.count(stencil->target.handle));
    sampleMasks.push_back(mask);++drawCalls;if(failGPU)return -EIO;*sequence=++fence;return 0;
}
extern "C" int sweetgl_metal_render_clear_multisample(const SweetRenderPass *pass,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t samples,uint32_t *sequence)
{std::lock_guard<std::recursive_mutex> lock(hostPageGate);assert(pass && multisamplePages.count(pass->target.handle) && (samples==2 || samples==4));++clearCalls;if(failGPU)return -EIO;*sequence=++fence;return 0;}
extern "C" int sweetgl_metal_multisample_resolve(const SweetMultisampleResolve *request,uint32_t *sequence)
{std::lock_guard<std::recursive_mutex> lock(hostPageGate);assert(request && multisamplePages.count(request->source.handle) && multisamplePages.count(request->target.handle));assert(request->source.offset==64);
    assert(request->samples==2 || request->samples==4);++resolveCalls;if(failResolve)return -EIO;*sequence=++fence;return 0;}
static MTLTextureDescriptor *ms_descriptor(MTLPixelFormat format,unsigned samples,MTLStorageMode storage)
{auto d=[MTLTextureDescriptor new];d.pixelFormat=format;d.width=17;d.height=9;d.sampleCount=samples;d.textureType=samples>1?MTLTextureType2DMultisample:MTLTextureType2D;d.storageMode=storage;d.usage=MTLTextureUsageRenderTarget;return d;}
static void ms_throw(void(^action)(void)){bool threw=false;@try{action();}@catch(id e){(void)e;threw=true;}assert(threw);}
int main(int argc,char **argv){@autoreleasepool{
    assert(argc==2);NSError *error=nil;auto device=SweetMTLCreateDevice(&error);auto library=[device newLibraryWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]] error:&error];
    assert([device supportsTextureSampleCount:1] && [device supportsTextureSampleCount:2] && [device supportsTextureSampleCount:4] && ![device supportsTextureSampleCount:3] && ![device supportsTextureSampleCount:8]);
    auto vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];assert(vertex && fragment);auto queue=[device newCommandQueue];
    for(unsigned samples=2;samples<=4;samples*=2){@autoreleasepool{
        auto c=ms_descriptor(MTLPixelFormatBGRA8Unorm,samples,MTLStorageModePrivate),z=ms_descriptor(MTLPixelFormatDepth32Float,samples,MTLStorageModePrivate),s=ms_descriptor(MTLPixelFormatStencil8,samples,MTLStorageModePrivate);
        auto color=[device newTextureWithDescriptor:c],depth=[device newTextureWithDescriptor:z],stencil=[device newTextureWithDescriptor:s];assert(color && depth && stencil);
        SweetMTLMultisampleTextureInfo info={};auto original=SweetMTLRetainMultisampleTextureBuffer(color,&info);assert(original && info.samples==samples && info.offset==64 && info.pitch>=17*4*samples);
        assert(color.sampleCount==samples && !color.buffer && color.storageMode==MTLStorageModePrivate);[original release];
        c.storageMode=MTLStorageModeShared;assert(![device newTextureWithDescriptor:c]);c.storageMode=MTLStorageModePrivate;
        c.sampleCount=UINT64_C(0x100000002);assert(![device newTextureWithDescriptor:c]);c.sampleCount=samples;
        auto rd=ms_descriptor(MTLPixelFormatBGRA8Unorm,1,MTLStorageModeShared),zd=ms_descriptor(MTLPixelFormatDepth32Float,1,MTLStorageModeShared);
        auto resolved=[device newTextureWithDescriptor:rd],resolvedDepth=[device newTextureWithDescriptor:zd];
        auto pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;pd.stencilAttachmentPixelFormat=MTLPixelFormatStencil8;pd.rasterSampleCount=samples;
        auto pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&error];assert(pipeline && !error);pd.rasterSampleCount=1;auto single=[device newRenderPipelineStateWithDescriptor:pd error:&error];assert(single);
        auto data=[device newBufferWithLength:48 options:0];auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionClear;
        pass.colorAttachments[0].resolveTexture=resolved;pass.colorAttachments[0].storeAction=MTLStoreActionMultisampleResolve;
        pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.resolveTexture=resolvedDepth;
        pass.depthAttachment.storeAction=MTLStoreActionMultisampleResolve;pass.depthAttachment.depthResolveFilter=MTLMultisampleDepthResolveFilterMax;
        pass.stencilAttachment.texture=stencil;pass.stencilAttachment.loadAction=MTLLoadActionClear;pass.stencilAttachment.storeAction=MTLStoreActionDontCare;
        auto command=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];
        pass.colorAttachments[0].resolveLevel=1;assert(![command renderCommandEncoderWithDescriptor:pass]);pass.colorAttachments[0].resolveLevel=0;
        auto encoder=[command renderCommandEncoderWithDescriptor:pass];assert(encoder);assert(pass.colorAttachments[0].resolveTexture==resolved);
        [encoder setVertexBuffer:data offset:0 atIndex:0];[encoder setRenderPipelineState:single];ms_throw(^{[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];});
        [encoder setRenderPipelineState:pipeline];assert(!SweetMTLSetDiagnosticSampleMask(encoder,1u<<samples));assert(SweetMTLSetDiagnosticSampleMask(encoder,1));
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];assert(SweetMTLSetDiagnosticSampleMask(encoder,2));[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];
        assert(command->_operations.count==4 && command->_operations[2][@"multisampleResolve"] && command->_operations[3][@"multisampleResolve"]);
        [pass release];[color release];[depth release];[stencil release];[resolved release];[resolvedDepth release];[pipeline release];[single release];[data release];[pd release];[c release];[z release];[s release];[rd release];[zd release];
        [command commit];[command waitUntilCompleted];
        dispatch_sync(((SweetMTLCommandQueue *)queue)->_execution, ^{});
        assert(command.status==MTLCommandBufferStatusCompleted && command.sweetGPUOperations==4 && !device.currentAllocatedSize && multisamplePages.empty());[command release];
    }}
    assert(drawCalls==4 && resolveCalls==4 && sampleMasks==std::vector<uint32_t>({1,2,1,2}));
    for(unsigned samples=2;samples<=4;samples*=2){@autoreleasepool{
        auto d=ms_descriptor(MTLPixelFormatBGRA8Unorm,samples,MTLStorageModePrivate),r=ms_descriptor(MTLPixelFormatBGRA8Unorm,1,MTLStorageModeShared);
        auto source=[device newTextureWithDescriptor:d],target=[device newTextureWithDescriptor:r];
        auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=source;pass.colorAttachments[0].resolveTexture=target;
        pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStoreAndMultisampleResolve;
        // Two color resolves must not target the same bytes, even from two
        // different single-sample texture objects on one original buffer.
        auto second=[device newTextureWithDescriptor:d];auto shared=[device newBufferWithLength:1280 options:0];
        auto aliasA=[shared newTextureWithDescriptor:r offset:64 bytesPerRow:128],aliasB=[shared newTextureWithDescriptor:r offset:64 bytesPerRow:128];
        pass.colorAttachments[0].resolveTexture=aliasA;pass.colorAttachments[1].texture=second;pass.colorAttachments[1].resolveTexture=aliasB;
        pass.colorAttachments[1].loadAction=MTLLoadActionClear;pass.colorAttachments[1].storeAction=MTLStoreActionMultisampleResolve;
        auto rejected=[[queue commandBuffer] retain];assert(![rejected renderCommandEncoderWithDescriptor:pass]);[rejected release];
        pass.colorAttachments[1].texture=nil;pass.colorAttachments[1].resolveTexture=nil;pass.colorAttachments[0].resolveTexture=target;
        [aliasA release];[aliasB release];[shared release];[second release];
        // Clear-only ends with the clear operation before resolve. A Load
        // pass can resolve previously stored samples without a new draw.
        for(unsigned phase=0;phase<4;++phase){@autoreleasepool{
            pass.colorAttachments[0].loadAction=phase==1?MTLLoadActionLoad:MTLLoadActionClear;
            auto command=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];auto encoder=[command renderCommandEncoderWithDescriptor:pass];assert(encoder);
            [encoder endEncoding];assert(command->_operations.count==(phase==1?1:2));
            unsigned priorResolve=resolveCalls;failGPU=phase==2;failResolve=phase==3;[command commit];[command waitUntilCompleted];
            assert(command.status==(phase<2?MTLCommandBufferStatusCompleted:MTLCommandBufferStatusError));
            assert(command.sweetGPUOperations==(phase<2?(phase==1?1:2):phase==3?1:0));
            if(phase==2)assert(resolveCalls==priorResolve);failGPU=false;failResolve=false;[command release];
        }}
        [pass release];[source release];[target release];[d release];[r release];
        dispatch_sync(((SweetMTLCommandQueue *)queue)->_execution, ^{});
    }}
    assert(multisamplePages.empty() && !device.currentAllocatedSize);
    for(unsigned samples=2;samples<=4;samples*=2){@autoreleasepool{
        auto d=ms_descriptor(MTLPixelFormatBGRA8Unorm,samples,MTLStorageModeMemoryless),r=ms_descriptor(MTLPixelFormatBGRA8Unorm,1,MTLStorageModeShared);
        auto original=[device newTextureWithDescriptor:d],a=[device newTextureWithDescriptor:r],b=[device newTextureWithDescriptor:r];
        assert(original && !original.buffer && original.sampleCount==samples && !original.allocatedSize);auto other=[device newCommandQueue];
        auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=original;pass.colorAttachments[0].resolveTexture=a;
        pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStoreAndMultisampleResolve;
        auto ca=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];assert(![ca renderCommandEncoderWithDescriptor:pass]);
        pass.colorAttachments[0].storeAction=MTLStoreActionMultisampleResolve;auto ea=[ca renderCommandEncoderWithDescriptor:pass];assert(ea);
        pass.colorAttachments[0].resolveTexture=b;auto cb=(SweetMTLCommandBuffer *)[[other commandBuffer] retain];auto eb=[cb renderCommandEncoderWithDescriptor:pass];assert(eb);
        SweetMTLMemorylessPassInfo ia={},ib={};auto ba=SweetMTLRetainMemorylessPassBuffer(ea,0,&ia),bb=SweetMTLRetainMemorylessPassBuffer(eb,0,&ib);
        assert(ba && bb && ia.handle!=ib.handle && ia.gpu_address!=ib.gpu_address && ia.pass_id!=ib.pass_id && ia.pitch>=17*4*samples);
        [ba release];[bb release];[ea endEncoding];[eb endEncoding];[pass release];[original release];[a release];[b release];[d release];[r release];
        [ca commit];[cb commit];[ca waitUntilCompleted];[cb waitUntilCompleted];
        assert(ca.status==MTLCommandBufferStatusCompleted && cb.status==MTLCommandBufferStatusCompleted && ca.sweetGPUOperations==2 && cb.sweetGPUOperations==2);
        // Terminal status retires operations before its publication. The
        // serial execution block's autorelease pool may drain immediately
        // afterwards; join both queues before asserting the final host count.
        dispatch_sync(((SweetMTLCommandQueue *)queue)->_execution, ^{});
        dispatch_sync(((SweetMTLCommandQueue *)other)->_execution, ^{});
        assert(multisamplePages.empty() && !device.currentAllocatedSize);[ca release];[cb release];[other release];
    }}
    [queue release];[vertex release];[fragment release];[library release];[device release];assert(createdPrograms==retiredPrograms);
    puts("Native multisample objects PASS: real 2x/4x layouts, original Private pages, exact sample-mask snapshots, clear-only/Load resolve order, overlapping-target rejection, Memoryless queue isolation, GPU/resolve failure and caller-release retirement; host GPU stub only");
}return 0;}
