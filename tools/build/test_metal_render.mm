/* Actual native texture/render methods and AIR lowering; no host GPU claim. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <assert.h>
#include <map>
#include <string>
#include <atomic>
static std::map<uint32_t,SweetMetalBacking> backing;
static std::map<uint32_t,IOSurfaceRef> imported;
static bool surfaceImportFail;
static uint32_t nextHandle=1,sequence;static unsigned programs,destroyed,draws,clears;
struct SweetMetalProgram {std::string vertex,fragment;SweetRenderPipelineConfig config;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *result)
{
    auto span=sweet_metal_backing_span(bytes);void *memory=nullptr;if(!span || posix_memalign(&memory,16384,span))return -ENOMEM;
    *result={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*65536,memory,nextHandle++,0};backing[result->handle]=*result;return 0;
}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(backing.count(handle));if(imported.count(handle))imported.erase(handle);else free(backing[handle].contents);backing.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){return backing.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t identity,uint64_t cpu,uint64_t required,SweetMetalBacking *result){
    memset(result,0,sizeof(*result));if(surfaceImportFail)return -EIO;
    IOSurfaceRef surface=IOSurfaceLookup(identity);assert(surface && IOSurfaceGetBaseAddress(surface)==(void *)(uintptr_t)cpu);
    uint64_t size=IOSurfaceGetBytesPerRow(surface)*IOSurfaceGetHeight(surface);assert(required<=size);
    *result={size,size,UINT64_C(0x40000040)+uint64_t(nextHandle)*65536,(void *)(uintptr_t)cpu,nextHandle++,0};
    backing[result->handle]=*result;imported[result->handle]=surface;CFRelease(surface);return 0;
}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){assert(!"render test does not blit");return -EINVAL;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){assert(!"render test does not compute");return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -EINVAL;}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){if(program){delete program;++destroyed;}}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *vs,const char *fs,const SweetRenderPipelineConfig *config,char *error,size_t errorBytes)
{
    if(error && errorBytes)error[0]=0;assert(vs && fs && config && config->vertex_buffers==1 && !config->fragment_buffers);
    assert(strstr(vs,"#version 310 es\n") && strstr(vs,"gl_Position.y = -gl_Position.y") && strstr(vs,"2.0*gl_Position.z - gl_Position.w"));
    assert(!strstr(vs,"readonly buffer QBuffer") && !strstr(vs,"_qbuffer0.data") && strstr(vs,"_smtl_vertex_buffer0_load(_q_word(") &&
        strstr(vs,"if(word>=span.y)return 0u") && strstr(vs,"uniform highp usampler2D _smtl_vertex_buffer0"));
    assert(strstr(fs,"#version 310 es\n") && strstr(fs,"out vec4 _qcolor"));
    if(strstr(fs,"_qsample")){
        assert(strstr(vs,"_smtl_vary_") && strstr(fs,"_smtl_vary_") && !strstr(vs,"_qvarying") && !strstr(fs,"_qvarying"));
        assert(!strstr(fs,"layout(binding=") && strstr(fs,"precision highp sampler2D;") && strstr(fs,"_qsample0_3"));
        assert(config->fragment_texture_slots==0x7654321du);
        auto semantic=[&](const char *text,const char *kind){auto line=std::string(text);auto at=line.find(kind);assert(at!=std::string::npos);at=line.find("_smtl_vary_",at);assert(at!=std::string::npos);return line.substr(at,line.find(';',at)-at);};
        assert(semantic(vs,"out vec2 ")==semantic(fs,"in vec2 ") && semantic(vs,"out vec4 ")==semantic(fs,"in vec4 "));
    }
    ++programs;return new SweetMetalProgram{vs,fs,*config};
}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t textureCount,
    const SweetShaderSamplerBinding *samplers,uint32_t samplerCount,uint32_t *fence)
{
    assert(program && pass && draw && count==1 && buffers[0].index==0 && backing.count(buffers[0].handle));
    assert(backing.count(pass->target.handle) && pass->target.offset==64 && pass->target.pitch==128 && pass->target.width==17 && pass->target.height==9);
    assert(draw->count==3 && draw->instances==2 && draw->scissor_y==2 && draw->scissor_height==5);
    assert(draw->winding==1 && draw->cull==2);assert(pass->load_action==(draws==0?2:1));
    assert(textureCount==1 && samplerCount==1 && textures[0].index==13 && textures[0].stage==2 && textures[0].width==7);
    assert(textures[0].offset==128 && textures[0].pitch==64 && backing.count(textures[0].handle));
    assert(samplers[0].index==3 && samplers[0].stage==2 && samplers[0].min_filter==1);
    ++draws;*fence=++sequence;return 0;
}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *pass,uint32_t *fence){assert(pass->load_action==2 && backing.count(pass->target.handle));++clears;*fence=++sequence;return 0;}
static unsigned mipCalls;static bool mipFail;
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *texture,uint32_t *fence)
{
    assert(texture && texture->mip_count==5 && !texture->reserved && texture->width==17 && texture->height==9);
    assert(backing.count(texture->handle));unsigned width=8,height=4;uint64_t offset=texture->mips[0].offset;
    for(unsigned level=0;level<4;++level){auto &mip=texture->mips[level];assert(backing.count(mip.handle) && mip.width==width && mip.height==height && mip.pitch==64 && mip.offset==offset);
        offset+=height*64;width=width>1?width/2:1;height=height>1?height/2:1;}
    ++mipCalls;*fence=mipFail?0:1234;return mipFail?-EIO:0;
}
static void exception(void(^action)(void)){bool raised=false;@try{action();}@catch(NSException *e){(void)e;raised=true;}assert(raised);}
static MTLTextureDescriptor *descriptor(unsigned width,unsigned height)
{
    auto result=[MTLTextureDescriptor new];result.textureType=MTLTextureType2D;result.width=width;result.height=height;
    result.storageMode=MTLStorageModeShared;result.pixelFormat=MTLPixelFormatBGRA8Unorm;result.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;return result;
}
static void textureRegions(id<MTLDevice> device)
{
    @autoreleasepool{
        auto desc=descriptor(17,9);id<MTLBuffer> buffer=[device newBufferWithLength:1280 options:0];assert(buffer);auto original=(unsigned char *)buffer.contents;memset(original,0xa5,1280);
        id<MTLTexture> texture=[buffer newTextureWithDescriptor:desc offset:64 bytesPerRow:128];assert(texture && texture.buffer==buffer && texture.bufferBytesPerRow==128);
        assert(![buffer newTextureWithDescriptor:desc offset:63 bytesPerRow:128] && ![buffer newTextureWithDescriptor:desc offset:64 bytesPerRow:64]);
        assert(![buffer newTextureWithDescriptor:desc offset:192 bytesPerRow:128]);
        unsigned char source[96],output[160];for(unsigned i=0;i<96;++i)source[i]=(unsigned char)(i*37);memset(output,0xe7,sizeof(output));
        MTLRegion region=MTLRegionMake2D(1,2,5,3);[texture replaceRegion:region mipmapLevel:0 slice:0 withBytes:source bytesPerRow:32 bytesPerImage:84];
        [texture getBytes:output+8 bytesPerRow:48 bytesPerImage:116 fromRegion:region mipmapLevel:0 slice:0];
        for(unsigned i=0;i<1280;++i){bool copied=false;for(unsigned y=0;y<3;++y){unsigned start=64+(y+2)*128+4;
                if(i>=start && i<start+20){assert(original[i]==source[y*32+i-start]);copied=true;}}
            if(!copied)assert(original[i]==0xa5);}
        for(unsigned i=0;i<sizeof(output);++i){bool copied=false;for(unsigned y=0;y<3;++y)if(i>=8+y*48 && i<28+y*48){assert(output[i]==source[y*32+i-8-y*48]);copied=true;}
            if(!copied)assert(output[i]==0xe7);}
        const unsigned char *sourcePointer=source;unsigned char *outputPointer=output;
        exception(^{[texture replaceRegion:region mipmapLevel:0 withBytes:sourcePointer bytesPerRow:19];});
        exception(^{[texture getBytes:outputPointer bytesPerRow:48 fromRegion:MTLRegionMake2D(16,8,2,1) mipmapLevel:0];});
        exception(^{[texture replaceRegion:region mipmapLevel:1 withBytes:sourcePointer bytesPerRow:32];});
        exception(^{[texture replaceRegion:region mipmapLevel:0 slice:1 withBytes:sourcePointer bytesPerRow:32 bytesPerImage:84];});
        exception(^{[texture getBytes:outputPointer bytesPerRow:48 bytesPerImage:115 fromRegion:region mipmapLevel:0 slice:0];});
        [buffer release];assert(texture.device==device && texture.buffer.contents==original);
        [texture release];desc.storageMode=MTLStorageModePrivate;id<MTLTexture> privateTexture=[device newTextureWithDescriptor:desc];assert(privateTexture && !privateTexture.buffer);
        exception(^{[privateTexture getBytes:outputPointer bytesPerRow:48 fromRegion:region mipmapLevel:0];});[privateTexture release];
        desc.storageMode=MTLStorageModeShared;desc.width=0;assert(![device newTextureWithDescriptor:desc]);[desc release];
    }
    assert(device.currentAllocatedSize==0 && backing.empty());
}
static void surfaceRegions(id<MTLDevice> device)
{
    @autoreleasepool{
        NSDictionary *properties=@{(id)kIOSurfaceWidth:@17,(id)kIOSurfaceHeight:@9,(id)kIOSurfaceBytesPerElement:@4,
            (id)kIOSurfaceBytesPerRow:@128,(id)kIOSurfaceAllocSize:@1152,(id)kIOSurfacePixelFormat:@0x42475241};
        IOSurfaceRef surface=IOSurfaceCreate((CFDictionaryRef)properties);assert(surface && IOSurfaceGetWidth(surface)==17);
        auto desc=descriptor(17,9);auto original=(unsigned char *)IOSurfaceGetBaseAddress(surface);assert(original && !((uintptr_t)original&63));
        assert(IOSurfaceLock(surface,0,nullptr)==KERN_SUCCESS);memset(original,0xa5,1152);assert(IOSurfaceUnlock(surface,0,nullptr)==KERN_SUCCESS);
        surfaceImportFail=true;assert(![device newTextureWithDescriptor:desc iosurface:surface plane:0] && backing.empty());surfaceImportFail=false;
        assert(![device newTextureWithDescriptor:desc iosurface:surface plane:1]);desc.width=18;assert(![device newTextureWithDescriptor:desc iosurface:surface plane:0]);desc.width=17;
        desc.pixelFormat=MTLPixelFormatRGBA16Float;assert(![device newTextureWithDescriptor:desc iosurface:surface plane:0]);desc.pixelFormat=MTLPixelFormatBGRA8Unorm;
        desc.hazardTrackingMode=(MTLHazardTrackingMode)3;assert(![device newTextureWithDescriptor:desc iosurface:surface plane:0]);desc.hazardTrackingMode=MTLHazardTrackingModeDefault;
        id<MTLTexture> texture=[device newTextureWithDescriptor:desc iosurface:surface plane:0];assert(texture && !texture.buffer && texture.iosurface==surface && texture.device==device);
        assert(texture.allocatedSize==1152 && device.currentAllocatedSize==1152 && texture.bufferOffset==0 && texture.bufferBytesPerRow==128);
        CFRelease(surface);surface=nullptr;assert(IOSurfaceGetBaseAddress(texture.iosurface)==original);
        unsigned char source[96],output[160];for(unsigned i=0;i<96;++i)source[i]=(unsigned char)(i*37);memset(output,0xe7,sizeof(output));
        auto region=MTLRegionMake2D(1,2,5,3);[texture replaceRegion:region mipmapLevel:0 withBytes:source bytesPerRow:32];
        [texture getBytes:output+8 bytesPerRow:48 fromRegion:region mipmapLevel:0];
        for(unsigned i=0;i<1152;++i){unsigned row=i/128,column=i%128;unsigned char expected=0xa5;
            if(row>=2 && row<5 && column>=4 && column<24)expected=source[(row-2)*32+column-4];assert(original[i]==expected);}
        for(unsigned i=0;i<160;++i){unsigned char expected=0xe7;if(i>=8){unsigned row=(i-8)/48,column=(i-8)%48;
            if(row<3 && column<20)expected=source[row*32+column];}assert(output[i]==expected);}
        desc.storageMode=MTLStorageModePrivate;desc.mipmapLevelCount=5;id<MTLTexture> privateTexture=[device newTextureWithDescriptor:desc iosurface:texture.iosurface plane:0];assert(privateTexture && privateTexture.storageMode==MTLStorageModePrivate && privateTexture.mipmapLevelCount==5);
        const unsigned char *input=source;unsigned char *out=output;
        exception(^{[privateTexture replaceRegion:region mipmapLevel:0 withBytes:input bytesPerRow:32];});
        exception(^{[privateTexture getBytes:out bytesPerRow:48 fromRegion:region mipmapLevel:0];});
        [texture release];assert(IOSurfaceGetBaseAddress(privateTexture.iosurface)==original);
        id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];auto encoder=[commands blitCommandEncoder];
        [encoder generateMipmapsForTexture:privateTexture];[encoder endEncoding];[privateTexture release];[commands commit];[commands waitUntilCompleted];
        uint32_t fence=0,operations=0;assert(SweetMTLCommandResult(commands,&fence,&operations) && fence==1234 && operations==4);[commands release];[queue release];[desc release];
    }
    assert(device.currentAllocatedSize==0 && backing.empty() && imported.empty());
    puts("actual host IOSurface texture: native pages, regions/padding, caller release, private CPU rejection, import rollback and final lifetime PASS; GPU transport stub only");
}
static void mipmapObjects(id<MTLDevice> device)
{
    unsigned before=mipCalls;
    @autoreleasepool{
        auto d=descriptor(17,9);d.mipmapLevelCount=5;id<MTLBuffer> raw=[device newBufferWithLength:1728 options:0];memset(raw.contents,0xa5,1728);
        id<MTLTexture> texture=[raw newTextureWithDescriptor:d offset:64 bytesPerRow:128];assert(texture && texture.mipmapLevelCount==5);
        d.mipmapLevelCount=6;assert(![device newTextureWithDescriptor:d]);d.mipmapLevelCount=5;
        unsigned char upload[32],output[64];for(unsigned i=0;i<32;++i)upload[i]=(unsigned char)(i*7);memset(output,0xe7,64);
        [texture replaceRegion:MTLRegionMake2D(0,0,4,2) mipmapLevel:2 withBytes:upload bytesPerRow:16];
        [texture getBytes:output bytesPerRow:32 fromRegion:MTLRegionMake2D(0,0,4,2) mipmapLevel:2];
        auto bytes=(unsigned char *)raw.contents;for(unsigned i=0;i<1728;++i){unsigned char expected=0xa5;
            if(i>=1472 && i<1600){unsigned row=(i-1472)/64,column=(i-1472)%64;if(column<16)expected=upload[row*16+column];}assert(bytes[i]==expected);}
        for(unsigned i=0;i<64;++i)assert(output[i]==(i%32<16?upload[i/32*16+i%32]:0xe7));
        const unsigned char *input=upload;exception(^{[texture replaceRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:5 withBytes:input bytesPerRow:4];});
        auto filter=[MTLSamplerDescriptor new];filter.mipFilter=MTLSamplerMipFilterLinear;id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:filter];assert(sampler);[sampler release];[filter release];
        id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];auto encoder=[commands blitCommandEncoder];
        [encoder generateMipmapsForTexture:texture];[encoder endEncoding];exception(^{[encoder generateMipmapsForTexture:texture];});
        [texture release];[raw release];assert(device.currentAllocatedSize==16384);[commands commit];[commands waitUntilCompleted];
        uint32_t fence=0,operations=0;assert(SweetMTLCommandResult(commands,&fence,&operations) && fence==1234 && operations==4 && mipCalls==before+1);
        [commands release];assert(backing.empty() && device.currentAllocatedSize==0);
        d.storageMode=MTLStorageModePrivate;texture=[device newTextureWithDescriptor:d];assert(texture && !texture.buffer && texture.storageMode==MTLStorageModePrivate);
        commands=[[queue commandBuffer] retain];encoder=[commands blitCommandEncoder];[encoder generateMipmapsForTexture:texture];[encoder endEncoding];[texture release];
        mipFail=true;[commands commit];[commands waitUntilCompleted];mipFail=false;assert(commands.status==MTLCommandBufferStatusError && commands.error && !SweetMTLCommandResult(commands,&fence,&operations));
        [commands release];[queue release];[d release];
    }assert(backing.empty() && device.currentAllocatedSize==0);puts("actual native mip objects: chain bounds/regions, retained queue snapshot, private texture CPU denial and GPU failure retirement PASS; transport stub only");
}
int main(int argc,char **argv){assert(argc==2);@autoreleasepool{
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);textureRegions(device);surfaceRegions(device);mipmapObjects(device);
    NSData *bytes=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];assert(bytes);
    dispatch_data_t data=dispatch_data_create(bytes.bytes,bytes.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);assert(library && !error && library.functionNames.count==4);
    id<MTLFunction> vertex=[library newFunctionWithName:@"texture_position"],fragment=[library newFunctionWithName:@"textured"];assert(vertex && fragment);
    auto desc=[MTLRenderPipelineDescriptor new];desc.vertexFunction=vertex;desc.fragmentFunction=fragment;desc.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    assert(![device newRenderPipelineStateWithDescriptor:nil error:&error] && error);error=nil;
    desc.rasterSampleCount=4;assert(![device newRenderPipelineStateWithDescriptor:desc error:&error] && error);desc.rasterSampleCount=1;error=nil;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:desc error:&error];
    if(!pipeline)fprintf(stderr,"render pipeline error=%s\n",error.localizedDescription.UTF8String);assert(pipeline && !error);[desc release];
    id<MTLCommandQueue> queue=[device newCommandQueue];auto targetDesc=descriptor(17,9),sampleDesc=descriptor(7,5);
    id<MTLBuffer> targetBuffer=[device newBufferWithLength:1280 options:0],sampleBuffer=[device newBufferWithLength:512 options:0],vertices=[device newBufferWithLength:192 options:0];
    id<MTLTexture> target=[targetBuffer newTextureWithDescriptor:targetDesc offset:64 bytesPerRow:128],sample=[sampleBuffer newTextureWithDescriptor:sampleDesc offset:128 bytesPerRow:64];assert(target && sample);
    auto samplerDesc=[MTLSamplerDescriptor new];samplerDesc.minFilter=MTLSamplerMinMagFilterLinear;samplerDesc.magFilter=MTLSamplerMinMagFilterLinear;
    id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:samplerDesc];assert(sampler);samplerDesc.normalizedCoordinates=NO;assert(![device newSamplerStateWithDescriptor:samplerDesc]);[samplerDesc release];
    auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];auto invalid=[MTLRenderPassDescriptor new];assert(![commands renderCommandEncoderWithDescriptor:invalid]);[invalid release];
    id<MTLRenderCommandEncoder> encoder=[[commands renderCommandEncoderWithDescriptor:pass] retain];assert(encoder && ![commands blitCommandEncoder]);
    [encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder setFragmentTexture:sample atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];
    [encoder setScissorRect:(MTLScissorRect){1,2,13,5}];[encoder setFrontFacingWinding:MTLWindingCounterClockwise];[encoder setCullMode:MTLCullModeBack];
    exception(^{[encoder setViewport:(MTLViewport){0,0,17,10,0,1}];});exception(^{[encoder setVertexBuffer:vertices offset:1 atIndex:0];});
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3 instanceCount:2];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3 instanceCount:2];[encoder endEncoding];exception(^{[encoder endEncoding];});
    [pass release];[encoder release];[vertices release];[target release];[sample release];[targetBuffer release];[sampleBuffer release];[targetDesc release];[sampleDesc release];
    [pipeline release];[vertex release];[fragment release];[library release];[sampler release];assert(device.currentAllocatedSize==3*16384);
    [commands commit];[commands waitUntilCompleted];uint32_t fence=0,operations=0;assert(SweetMTLCommandResult(commands,&fence,&operations) && operations==2 && fence==2);
    assert(draws==2 && programs==1 && destroyed==1 && device.currentAllocatedSize==0 && backing.empty());[commands release];
    @autoreleasepool{auto d=descriptor(17,9);id<MTLTexture> t=[device newTextureWithDescriptor:d];assert(t && t.bufferBytesPerRow==128);[d release];
        auto p=[MTLRenderPassDescriptor new];p.colorAttachments[0].texture=t;p.colorAttachments[0].loadAction=MTLLoadActionClear;p.colorAttachments[0].storeAction=MTLStoreActionStore;
        id<MTLCommandBuffer> c=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> e=[c renderCommandEncoderWithDescriptor:p];assert(e);[e endEncoding];[t release];[p release];
        [c commit];[c waitUntilCompleted];assert(c.status==MTLCommandBufferStatusCompleted && !c.error && clears==1);[c release];}
    assert(device.currentAllocatedSize==0 && backing.empty());[queue release];[device release];
    puts("actual Metal render objects: stride/region guards, AIR semantic linking, snapshots, owned textures, clear, factory rollback and final lifetime PASS");
}return 0;}
