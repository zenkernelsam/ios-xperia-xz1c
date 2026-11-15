/* Actual native MRT objects/snapshots/lifetime; GPU transport is host-stubbed. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <cassert>
#include <map>
static std::map<uint32_t,SweetMetalBacking> pages;
static uint32_t nextHandle=1,draws,programs,retired;
struct SweetMetalProgram{SweetRenderPipelineConfig config;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t n,SweetMetalBacking *out){auto span=sweet_metal_backing_span(n);void *p=nullptr;if(!span || posix_memalign(&p,16384,span))return -ENOMEM;
    *out={n,span,UINT64_C(0x40000000)+nextHandle*65536,p,nextHandle++,0};pages[out->handle]=*out;return 0;}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(pages.count(handle));free(pages[handle].contents);pages.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t h){return pages.count(h)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -ENOTSUP;}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *p){if(p){delete p;++retired;}}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *v,const char *f,const SweetRenderPipelineConfig *c,char *error,size_t n)
{if(error && n)error[0]=0;assert(v && f && c && c->color_mask==3 && c->vertex_buffers==1 && !c->fragment_buffers && !c->reserved);
    assert(strstr(f,"layout(location=0) out vec4 _qcolor;") && strstr(f,"layout(location=1) out vec4 _qcolor1;") && !strstr(f,"_smtl_xr_pack"));
    assert(c->format==MTLPixelFormatBGR10_XR_sRGB && c->blend_enabled && c->colors[0].format==MTLPixelFormatBGRA8Unorm && c->colors[0].write_mask==MTLColorWriteMaskGreen);
    ++programs;return new SweetMetalProgram{*c};}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *p,const SweetRenderPass *pass,const SweetRenderDraw *draw,const SweetShaderBufferBinding *buffers,uint32_t n,
    const SweetShaderTextureBinding *,uint32_t textures,const SweetShaderSamplerBinding *,uint32_t samplers,uint32_t *seq)
{
    assert(p && pass && draw && n==1 && buffers[0].index==0 && !textures && !samplers && pass->color_mask==3 && !pass->reserved);
    auto a=sweet_render_attachment(pass,0),b=sweet_render_attachment(pass,1);assert(a.target.handle==b.target.handle && pages.count(a.target.handle));
    assert(a.target.offset==64 && b.target.offset==1280 && a.target.format==555 && b.target.format==80 && b.target.index==1 && b.target.pitch==128);
    assert(a.load_action==MTLLoadActionLoad && b.load_action==MTLLoadActionClear && b.clear[2]==1);
    assert(draw->blend_color[0]==.25f && draw->blend_color[3]==.5f && draw->count==3);*seq=++draws;return 0;
}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *,uint32_t *){return -ENOTSUP;}
static void exception(void(^body)(void)){bool threw=false;@try{body();}@catch(NSException *e){(void)e;threw=true;}assert(threw);}
int main(int argc,char **argv){assert(argc==2);@autoreleasepool{
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);
    NSData *source=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];assert(source);dispatch_data_t data=dispatch_data_create(source.bytes,source.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);assert(library && !error);
    id<MTLFunction> vs=[library newFunctionWithName:@"mrt_position"],fs=[library newFunctionWithName:@"mrt_colour"];
    MTLRenderPipelineDescriptor *descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=vs;descriptor.fragmentFunction=fs;
    descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatBGR10_XR_sRGB;descriptor.colorAttachments[0].blendingEnabled=YES;
    descriptor.colorAttachments[1].pixelFormat=MTLPixelFormatBGRA8Unorm;descriptor.colorAttachments[1].writeMask=MTLColorWriteMaskGreen;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];assert(pipeline && !error);
    descriptor.colorAttachments[1].pixelFormat=MTLPixelFormatInvalid;assert(![device newRenderPipelineStateWithDescriptor:descriptor error:&error] && error);error=nil;
    id<MTLBuffer> raw=[device newBufferWithLength:2496 options:0],vertices=[device newBufferWithLength:48 options:0];
    MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=17;d.height=9;d.pixelFormat=MTLPixelFormatBGR10_XR_sRGB;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;
    id<MTLTexture> a=[raw newTextureWithDescriptor:d offset:64 bytesPerRow:128];d.pixelFormat=MTLPixelFormatBGRA8Unorm;id<MTLTexture> b=[raw newTextureWithDescriptor:d offset:1280 bytesPerRow:128];assert(a && b);
    id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];
    pass.colorAttachments[0].texture=a;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    pass.colorAttachments[1].texture=b;pass.colorAttachments[1].loadAction=MTLLoadActionClear;pass.colorAttachments[1].storeAction=MTLStoreActionStore;pass.colorAttachments[1].clearColor=MTLClearColorMake(0,0,1,1);
    id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];
    [encoder setBlendColorRed:.25f green:0 blue:1 alpha:.5f];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];
    [pass release];[a release];[b release];[raw release];[vertices release];[descriptor release];[d release];[pipeline release];[vs release];[fs release];[library release];
    assert(device.currentAllocatedSize==32768);[commands commit];[commands waitUntilCompleted];uint32_t seq=0,ops=0;assert(SweetMTLCommandResult(commands,&seq,&ops) && seq==1 && ops==1);
    [commands release];assert(draws==1 && programs==retired && pages.empty() && device.currentAllocatedSize==0);
    @autoreleasepool{d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=17;d.height=9;d.pixelFormat=MTLPixelFormatBGRA8Unorm;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;
        raw=[device newBufferWithLength:2496 options:0];a=[raw newTextureWithDescriptor:d offset:64 bytesPerRow:128];b=[raw newTextureWithDescriptor:d offset:128 bytesPerRow:128];
        pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=a;pass.colorAttachments[1].texture=b;commands=[[queue commandBuffer] retain];assert(![commands renderCommandEncoderWithDescriptor:pass]);
        [commands release];[pass release];[a release];[b release];[raw release];[d release];}
    assert(pages.empty() && !device.currentAllocatedSize);[queue release];[device release];
    puts("actual native MRT: separate formats/blend/masks, sameBO nonoverlap, immutable queue snapshots, caller release, overlap rejection and final lifetime PASS; host transport stub only");
}return 0;}
