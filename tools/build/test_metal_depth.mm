/* Real native Metal depth descriptors/resources/encoder snapshots. GPU is stubbed. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <cassert>
#include <map>
#include <vector>
#include "MultisampleBackendHostStub.h"
static std::map<uint32_t,SweetMetalBacking> pages;
static std::vector<SweetRenderDepthPass> depthDraws;
static std::vector<SweetRenderStencilPass> stencilDraws;
static std::vector<bool> stencilHasDepth;
static unsigned stencilClears;static bool stencilSupported=true;
static uint32_t nextHandle=1,sequence;static unsigned programs,retired,clears;static bool supported=true;
struct SweetMetalProgram{SweetRenderPipelineConfig config;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_depth32_supported(void){return supported;}
extern "C" int sweetgl_metal_stencil8_supported(void){return stencilSupported;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{auto span=sweet_metal_backing_span(bytes);void *p=nullptr;if(!span || posix_memalign(&p,16384,span))return -ENOMEM;
    *out={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*65536,p,nextHandle++,0};pages[out->handle]=*out;return 0;}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(pages.count(handle));free(pages[handle].contents);pages.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){return pages.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){assert(!"depth test does not blit");return -EINVAL;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){assert(!"depth test does not compute");return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -EINVAL;}
extern "C" int sweetgl_metal_program_cache_state(const SweetMetalProgram *){return 0;}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){if(program){delete program;++retired;}}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,char *,size_t)
{assert(vertex && fragment && config && config->format==80 && config->vertex_buffers==1 && config->fragment_texture_slots==0x7654321du);++programs;return new SweetMetalProgram{*config};}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,
    const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){assert(!"Missing original depth must not use color-only draw");return -EINVAL;}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *,uint32_t *){assert(!"Missing original depth must not use color-only clear");return -EINVAL;}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -EINVAL;}
extern "C" int sweetgl_metal_render_draw_depth(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t n,const SweetShaderTextureBinding *textures,uint32_t t,const SweetShaderSamplerBinding *,uint32_t s,uint32_t *fence)
{
    assert(program && pass && depth && draw && n==1 && !t && !s && buffers && !textures[0].handle);
    assert(pages.count(pass->target.handle) && pages.count(depth->target.handle) && depth->target.format==252 && depth->target.width==17 && depth->target.height==9);
    assert(depth->target.pitch==128 && depth->target.offset==64 && depth->target.usage==4 && draw->count==3);
    assert(sweet_depth_stencil_valid(&depth->state));depthDraws.push_back(*depth);*fence=++sequence;return 0;
}
extern "C" int sweetgl_metal_render_clear_depth(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,uint32_t *fence)
{assert(pass && depth && depth->load_action==2 && depth->clear_depth==0.75 && pages.count(depth->target.handle));++clears;*fence=++sequence;return 0;}
extern "C" int sweetgl_metal_render_draw_stencil(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t n,const SweetShaderTextureBinding *,uint32_t t,const SweetShaderSamplerBinding *,uint32_t s,uint32_t *fence)
{
    assert(program && pass && stencil && draw && n==1 && buffers && !t && !s && pages.count(pass->target.handle));
    assert(pages.count(stencil->target.handle) && stencil->target.format==253 && stencil->target.pitch==64 && stencil->target.offset==64 && stencil->target.usage==4);
    assert(!stencil->reserved && sweet_depth_stencil_valid(&stencil->state));
    if(depth){assert(pages.count(depth->target.handle) && depth->target.format==252);assert(!memcmp(&depth->state,&stencil->state,sizeof(depth->state)));}
    stencilHasDepth.push_back(depth!=nullptr);stencilDraws.push_back(*stencil);*fence=++sequence;return 0;
}
extern "C" int sweetgl_metal_render_clear_stencil(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,uint32_t *fence)
{assert(pass && stencil && stencil->load_action==2 && stencil->clear_stencil==0xab && pages.count(stencil->target.handle));
    assert(!depth || depth->load_action==MTLLoadActionLoad);++stencilClears;*fence=++sequence;return 0;}
static void expectThrow(void(^action)(void)){bool thrown=false;@try{action();}@catch(id error){(void)error;thrown=true;}assert(thrown);}
static MTLTextureDescriptor *descriptor(MTLPixelFormat format,unsigned width,unsigned height)
{auto d=[MTLTextureDescriptor new];d.pixelFormat=format;d.width=width;d.height=height;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;return d;}
static void stateObjects(id<MTLDevice> device)
{
    auto d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=MTLCompareFunctionLessEqual;d.depthWriteEnabled=YES;d.label=@"original depth";
    auto front=[MTLStencilDescriptor new];front.stencilCompareFunction=MTLCompareFunctionEqual;front.stencilFailureOperation=MTLStencilOperationInvert;
    front.depthFailureOperation=MTLStencilOperationIncrementWrap;front.depthStencilPassOperation=MTLStencilOperationDecrementWrap;
    front.readMask=0x1234;front.writeMask=0xffaa;d.frontFaceStencil=front;[front release];d.backFaceStencil=nil;
    id<MTLDepthStencilState> state=[device newDepthStencilStateWithDescriptor:d];assert(state && state.device==device && [state.label isEqualToString:@"original depth"]);
    auto actual=((SweetMTLDepthStencilState *)state)->_state;assert(actual.compare==3 && actual.write && actual.front.enabled && actual.back.enabled);
    // Apple descriptors recreate a default back-face descriptor after nil is
    // assigned; capture the getter's actual Always/Keep values, not the setter.
    assert(actual.back.compare==7 && actual.back.fail==0 && actual.back.depth_fail==0 && actual.back.pass==0);
    assert(actual.front.compare==2 && actual.front.fail==5 && actual.front.depth_fail==6 && actual.front.pass==7 && actual.front.read_mask==0x34 && actual.front.write_mask==0xaa);
    d.depthCompareFunction=MTLCompareFunctionAlways;d.depthWriteEnabled=NO;d.label=@"changed";
    assert(((SweetMTLDepthStencilState *)state)->_state.compare==3 && [state.label isEqualToString:@"original depth"]);
    assert(![device newDepthStencilStateWithDescriptor:nil]);d.depthCompareFunction=(MTLCompareFunction)8;assert(![device newDepthStencilStateWithDescriptor:d]);
    [state release];[d release];
}
static void depthResources(id<MTLDevice> device)
{
    auto d=descriptor(MTLPixelFormatDepth32Float,17,9);auto buffer=[device newBufferWithLength:1280 options:0];memset(buffer.contents,0xa5,1280);
    id<MTLTexture> depth=[buffer newTextureWithDescriptor:d offset:64 bytesPerRow:128];assert(depth && depth.pixelFormat==252 && depth.buffer==buffer);
    unsigned char source[32],out[64];memset(source,0x5a,32);memset(out,0xe7,64);auto region=MTLRegionMake2D(2,3,4,2);
    [depth replaceRegion:region mipmapLevel:0 withBytes:source bytesPerRow:16];[depth getBytes:out+8 bytesPerRow:24 fromRegion:region mipmapLevel:0];
    auto original=(unsigned char *)buffer.contents;for(unsigned i=0;i<1280;++i){bool copied=(i>=456 && i<472)||(i>=584 && i<600);assert(original[i]==(copied?0x5a:0xa5));}
    for(unsigned i=0;i<64;++i){bool copied=(i>=8 && i<24)||(i>=32 && i<48);assert(out[i]==(copied?0x5a:0xe7));}
    d.mipmapLevelCount=2;assert(![device newTextureWithDescriptor:d]);d.mipmapLevelCount=1;d.usage=MTLTextureUsageShaderRead;assert(![device newTextureWithDescriptor:d]);d.usage=MTLTextureUsageRenderTarget;
    supported=false;assert(![device newTextureWithDescriptor:d]);supported=true;d.storageMode=MTLStorageModePrivate;
    id<MTLTexture> privateDepth=[device newTextureWithDescriptor:d];assert(privateDepth && !privateDepth.buffer);
    void *output=out;expectThrow(^{[privateDepth getBytes:output bytesPerRow:24 fromRegion:region mipmapLevel:0];});[privateDepth release];
    [depth release];[buffer release];[d release];assert(pages.empty() && !device.currentAllocatedSize);
}
static void stencilResources(id<MTLDevice> device)
{
    auto d=descriptor(MTLPixelFormatStencil8,17,9);auto buffer=[device newBufferWithLength:768 options:0];memset(buffer.contents,0xa5,768);
    id<MTLTexture> texture=[buffer newTextureWithDescriptor:d offset:64 bytesPerRow:64];assert(texture && texture.pixelFormat==253 && texture.bufferBytesPerRow==64);
    unsigned char input[12],output[32];memset(input,0x3d,sizeof(input));memset(output,0xe7,sizeof(output));auto region=MTLRegionMake2D(3,2,4,2);
    [texture replaceRegion:region mipmapLevel:0 withBytes:input bytesPerRow:6];[texture getBytes:output+8 bytesPerRow:8 fromRegion:region mipmapLevel:0];
    auto bytes=(unsigned char *)buffer.contents;for(unsigned i=0;i<768;++i){bool touched=(i>=195 && i<199)||(i>=259 && i<263);assert(bytes[i]==(touched?0x3d:0xa5));}
    for(unsigned i=0;i<32;++i){bool touched=(i>=8 && i<12)||(i>=16 && i<20);assert(output[i]==(touched?0x3d:0xe7));}
    d.mipmapLevelCount=2;assert(![device newTextureWithDescriptor:d]);d.mipmapLevelCount=1;
    d.usage=MTLTextureUsageShaderRead;assert(![device newTextureWithDescriptor:d]);d.usage=MTLTextureUsageRenderTarget;
    stencilSupported=false;assert(![device newTextureWithDescriptor:d]);stencilSupported=true;
    d.storageMode=MTLStorageModePrivate;auto privateTexture=[device newTextureWithDescriptor:d];assert(privateTexture && !privateTexture.buffer);
    void *out=output;expectThrow(^{[privateTexture getBytes:out bytesPerRow:8 fromRegion:region mipmapLevel:0];});[privateTexture release];
    [texture release];[buffer release];[d release];assert(pages.empty() && !device.currentAllocatedSize);
}
static void stencilObjects(id<MTLDevice> device,id<MTLLibrary> library,id<MTLFunction> vertex,id<MTLFunction> fragment)
{
    (void)library;stencilResources(device);
    for(unsigned hasDepth=0;hasDepth<2;++hasDepth){@autoreleasepool{
        auto pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        pd.depthAttachmentPixelFormat=hasDepth?MTLPixelFormatDepth32Float:MTLPixelFormatInvalid;pd.stencilAttachmentPixelFormat=MTLPixelFormatStencil8;NSError *error=nil;
        auto pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&error];assert(pipeline && !error);
        auto cd=descriptor(MTLPixelFormatBGRA8Unorm,17,9),dd=descriptor(MTLPixelFormatDepth32Float,17,9),sd=descriptor(MTLPixelFormatStencil8,17,9);
        auto target=[device newTextureWithDescriptor:cd];id<MTLTexture> depth=hasDepth?[device newTextureWithDescriptor:dd]:nil;
        auto sb=[device newBufferWithLength:768 options:0];auto stencil=[sb newTextureWithDescriptor:sd offset:64 bytesPerRow:64];assert(stencil);
        auto vertices=[device newBufferWithLength:192 options:0];auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;
        pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;
        pass.stencilAttachment.texture=stencil;pass.stencilAttachment.loadAction=MTLLoadActionClear;pass.stencilAttachment.storeAction=MTLStoreActionStore;pass.stencilAttachment.clearStencil=0x1ab;
        auto queue=[device newCommandQueue];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];
        pass.stencilAttachment.level=1;assert(![commands renderCommandEncoderWithDescriptor:pass]);pass.stencilAttachment.level=0;
        id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];
        auto stateDesc=[MTLDepthStencilDescriptor new];stateDesc.depthCompareFunction=MTLCompareFunctionLess;stateDesc.depthWriteEnabled=YES;
        stateDesc.frontFaceStencil.stencilCompareFunction=MTLCompareFunctionEqual;stateDesc.frontFaceStencil.depthStencilPassOperation=MTLStencilOperationIncrementWrap;
        stateDesc.frontFaceStencil.readMask=0xf0;stateDesc.frontFaceStencil.writeMask=0x0f;
        stateDesc.backFaceStencil.stencilCompareFunction=MTLCompareFunctionNotEqual;stateDesc.backFaceStencil.depthStencilPassOperation=MTLStencilOperationInvert;
        stateDesc.backFaceStencil.readMask=0x7f;stateDesc.backFaceStencil.writeMask=0xff;
        auto state=[device newDepthStencilStateWithDescriptor:stateDesc];assert(state);[encoder setStencilFrontReferenceValue:0x123 backReferenceValue:0x234];[encoder setDepthStencilState:state];
        unsigned start=stencilDraws.size();[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder setDepthStencilState:nil];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[state release];[stateDesc release];[pass release];[stencil release];[sb release];[depth release];[target release];[vertices release];[pipeline release];[pd release];
        assert(stencilDraws.size()==start);[commands commit];[commands waitUntilCompleted];assert(commands.status==MTLCommandBufferStatusCompleted);
        assert(stencilDraws.size()==start+2 && stencilHasDepth[start]==bool(hasDepth));
        const auto &a=stencilDraws[start];const auto &b=stencilDraws[start+1];assert(a.load_action==2 && a.clear_stencil==0xab && b.load_action==1);
        assert(a.state.front.compare==2 && a.state.front.pass==6 && a.state.front.read_mask==0xf0 && a.state.front.write_mask==15 &&
            a.state.back.compare==5 && a.state.back.pass==5 && a.state.front_reference==0x23 && a.state.back_reference==0x34);
        assert(b.state.compare==7 && !b.state.write && !b.state.front.enabled && !b.state.back.enabled && b.state.front_reference==0x23 && b.state.back_reference==0x34);
        [commands release];assert(pages.empty() && !device.currentAllocatedSize);
        target=[device newTextureWithDescriptor:cd];depth=hasDepth?[device newTextureWithDescriptor:dd]:nil;stencil=[device newTextureWithDescriptor:sd];
        pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.stencilAttachment.texture=stencil;pass.stencilAttachment.loadAction=MTLLoadActionClear;pass.stencilAttachment.clearStencil=0xab;
        commands=[[queue commandBuffer] retain];encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder endEncoding];[pass release];[target release];[depth release];[stencil release];
        [commands commit];[commands waitUntilCompleted];assert(commands.status==MTLCommandBufferStatusCompleted);[commands release];[queue release];[cd release];[dd release];[sd release];
        assert(pages.empty() && !device.currentAllocatedSize);
    }}
    assert(stencilClears==2);puts("Native stencil objects PASS: Stencil8 guards, original front/back compare/ops/masks, independent Load/Clear, optional depth, immutable snapshots, caller release and retirement; host GPU stub only");
}
int main(int argc,char **argv){assert(argc==2);@autoreleasepool{
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);stateObjects(device);depthResources(device);
    NSData *bytes=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];assert(bytes);
    dispatch_data_t data=dispatch_data_create(bytes.bytes,bytes.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);assert(library && !error);
    id<MTLFunction> vertex=[library newFunctionWithName:@"texture_position"],fragment=[library newFunctionWithName:@"textured"];
    auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=vertex;d.fragmentFunction=fragment;d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:d error:&error];assert(pipeline && !error);
    supported=false;assert(![device newRenderPipelineStateWithDescriptor:d error:&error] && error);supported=true;error=nil;
    d.stencilAttachmentPixelFormat=MTLPixelFormatDepth32Float;assert(![device newRenderPipelineStateWithDescriptor:d error:&error]);error=nil;d.stencilAttachmentPixelFormat=MTLPixelFormatInvalid;
    auto td=descriptor(MTLPixelFormatBGRA8Unorm,17,9),dd=descriptor(MTLPixelFormatDepth32Float,17,9);
    id<MTLTexture> target=[device newTextureWithDescriptor:td];auto db=[device newBufferWithLength:1280 options:0];id<MTLTexture> depth=[db newTextureWithDescriptor:dd offset:64 bytesPerRow:128];
    auto vertices=[device newBufferWithLength:192 options:0];auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=0.75;
    id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];
    // Invalid attachment snapshots must fail before an encoder claims the queue.
    pass.depthAttachment.clearDepth=-0.25;assert(![commands renderCommandEncoderWithDescriptor:pass]);pass.depthAttachment.clearDepth=0.75;
    pass.depthAttachment.level=1;assert(![commands renderCommandEncoderWithDescriptor:pass]);pass.depthAttachment.level=0;
    id<MTLTexture> alias=[db newTextureWithDescriptor:td offset:64 bytesPerRow:128];assert(alias);
    pass.colorAttachments[0].texture=alias;assert(![commands renderCommandEncoderWithDescriptor:pass]);pass.colorAttachments[0].texture=target;[alias release];
    dd.width=18;id<MTLTexture> wrongSize=[device newTextureWithDescriptor:dd];assert(wrongSize);
    pass.depthAttachment.texture=wrongSize;assert(![commands renderCommandEncoderWithDescriptor:pass]);pass.depthAttachment.texture=depth;[wrongSize release];dd.width=17;
    id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];
    auto sd=[MTLDepthStencilDescriptor new];sd.depthCompareFunction=MTLCompareFunctionLess;sd.depthWriteEnabled=YES;id<MTLDepthStencilState> state=[device newDepthStencilStateWithDescriptor:sd];
    id<MTLDevice> another=SweetMTLCreateDevice(&error);id<MTLDepthStencilState> foreign=[another newDepthStencilStateWithDescriptor:sd];assert(foreign);
    expectThrow(^{[encoder setDepthStencilState:foreign];});[foreign release];[another release];
    [encoder setStencilFrontReferenceValue:0x123 backReferenceValue:0x234];[encoder setDepthStencilState:state];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [encoder setDepthStencilState:nil];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];
    expectThrow(^{[encoder setDepthStencilState:state];});
    [state release];[sd release];[pass release];[depth release];[db release];[target release];[vertices release];[pipeline release];[d release];
    assert(!depthDraws.size() && device.currentAllocatedSize==3*16384);
    [commands commit];[commands waitUntilCompleted];uint32_t fence=0,operations=0;assert(SweetMTLCommandResult(commands,&fence,&operations) && operations==2);
    assert(depthDraws.size()==2 && depthDraws[0].load_action==2 && depthDraws[1].load_action==1);
    assert(depthDraws[0].state.compare==1 && depthDraws[0].state.write && depthDraws[1].state.compare==7 && !depthDraws[1].state.write);
    assert(depthDraws[1].state.front_reference==0x23 && depthDraws[1].state.back_reference==0x34 && pages.empty() && !device.currentAllocatedSize);
    [commands release];
    @autoreleasepool{target=[device newTextureWithDescriptor:td];depth=[device newTextureWithDescriptor:dd];pass=[MTLRenderPassDescriptor new];
        pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.clearDepth=0.75;pass.depthAttachment.storeAction=MTLStoreActionStore;
        commands=[[queue commandBuffer] retain];encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder endEncoding];[target release];[depth release];[pass release];
        [commands commit];[commands waitUntilCompleted];assert(commands.status==MTLCommandBufferStatusCompleted && clears==1);[commands release];}
    stencilObjects(device,library,vertex,fragment);[vertex release];[fragment release];[library release];
    assert(programs==retired && pages.empty() && !device.currentAllocatedSize);[td release];[dd release];[queue release];[device release];
    puts("Native depth objects PASS: original descriptors/Depth32 pages, guards, support rejection, immutable state/attachment snapshots, caller release, depth-only clear, final retirement; host GPU stub only");
}return 0;}
