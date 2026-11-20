// SPDX-License-Identifier: BSD-2-Clause
// Native Metal objects over the existing linear resource and Mesa contract.
#include "MetalTextureRules.h"
#include <cmath>
#include "../sweetgl/XRColourShader.h"
#include "../sweetgl/MetalMultisampleAPI.h"
#include "../sweetgl/MultisampleLayout.h"
#include <IOSurface/IOSurfaceRef.h>
@interface SweetMTLTexture : NSObject <MTLTexture> {
@public SweetMTLBuffer *_buffer;SweetTextureLayout _layout;MTLPixelFormat _format;MTLTextureUsage _usage;NSString *_label;
    IOSurfaceRef _surface;SweetMTLDevice *_surfaceOwner;SweetMetalBacking _surfaceBacking;MTLResourceOptions _surfaceOptions;
    SweetMTLBuffer *_mipBuffer;SweetTextureLayout _levels[15];uint32_t _levelCount;
    SweetMTLDevice *_memorylessOwner;
    uint32_t _samples;
}
- (instancetype)initWithBuffer:(SweetMTLBuffer *)buffer descriptor:(MTLTextureDescriptor *)descriptor offset:(NSUInteger)offset pitch:(NSUInteger)pitch;
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)descriptor surface:(IOSurfaceRef)surface plane:(NSUInteger)plane;
- (instancetype)initMemorylessWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)descriptor;
- (SweetMTLTexture *)newMemorylessPassBackingForDevice:(SweetMTLDevice *)owner;
- (SweetMetalBacking)sweetBacking;
- (void *)synchronizedContents;
- (SweetTextureLayout)layoutForLevel:(NSUInteger)level;
- (void *)synchronizedContentsForLevel:(NSUInteger)level;
- (SweetShaderTextureBinding)bindingForIndex:(unsigned)index stage:(unsigned)stage;
@end
@interface SweetMTLSampler : NSObject <MTLSamplerState> {
@public SweetMTLDevice *_owner;SweetShaderSamplerBinding _binding;NSString *_label;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLSamplerDescriptor *)descriptor;
@end
@interface SweetMTLRenderPipeline : NSObject <MTLRenderPipelineState> {
@public SweetMTLDevice *_owner;SweetMTLFunction *_vertex,*_fragment;SweetMetalProgram *_program;
    MTLRenderPipelineDescriptor *_descriptor;MTLPixelFormat _depthFormat,_stencilFormat;uint32_t _samples;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLRenderPipelineDescriptor *)descriptor error:(NSError **)error;
@end
@interface SweetMTLRenderEncoder : NSObject <MTLRenderCommandEncoder> {
    SweetMTLCommandBuffer *_commands;SweetMTLTexture *_target;NSArray *_targets;SweetMTLRenderPipeline *_pipeline;
    NSMutableDictionary *_vertexBuffers,*_fragmentBuffers,*_vertexTextures,*_fragmentTextures,*_vertexSamplers,*_fragmentSamplers;
    SweetRenderPass _pass;SweetRenderDraw _draw;SweetRenderDepthPass _depthPass;SweetMTLTexture *_depthTarget;SweetDepthStencilState _depthState;
    SweetRenderStencilPass _stencilPass;SweetMTLTexture *_stencilTarget;
    NSArray *_memorylessBindings;
    NSArray *_resolveOperations;uint32_t _samples,_sampleMask;
    BOOL _ended,_drew;NSString *_label;
}
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands descriptor:(MTLRenderPassDescriptor *)descriptor;
- (id<MTLBuffer>)retainMemorylessPassBuffer:(uint32_t)slot info:(SweetMTLMemorylessPassInfo *)info;
- (BOOL)setDiagnosticSampleMask:(uint32_t)mask;
@end

@implementation SweetMTLTexture
- (instancetype)initMemorylessWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)d
{
    if(!(self=[super init]))return nil;
    bool depth=d && d.pixelFormat==MTLPixelFormatDepth32Float,stencil=d && d.pixelFormat==MTLPixelFormatStencil8;
    unsigned pixel=d?sweet_texture_pixel_bytes((uint32_t)d.pixelFormat):0;uint64_t bytes=0;
    unsigned samples=d && d.sampleCount<=4?(unsigned)d.sampleCount:0;
    uint64_t pitch=d?((uint64_t)d.width*pixel*samples+63)&~UINT64_C(63):0;
    if(!owner || !d || !pixel || d.width>16384 || d.height>16384 || d.storageMode!=MTLStorageModeMemoryless || d.textureType!=(samples>1?MTLTextureType2DMultisample:MTLTextureType2D) ||
        d.depth!=1 || d.arrayLength!=1 || !sweet_multisample_count(samples) || d.mipmapLevelCount!=1 || d.usage!=MTLTextureUsageRenderTarget ||
        !sweet_metal_shared_options(d.resourceOptions&~MTLResourceStorageModeMask) ||
        (samples==1 && ((depth && !sweetgl_metal_depth32_supported()) || (stencil && !sweetgl_metal_stencil8_supported()))) ||
        (samples>1 && !sweetgl_metal_multisample_supported((uint32_t)d.pixelFormat,samples)) ||
        (samples==1 && !sweet_texture_chain(d.width,d.height,(uint32_t)d.pixelFormat,0,pitch,1,(UINT64_C(256)<<20)-128,_levels,bytes)) ||
        (samples>1 && !sweet_multisample_layout((uint32_t)d.width,(uint32_t)d.height,pixel,samples,(uint32_t)pitch,0,(UINT64_C(256)<<20)-128))){
        [self release];return nil;}
    if(samples>1)_levels[0]={(uint32_t)d.width,(uint32_t)d.height,(uint32_t)pitch,pixel*samples,0,pitch*d.height};
    _samples=samples;
    _memorylessOwner=[owner retain];_layout=_levels[0];_levelCount=1;_format=d.pixelFormat;_usage=d.usage;
    _surfaceOptions=(d.resourceOptions&~(MTLResourceCPUCacheModeMask|MTLResourceHazardTrackingModeMask))|MTLResourceHazardTrackingModeTracked;return self;
}
- (SweetMTLTexture *)newMemorylessPassBackingForDevice:(SweetMTLDevice *)owner
{
    if(!_memorylessOwner || owner!=_memorylessOwner)return nil;
    // No backing is shared between encoders, even when one public transient
    // texture is reused on concurrent queues. All draws in this pass retain
    // this one private page until its actual fences retire.
    auto d=[MTLTextureDescriptor new];d.width=_layout.width;d.height=_layout.height;d.pixelFormat=_format;
    d.sampleCount=_samples;d.textureType=_samples>1?MTLTextureType2DMultisample:MTLTextureType2D;
    d.usage=MTLTextureUsageRenderTarget;d.resourceOptions=(_surfaceOptions&~MTLResourceStorageModeMask)|MTLResourceStorageModePrivate;
    auto buffer=(SweetMTLBuffer *)[owner newBufferWithLength:_layout.bytes+128 options:_surfaceOptions&~MTLResourceStorageModeMask];
    auto texture=buffer?[[SweetMTLTexture alloc] initWithBuffer:buffer descriptor:d offset:64 pitch:_layout.pitch]:nil;
    [buffer release];[d release];return texture;
}
- (instancetype)initWithBuffer:(SweetMTLBuffer *)buffer descriptor:(MTLTextureDescriptor *)d offset:(NSUInteger)offset pitch:(NSUInteger)pitch
{
    if(!(self=[super init]))return nil;
    uint64_t total=0;
    bool depth=d && d.pixelFormat==MTLPixelFormatDepth32Float;
    bool stencil=d && d.pixelFormat==MTLPixelFormatStencil8;
    unsigned samples=d && d.sampleCount<=4?(unsigned)d.sampleCount:0;bool multisample=samples==2 || samples==4;
    if(![buffer isKindOfClass:[SweetMTLBuffer class]] || !d || d.width>16384 || d.height>16384 || pitch>UINT32_MAX || d.textureType!=(multisample?MTLTextureType2DMultisample:MTLTextureType2D) || d.depth!=1 || d.arrayLength!=1 || d.mipmapLevelCount>15 ||
        !sweet_multisample_count(samples) || !sweet_metal_shared_options(d.resourceOptions&~MTLResourceStorageModeMask) ||
        (d.storageMode!=MTLStorageModeShared && d.storageMode!=MTLStorageModePrivate) ||
        (multisample && (d.storageMode!=MTLStorageModePrivate || d.mipmapLevelCount!=1 || d.usage!=MTLTextureUsageRenderTarget || !sweetgl_metal_multisample_supported((uint32_t)d.pixelFormat,samples))) ||
        (d.usage&~(MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget)) ||
        (depth && (d.mipmapLevelCount!=1 || d.usage!=MTLTextureUsageRenderTarget || !sweetgl_metal_depth32_supported())) ||
        (stencil && (d.mipmapLevelCount!=1 || d.usage!=MTLTextureUsageRenderTarget || !sweetgl_metal_stencil8_supported())) ||
        (!multisample && !sweet_texture_chain(d.width,d.height,(uint32_t)d.pixelFormat,offset,pitch,d.mipmapLevelCount,buffer.length,_levels,total)) ||
        (multisample && !sweet_multisample_layout((uint32_t)d.width,(uint32_t)d.height,sweet_texture_pixel_bytes((uint32_t)d.pixelFormat),samples,(uint32_t)pitch,offset,buffer.length))){
        [self release];return nil;}
    if(multisample)_levels[0]={(uint32_t)d.width,(uint32_t)d.height,(uint32_t)pitch,sweet_texture_pixel_bytes((uint32_t)d.pixelFormat)*samples,offset,pitch*d.height};
    _samples=samples;
    _layout=_levels[0];_levelCount=(uint32_t)d.mipmapLevelCount;_buffer=[buffer retain];_format=d.pixelFormat;_usage=d.usage;_surfaceOptions=d.resourceOptions&~MTLResourceCPUCacheModeMask;return self;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)d surface:(IOSurfaceRef)surface plane:(NSUInteger)plane
{
    if(!(self=[super init]))return nil;
    if(surface)_surface=(IOSurfaceRef)CFRetain(surface);
    unsigned pixel=d?sweet_texture_pixel_bytes((uint32_t)d.pixelFormat):0;bool valid=owner && d && d.pixelFormat!=MTLPixelFormatDepth32Float && d.pixelFormat!=MTLPixelFormatStencil8 && surface && !plane && !IOSurfaceGetPlaneCount(surface) &&
        d.textureType==MTLTextureType2D && d.depth==1 && d.arrayLength==1 && d.sampleCount==1 && d.mipmapLevelCount<=15 &&
        (d.storageMode==MTLStorageModeShared || d.storageMode==MTLStorageModePrivate) &&
        sweet_metal_shared_options(d.resourceOptions&~MTLResourceStorageModeMask) &&
        !(d.usage&~(MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget)) && pixel && d.width==IOSurfaceGetWidth(surface) && d.height==IOSurfaceGetHeight(surface) &&
        IOSurfaceGetBytesPerElement(surface)==pixel && d.width && d.height && IOSurfaceGetBaseAddress(surface) &&
        !((uintptr_t)IOSurfaceGetBaseAddress(surface)&63) && !(IOSurfaceGetBytesPerRow(surface)&63);
    if(valid){uint64_t pitch=IOSurfaceGetBytesPerRow(surface),capacity=IOSurfaceGetAllocSize(surface),total=0;
        valid=sweet_texture_chain(d.width,d.height,(uint32_t)d.pixelFormat,0,pitch,d.mipmapLevelCount,UINT64_C(256)<<20,_levels,total) && _levels[0].bytes<=capacity;
        if(valid){_layout=_levels[0];_levelCount=(uint32_t)d.mipmapLevelCount;}
        if(valid && (d.usage&MTLTextureUsageRenderTarget)){auto lock=IOSurfaceLock(surface,0,nullptr);valid=lock==KERN_SUCCESS;
            if(valid)valid=IOSurfaceUnlock(surface,0,nullptr)==KERN_SUCCESS;}
        if(valid)valid=sweetgl_metal_surface_import(IOSurfaceGetID(surface),(uintptr_t)IOSurfaceGetBaseAddress(surface),_layout.bytes,&_surfaceBacking)==0 &&
            sweet_metal_surface_backing_valid(&_surfaceBacking);
        if(valid && _levelCount>1){_mipBuffer=(SweetMTLBuffer *)[owner newBufferWithLength:total-_layout.bytes options:d.resourceOptions&~MTLResourceStorageModeMask];valid=_mipBuffer!=nil;
            if(valid)for(unsigned level=1;level<_levelCount;++level)_levels[level].offset-=_layout.bytes;}}
    if(!valid){if(_surfaceBacking.handle)(void)sweetgl_metal_buffer_release(_surfaceBacking.handle);memset(&_surfaceBacking,0,sizeof(_surfaceBacking));
        fprintf(stderr,"SWEET METAL IOSURFACE rejected id=%u plane=%lu format=%lu width=%lu height=%lu\n",surface?IOSurfaceGetID(surface):0,(unsigned long)plane,(unsigned long)(d?d.pixelFormat:0),(unsigned long)(d?d.width:0),(unsigned long)(d?d.height:0));[self release];return nil;}
    _surfaceOwner=[owner retain];_format=d.pixelFormat;_usage=d.usage;_surfaceOptions=d.resourceOptions&~MTLResourceCPUCacheModeMask;_samples=1;
    [_surfaceOwner->_resourceLock lock];_surfaceOwner->_allocatedBytes+=_surfaceBacking.allocated_bytes;[_surfaceOwner->_resourceLock unlock];
    return self;
}
- (void)dealloc{if(_surfaceBacking.handle){int status=sweetgl_metal_buffer_release(_surfaceBacking.handle);
        if(!status){[_surfaceOwner->_resourceLock lock];_surfaceOwner->_allocatedBytes-=_surfaceBacking.allocated_bytes;[_surfaceOwner->_resourceLock unlock];}
        else fprintf(stderr,"SWEET METAL IOSURFACE release failed handle=%u status=%d\n",_surfaceBacking.handle,status);}
    if(_surface)CFRelease(_surface);[_surfaceOwner release];[_memorylessOwner release];[_mipBuffer release];[_buffer release];[_label release];[super dealloc];}
- (SweetMetalBacking)sweetBacking{return _memorylessOwner?SweetMetalBacking{}:_surface?_surfaceBacking:_buffer->_backing;}
- (void *)synchronizedContents{return _memorylessOwner?nullptr:_surface?(sweetgl_metal_buffer_synchronize(_surfaceBacking.handle)?nullptr:_surfaceBacking.contents):_buffer.contents;}
- (SweetTextureLayout)layoutForLevel:(NSUInteger)level
{if(level>=_levelCount)[NSException raise:NSInvalidArgumentException format:@"Metal mipmap level outside texture"];return _levels[level];}
- (void *)synchronizedContentsForLevel:(NSUInteger)level
{(void)[self layoutForLevel:level];return _surface && level?_mipBuffer.contents:[self synchronizedContents];}
- (id<MTLDevice>)device{return _memorylessOwner?_memorylessOwner:_surface?_surfaceOwner:_buffer.device;}
- (id<MTLBuffer>)buffer{return _memorylessOwner || self.storageMode==MTLStorageModePrivate?nil:_buffer;}
- (IOSurfaceRef)iosurface{return _surface;}
- (NSUInteger)iosurfacePlane{return 0;}
- (NSUInteger)bufferOffset{return _layout.offset;}
- (NSUInteger)bufferBytesPerRow{return _memorylessOwner?0:_layout.pitch;}
- (NSUInteger)width{return _layout.width;}
- (NSUInteger)height{return _layout.height;}
- (NSUInteger)depth{return 1;}
- (NSUInteger)arrayLength{return 1;}
- (NSUInteger)mipmapLevelCount{return _levelCount;}
- (NSUInteger)sampleCount{return _samples;}
- (MTLTextureType)textureType{return _samples>1?MTLTextureType2DMultisample:MTLTextureType2D;}
- (MTLPixelFormat)pixelFormat{return _format;}
- (MTLTextureUsage)usage{return _usage;}
- (MTLStorageMode)storageMode{return (MTLStorageMode)((_surfaceOptions&MTLResourceStorageModeMask)>>MTLResourceStorageModeShift);}
- (MTLCPUCacheMode)cpuCacheMode{return MTLCPUCacheModeDefaultCache;}
- (MTLHazardTrackingMode)hazardTrackingMode{return _memorylessOwner?MTLHazardTrackingModeTracked:_surface?MTLHazardTrackingModeTracked:_buffer.hazardTrackingMode;}
- (MTLResourceOptions)resourceOptions{return _surfaceOptions;}
- (id<MTLHeap>)heap{return nil;}
- (NSUInteger)heapOffset{return 0;}
- (id<MTLTexture>)parentTexture{return nil;}
- (BOOL)isAliasable{return NO;}
- (BOOL)allowGPUOptimizedContents{return NO;}
- (NSUInteger)allocatedSize{return _memorylessOwner?0:_surface?_surfaceBacking.allocated_bytes+_mipBuffer.allocatedSize:_buffer.allocatedSize;}
- (BOOL)framebufferOnly{return _memorylessOwner!=nil;}
- (BOOL)isFramebufferOnly{return [self framebufferOnly];}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)replaceRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level withBytes:(const void *)bytes bytesPerRow:(NSUInteger)pitch
{
    uint64_t offset=0,span=0;SweetTextureLayout layout=[self layoutForLevel:level];
    if(self.storageMode!=MTLStorageModeShared || region.origin.z || region.size.depth!=1 || !sweet_texture_region(layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (span && (!bytes || span>UINTPTR_MAX-(uintptr_t)bytes)))[NSException raise:NSInvalidArgumentException format:@"Invalid Metal texture upload region/stride"];
    if(!span)return;unsigned char *target=(unsigned char *)[self synchronizedContentsForLevel:level];if(!target)[NSException raise:NSInternalInconsistencyException format:@"Texture backing synchronization failed"];
    if(_surface && !level && IOSurfaceLock(_surface,0,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface write lock failed"];
    for(NSUInteger row=0;row<region.size.height;++row)memcpy(target+offset+row*layout.pitch,(const unsigned char *)bytes+row*pitch,region.size.width*layout.bytes_per_pixel);
    if(_surface && !level && IOSurfaceUnlock(_surface,0,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface write unlock failed"];
}
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)pitch fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level
{
    uint64_t offset=0,span=0;SweetTextureLayout layout=[self layoutForLevel:level];
    if(self.storageMode!=MTLStorageModeShared || region.origin.z || region.size.depth!=1 || !sweet_texture_region(layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (span && (!bytes || span>UINTPTR_MAX-(uintptr_t)bytes)))[NSException raise:NSInvalidArgumentException format:@"Invalid Metal texture read region/stride"];
    if(!span)return;const unsigned char *source=(const unsigned char *)[self synchronizedContentsForLevel:level];if(!source)[NSException raise:NSInternalInconsistencyException format:@"Texture backing synchronization failed"];
    if(_surface && !level && IOSurfaceLock(_surface,kIOSurfaceLockReadOnly,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface read lock failed"];
    for(NSUInteger row=0;row<region.size.height;++row)memcpy((unsigned char *)bytes+row*pitch,source+offset+row*layout.pitch,region.size.width*layout.bytes_per_pixel);
    if(_surface && !level && IOSurfaceUnlock(_surface,kIOSurfaceLockReadOnly,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface read unlock failed"];
}
- (void)replaceRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice withBytes:(const void *)bytes bytesPerRow:(NSUInteger)pitch bytesPerImage:(NSUInteger)imagePitch
{
    uint64_t offset=0,span=0;if(slice || !sweet_texture_region([self layoutForLevel:level],region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (imagePitch && imagePitch<span))[NSException raise:NSInvalidArgumentException format:@"Invalid 2D texture slice/image stride"];
    [self replaceRegion:region mipmapLevel:level withBytes:bytes bytesPerRow:pitch];
}
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)pitch bytesPerImage:(NSUInteger)imagePitch fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice
{
    uint64_t offset=0,span=0;if(slice || !sweet_texture_region([self layoutForLevel:level],region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (imagePitch && imagePitch<span))[NSException raise:NSInvalidArgumentException format:@"Invalid 2D texture slice/image stride"];
    [self getBytes:bytes bytesPerRow:pitch fromRegion:region mipmapLevel:level];
}
- (void)doesNotRecognizeSelector:(SEL)selector
{fprintf(stderr,"SWEET METAL missing class=texture selector=%s original_exception=1\n",sel_getName(selector));[super doesNotRecognizeSelector:selector];}
- (SweetShaderTextureBinding)bindingForIndex:(unsigned)index stage:(unsigned)stage
{if(_memorylessOwner)return {};SweetShaderTextureBinding binding={[self sweetBacking].handle,index,stage,_layout.width,_layout.height,_layout.pitch,(uint32_t)_format,(uint32_t)_usage,_layout.offset};
    binding.mip_count=_levelCount;for(unsigned level=1;level<_levelCount;++level){auto layout=_levels[level];binding.mips[level-1]={_surface?_mipBuffer->_backing.handle:_buffer->_backing.handle,layout.width,layout.height,layout.pitch,layout.offset};}return binding;}
@end

#include "MetalMemoryless.inc"
#include "MetalMultisample.inc"

@implementation SweetMTLSampler
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLSamplerDescriptor *)d
{
    if(!(self=[super init]))return nil;
    if(!owner || !d || d.minFilter>MTLSamplerMinMagFilterLinear || d.magFilter>MTLSamplerMinMagFilterLinear || d.mipFilter>MTLSamplerMipFilterLinear ||
        (d.sAddressMode!=MTLSamplerAddressModeClampToEdge && d.sAddressMode!=MTLSamplerAddressModeMirrorClampToEdge && d.sAddressMode!=MTLSamplerAddressModeRepeat && d.sAddressMode!=MTLSamplerAddressModeMirrorRepeat) ||
        (d.tAddressMode!=MTLSamplerAddressModeClampToEdge && d.tAddressMode!=MTLSamplerAddressModeMirrorClampToEdge && d.tAddressMode!=MTLSamplerAddressModeRepeat && d.tAddressMode!=MTLSamplerAddressModeMirrorRepeat) ||
        !d.normalizedCoordinates || d.compareFunction!=MTLCompareFunctionNever || d.maxAnisotropy!=1 || !std::isfinite(d.lodMinClamp) ||
        !std::isfinite(d.lodMaxClamp) || d.lodMinClamp>d.lodMaxClamp){[self release];return nil;}
    _owner=[owner retain];_binding={0,0,(uint32_t)d.minFilter,(uint32_t)d.magFilter,(uint32_t)d.mipFilter,(uint32_t)d.sAddressMode,(uint32_t)d.tAddressMode,1,d.lodMinClamp,d.lodMaxClamp};
    _label=[d.label copy];return self;
}
- (void)dealloc{[_owner release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _owner;}
- (NSString *)label{return _label;}
@end

static bool render_entry(SweetMTLFunction *function,unsigned stage,GPU::Compiler::Entry &entry,std::string &error)
{
    if(![function isKindOfClass:[SweetMTLFunction class]] || function->_stage!=stage){error="Wrong AIR graphics stage";return false;}
    // This native function owns an immutable library slice and constant-value
    // snapshot. Cache only successful translations, with a per-function lock;
    // different specializations never share an entry or a transient failure.
    std::lock_guard<std::mutex> lock(function->_graphicsMutex);
    if(function->_graphicsEntry){entry=*function->_graphicsEntry;
        fprintf(stderr,"SWEET AIR GRAPHICS cache=hit stage=%u name=%s bytes=%lu\n",stage,function->_name.UTF8String,(unsigned long)entry.glsl.size());return true;}
    NSData *source=function->_library->_entries[function->_name];std::vector<GPU::Compiler::Entry> entries;
    if(!source || !GPU::Compiler::lowerAIR(source.bytes,source.length,entries,error,function->_name.UTF8String,function->_constants) || entries.size()!=1)return false;
    function->_graphicsEntry=std::make_unique<GPU::Compiler::Entry>(std::move(entries[0]));entry=*function->_graphicsEntry;
    fprintf(stderr,"SWEET AIR GRAPHICS cache=miss stage=%u name=%s bytes=%lu\n",stage,function->_name.UTF8String,(unsigned long)entry.glsl.size());return true;
}
@implementation SweetMTLRenderPipeline
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLRenderPipelineDescriptor *)d error:(NSError **)error
{
    if(!(self=[super init]))return nil;std::string diagnostic;
    if(!owner || !d){if(error)*error=metal_compile_error("Render descriptor and device required");[self release];return nil;}
    auto vertex=(SweetMTLFunction *)d.vertexFunction,fragment=(SweetMTLFunction *)d.fragmentFunction;
    auto color=d.colorAttachments[0];bool valid=owner && d && [vertex isKindOfClass:[SweetMTLFunction class]] && [fragment isKindOfClass:[SweetMTLFunction class]] && vertex.device==owner && fragment.device==owner && d.rasterSampleCount<=4 && sweet_multisample_count((uint32_t)d.rasterSampleCount) &&
        (d.depthAttachmentPixelFormat==MTLPixelFormatInvalid || (d.depthAttachmentPixelFormat==MTLPixelFormatDepth32Float && sweetgl_metal_depth32_supported())) &&
        (d.stencilAttachmentPixelFormat==MTLPixelFormatInvalid || (d.stencilAttachmentPixelFormat==MTLPixelFormatStencil8 && sweetgl_metal_stencil8_supported())) &&
        !d.alphaToCoverageEnabled && !d.alphaToOneEnabled && d.rasterizationEnabled && !d.vertexDescriptor.layouts[0].stride &&
        color.pixelFormat!=MTLPixelFormatDepth32Float && color.pixelFormat!=MTLPixelFormatStencil8 && sweet_texture_pixel_bytes((uint32_t)color.pixelFormat);
    SweetRenderPipelineConfig config={};
    if(valid)for(unsigned i=0;i<8;++i){auto c=d.colorAttachments[i];if(c.pixelFormat==MTLPixelFormatInvalid)continue;
        if(c.pixelFormat==MTLPixelFormatDepth32Float || c.pixelFormat==MTLPixelFormatStencil8 || !sweet_texture_pixel_bytes((uint32_t)c.pixelFormat)){valid=false;break;}config.color_mask|=1u<<i;
        if(d.rasterSampleCount>1 && !sweetgl_metal_multisample_supported((uint32_t)c.pixelFormat,(uint32_t)d.rasterSampleCount)){valid=false;break;}
        SweetRenderColorConfig state={(uint32_t)c.pixelFormat,(uint32_t)c.blendingEnabled,(uint32_t)c.sourceRGBBlendFactor,(uint32_t)c.destinationRGBBlendFactor,(uint32_t)c.rgbBlendOperation,
            (uint32_t)c.sourceAlphaBlendFactor,(uint32_t)c.destinationAlphaBlendFactor,(uint32_t)c.alphaBlendOperation,(uint32_t)c.writeMask};
        if(state.source_rgb>14 || state.destination_rgb>14 || state.source_alpha>14 || state.destination_alpha>14 || state.operation_rgb>4 || state.operation_alpha>4 || state.write_mask>15){valid=false;diagnostic="Unsupported color attachment blend/mask";break;}
        if(i)config.colors[i-1]=state;
    }
    if(valid && d.rasterSampleCount>1)valid=
        (d.depthAttachmentPixelFormat==MTLPixelFormatInvalid || sweetgl_metal_multisample_supported((uint32_t)d.depthAttachmentPixelFormat,(uint32_t)d.rasterSampleCount)) &&
        (d.stencilAttachmentPixelFormat==MTLPixelFormatInvalid || sweetgl_metal_multisample_supported((uint32_t)d.stencilAttachmentPixelFormat,(uint32_t)d.rasterSampleCount));
    GPU::Compiler::Entry vs,fs;std::string vertex_source,fragment_source;
    if(valid)valid=render_entry(vertex,1,vs,diagnostic) && render_entry(fragment,2,fs,diagnostic);
    if(valid)valid=!(vs.reflection.attributeMask || fs.reflection.framebufferMask || vs.reflection.writeMask) &&
        SweetAIR::graphicsESSL(vs.glsl,1,vs.reflection,vertex_source,diagnostic) && SweetAIR::graphicsESSL(fs.glsl,2,fs.reflection,fragment_source,diagnostic);
    if(valid)for(unsigned i=0;i<fs.reflection.varyingCount;++i){bool found=false;auto &input=fs.reflection.varyings[i];
        for(unsigned j=0;j<vs.reflection.varyingCount;++j){auto &output=vs.reflection.varyings[j];if(!memcmp(input.semantic,output.semantic,sizeof(input.semantic)) && input.components==output.components && input.mode==output.mode){found=true;break;}}
        if(!found){valid=false;diagnostic="AIR vertex/fragment varyings disagree";break;}}
    config.format=(uint32_t)color.pixelFormat;config.vertex_buffers=vs.reflection.bufferMask;
    config.fragment_buffers=fs.reflection.bufferMask;config.fragment_writes=fs.reflection.writeMask;config.vertex_texture_slots=vs.reflection.textureBindings;config.fragment_texture_slots=fs.reflection.textureBindings;
    config.blend_enabled=color.blendingEnabled;config.source_rgb=(uint32_t)color.sourceRGBBlendFactor;config.destination_rgb=(uint32_t)color.destinationRGBBlendFactor;config.operation_rgb=(uint32_t)color.rgbBlendOperation;
    config.source_alpha=(uint32_t)color.sourceAlphaBlendFactor;config.destination_alpha=(uint32_t)color.destinationAlphaBlendFactor;config.operation_alpha=(uint32_t)color.alphaBlendOperation;config.write_mask=(uint32_t)color.writeMask;
    if(config.source_rgb>14 || config.destination_rgb>14 || config.source_alpha>14 || config.destination_alpha>14 || config.operation_rgb>4 || config.operation_alpha>4 || config.write_mask>15)valid=false;
    if(valid){uint32_t outputs=0;for(unsigned i=0;i<8;++i){std::string name=i?"_qcolor"+std::to_string(i):"_qcolor";
            if(fragment_source.find("layout(location="+std::to_string(i)+") out vec4 "+name+";")!=std::string::npos)outputs|=1u<<i;}
        if(!outputs || (outputs&~config.color_mask)){valid=false;diagnostic="Actual AIR color outputs require matching attachment formats";}}
    char message[4096]={0};if(valid)_program=d.rasterSampleCount>1?
        sweetgl_metal_render_create_multisample(vertex_source.c_str(),fragment_source.c_str(),&config,(uint32_t)d.rasterSampleCount,message,sizeof(message)):
        sweetgl_metal_render_create(vertex_source.c_str(),fragment_source.c_str(),&config,message,sizeof(message));
    if(!_program){if(diagnostic.empty())diagnostic=message;if(diagnostic.empty())diagnostic="Render descriptor or actual AIR interface is unsupported";
        if(error)*error=metal_compile_error(diagnostic);[self release];return nil;}
    _owner=[owner retain];_vertex=[vertex retain];_fragment=[fragment retain];_descriptor=[d copy];_depthFormat=d.depthAttachmentPixelFormat;_stencilFormat=d.stencilAttachmentPixelFormat;_samples=(uint32_t)d.rasterSampleCount;return self;
}
- (void)dealloc{sweetgl_metal_program_destroy(_program);[_owner release];[_vertex release];[_fragment release];[_descriptor release];[super dealloc];}
- (id<MTLDevice>)device{return _owner;}
- (NSString *)label{return _descriptor.label;}
- (void)doesNotRecognizeSelector:(SEL)selector
{fprintf(stderr,"SWEET METAL missing class=render_pipeline selector=%s original_exception=1\n",sel_getName(selector));[super doesNotRecognizeSelector:selector];}
@end

@implementation SweetMTLRenderEncoder
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands descriptor:(MTLRenderPassDescriptor *)d
{
    if(!(self=[super init]))return nil;
    auto resolved=memoryless_pass_descriptor(commands,d,&_memorylessBindings);
    if(!resolved || !multisample_pass_resolves(commands,resolved,&_resolveOperations)){[resolved release];[self release];return nil;}d=resolved;auto target=(SweetMTLTexture *)d.colorAttachments[0].texture;
    bool valid=d && [target isKindOfClass:[SweetMTLTexture class]] && target.device==commands.device && (!target.usage || (target.usage&MTLTextureUsageRenderTarget)) &&
        !d.colorAttachments[0].resolveTexture && d.colorAttachments[0].level==0 &&
        d.colorAttachments[0].slice==0 && d.colorAttachments[0].depthPlane==0 && d.colorAttachments[0].loadAction<=MTLLoadActionClear &&
        d.colorAttachments[0].storeAction<=MTLStoreActionStore;
    _samples=valid?(uint32_t)target.sampleCount:1;_sampleMask=UINT32_MAX;valid=valid && sweet_multisample_count(_samples);
    NSMutableArray *targets=[NSMutableArray new];
    if(valid)for(unsigned i=0;i<8;++i){auto a=d.colorAttachments[i];auto texture=(SweetMTLTexture *)a.texture;
        [targets addObject:texture?texture:(id)[NSNull null]];if(!texture)continue;
        valid=[texture isKindOfClass:[SweetMTLTexture class]] && texture.pixelFormat!=MTLPixelFormatDepth32Float && texture.pixelFormat!=MTLPixelFormatStencil8 && texture.device==commands.device && (!texture.usage || (texture.usage&MTLTextureUsageRenderTarget)) &&
            texture.width==target.width && texture.height==target.height && texture.sampleCount==_samples && !a.resolveTexture && !a.level && !a.slice && !a.depthPlane && a.loadAction<=MTLLoadActionClear && a.storeAction<=MTLStoreActionStore;
        if(!valid)break;
        for(unsigned j=0;j<i && valid;++j){id previous=targets[j];if(previous==[NSNull null])continue;auto other=(SweetMTLTexture *)previous;
            if(texture==other || (texture->_surface && texture->_surface==other->_surface))valid=false;
            else if([texture sweetBacking].handle==[other sweetBacking].handle){uint64_t a0=texture->_layout.offset,a1=a0+texture->_layout.bytes,b0=other->_layout.offset,b1=b0+other->_layout.bytes;
                if(a0<b1 && b0<a1)valid=false;}}
        if(!valid)break;_pass.color_mask|=1u<<i;
        if(i){auto binding=[texture bindingForIndex:i stage:0];_pass.colors[i-1]={binding,(uint32_t)a.loadAction,(uint32_t)a.storeAction,{a.clearColor.red,a.clearColor.green,a.clearColor.blue,a.clearColor.alpha}};}
    }
    auto depth=(SweetMTLTexture *)d.depthAttachment.texture;
    if(valid && depth){auto a=d.depthAttachment;
        valid=[depth isKindOfClass:[SweetMTLTexture class]] && depth.device==commands.device && depth.pixelFormat==MTLPixelFormatDepth32Float &&
            depth.usage==MTLTextureUsageRenderTarget && depth.width==target.width && depth.height==target.height && depth.sampleCount==_samples && !depth->_surface && depth.mipmapLevelCount==1 &&
            !a.resolveTexture && !a.level && !a.slice && !a.depthPlane && a.loadAction<=MTLLoadActionClear && a.storeAction<=MTLStoreActionStore &&
            std::isfinite(a.clearDepth) && a.clearDepth>=0 && a.clearDepth<=1;
        for(id item in targets)if(valid && item!=[NSNull null]){auto color=(SweetMTLTexture *)item;
            if([color sweetBacking].handle==[depth sweetBacking].handle){uint64_t begin=color->_layout.offset,end=begin+color->_layout.bytes;
                if(begin<depth->_layout.offset+depth->_layout.bytes && depth->_layout.offset<end)valid=false;}}
        if(valid){_depthPass.target=[depth bindingForIndex:0 stage:0];_depthPass.load_action=(uint32_t)a.loadAction;
            _depthPass.store_action=(uint32_t)a.storeAction;_depthPass.clear_depth=a.clearDepth;_depthTarget=[depth retain];}
    }
    auto stencil=(SweetMTLTexture *)d.stencilAttachment.texture;
    if(valid && stencil){auto a=d.stencilAttachment;
        valid=[stencil isKindOfClass:[SweetMTLTexture class]] && stencil.device==commands.device && stencil.pixelFormat==MTLPixelFormatStencil8 &&
            stencil.usage==MTLTextureUsageRenderTarget && stencil.width==target.width && stencil.height==target.height && stencil.sampleCount==_samples && !stencil->_surface && stencil.mipmapLevelCount==1 &&
            !a.resolveTexture && !a.level && !a.slice && !a.depthPlane && a.loadAction<=MTLLoadActionClear && a.storeAction<=MTLStoreActionStore;
        for(id item in targets)if(valid && item!=[NSNull null]){auto color=(SweetMTLTexture *)item;
            if([color sweetBacking].handle==[stencil sweetBacking].handle){uint64_t begin=color->_layout.offset,end=begin+color->_layout.bytes;
                if(begin<stencil->_layout.offset+stencil->_layout.bytes && stencil->_layout.offset<end)valid=false;}}
        if(valid && depth && [depth sweetBacking].handle==[stencil sweetBacking].handle &&
            depth->_layout.offset<stencil->_layout.offset+stencil->_layout.bytes && stencil->_layout.offset<depth->_layout.offset+depth->_layout.bytes)valid=false;
        if(valid){_stencilPass.target=[stencil bindingForIndex:0 stage:0];_stencilPass.load_action=(uint32_t)a.loadAction;
            _stencilPass.store_action=(uint32_t)a.storeAction;_stencilPass.clear_stencil=a.clearStencil&255u;_stencilTarget=[stencil retain];}
    }
    if(!valid){[targets release];[resolved release];[self release];return nil;}_targets=[targets copy];[targets release];_commands=[commands retain];_target=[target retain];
    auto attachment=d.colorAttachments[0];_pass.target=[target bindingForIndex:0 stage:0];_pass.load_action=(uint32_t)attachment.loadAction;_pass.store_action=(uint32_t)attachment.storeAction;
    _pass.clear[0]=attachment.clearColor.red;_pass.clear[1]=attachment.clearColor.green;_pass.clear[2]=attachment.clearColor.blue;_pass.clear[3]=attachment.clearColor.alpha;
    _draw={(uint32_t)MTLPrimitiveTypeTriangle,0,0,1,(uint32_t)MTLCullModeNone,(uint32_t)MTLWindingClockwise,0,0,(double)target.width,(double)target.height,0,1,0,0,(uint32_t)target.width,(uint32_t)target.height};
    _depthState=sweet_depth_stencil_default();
    _vertexBuffers=[NSMutableDictionary new];_fragmentBuffers=[NSMutableDictionary new];_vertexTextures=[NSMutableDictionary new];_fragmentTextures=[NSMutableDictionary new];_vertexSamplers=[NSMutableDictionary new];_fragmentSamplers=[NSMutableDictionary new];[resolved release];return self;
}
- (void)dealloc{[_commands release];[_target release];[_targets release];[_depthTarget release];[_stencilTarget release];[_memorylessBindings release];[_resolveOperations release];[_pipeline release];[_vertexBuffers release];[_fragmentBuffers release];[_vertexTextures release];[_fragmentTextures release];[_vertexSamplers release];[_fragmentSamplers release];[_label release];[super dealloc];}
- (id<MTLBuffer>)retainMemorylessPassBuffer:(uint32_t)slot info:(SweetMTLMemorylessPassInfo *)info
{
    if(_ended || !info || slot>9)return nil;
    for(NSDictionary *entry in _memorylessBindings)if([entry[@"slot"] unsignedIntValue]==slot){
        auto texture=(SweetMTLTexture *)entry[@"private"];auto publicTexture=(SweetMTLTexture *)entry[@"public"];
        if(!texture->_buffer || texture.device!=self.device || publicTexture.device!=self.device)return nil;
        auto backing=texture->_buffer->_backing;
        *info={ [entry[@"pass"] unsignedLongLongValue],(uint64_t)(uintptr_t)publicTexture,backing.gpu_address,backing.logical_bytes,
            slot,backing.handle,(uint32_t)texture.width,(uint32_t)texture.height,texture->_layout.pitch,(uint32_t)texture->_layout.offset,(uint32_t)texture.pixelFormat,0};
        return [texture->_buffer retain];
    }return nil;
}
- (id<MTLDevice>)device{return _commands.device;}
- (BOOL)setDiagnosticSampleMask:(uint32_t)mask
{if(_ended || _samples==1 || (mask&~((1u<<_samples)-1u)))return NO;_sampleMask=mask;return YES;}
- (void)doesNotRecognizeSelector:(SEL)selector
{fprintf(stderr,"SWEET METAL missing class=render_encoder selector=%s original_exception=1\n",sel_getName(selector));[super doesNotRecognizeSelector:selector];}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)setRenderPipelineState:(id<MTLRenderPipelineState>)pipeline
{if(_ended || ![pipeline isKindOfClass:[SweetMTLRenderPipeline class]] || pipeline.device!=self.device)[NSException raise:NSInvalidArgumentException format:@"Invalid Metal render pipeline owner"];
    [pipeline retain];[_pipeline release];_pipeline=(SweetMTLRenderPipeline *)pipeline;}
- (void)setDepthStencilState:(id<MTLDepthStencilState>)state
{
    if(_ended || (state && (![state isKindOfClass:[SweetMTLDepthStencilState class]] || state.device!=self.device)))
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal depth/stencil state owner"];
    uint32_t front=_depthState.front_reference,back=_depthState.back_reference;
    _depthState=state?((SweetMTLDepthStencilState *)state)->_state:sweet_depth_stencil_default();
    _depthState.front_reference=front;_depthState.back_reference=back;
}
- (void)setStencilReferenceValue:(uint32_t)value{[self setStencilFrontReferenceValue:value backReferenceValue:value];}
- (void)setStencilFrontReferenceValue:(uint32_t)front backReferenceValue:(uint32_t)back
{if(_ended)[NSException raise:NSInvalidArgumentException format:@"Metal render encoder ended"];
    _depthState.front_reference=front&255u;_depthState.back_reference=back&255u;}
- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha
{if(_ended || !std::isfinite(red) || !std::isfinite(green) || !std::isfinite(blue) || !std::isfinite(alpha))[NSException raise:NSInvalidArgumentException format:@"Invalid blend constant"];
    _draw.blend_color[0]=red;_draw.blend_color[1]=green;_draw.blend_color[2]=blue;_draw.blend_color[3]=alpha;}
- (void)setBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset index:(NSUInteger)index dictionary:(NSMutableDictionary *)dictionary
{
    if(_ended || index>=8 || (buffer && (![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=self.device || offset>=buffer.length || (offset&3))))
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal render buffer range/owner"];
    @autoreleasepool{if(buffer)dictionary[@(index)]=@{@"buffer":buffer,@"offset":@(offset)};else [dictionary removeObjectForKey:@(index)];}
}
- (void)setVertexBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index{[self setBuffer:buffer offset:offset index:index dictionary:_vertexBuffers];}
- (void)setFragmentBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index{[self setBuffer:buffer offset:offset index:index dictionary:_fragmentBuffers];}
- (void)setTexture:(id<MTLTexture>)texture index:(NSUInteger)index dictionary:(NSMutableDictionary *)dictionary
{if(_ended || index>=16 || (texture && (![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=self.device || texture.storageMode==MTLStorageModeMemoryless || (texture.usage && !(texture.usage&MTLTextureUsageShaderRead)))))
    [NSException raise:NSInvalidArgumentException format:@"Invalid Metal render texture owner/usage"];
    if(texture)dictionary[@(index)]=texture;else [dictionary removeObjectForKey:@(index)];}
- (void)setVertexTexture:(id<MTLTexture>)texture atIndex:(NSUInteger)index{[self setTexture:texture index:index dictionary:_vertexTextures];}
- (void)setFragmentTexture:(id<MTLTexture>)texture atIndex:(NSUInteger)index{[self setTexture:texture index:index dictionary:_fragmentTextures];}
- (void)setSampler:(id<MTLSamplerState>)sampler index:(NSUInteger)index dictionary:(NSMutableDictionary *)dictionary
{if(_ended || index>=8 || (sampler && (![sampler isKindOfClass:[SweetMTLSampler class]] || sampler.device!=self.device)))
    [NSException raise:NSInvalidArgumentException format:@"Invalid Metal sampler owner/index"];
    if(sampler)dictionary[@(index)]=sampler;else [dictionary removeObjectForKey:@(index)];}
- (void)setVertexSamplerState:(id<MTLSamplerState>)sampler atIndex:(NSUInteger)index{[self setSampler:sampler index:index dictionary:_vertexSamplers];}
- (void)setFragmentSamplerState:(id<MTLSamplerState>)sampler atIndex:(NSUInteger)index{[self setSampler:sampler index:index dictionary:_fragmentSamplers];}
- (void)setViewport:(MTLViewport)v
{if(_ended || !std::isfinite(v.originX) || !std::isfinite(v.originY) || !std::isfinite(v.width) || !std::isfinite(v.height) ||
    !std::isfinite(v.znear) || !std::isfinite(v.zfar) || v.znear<0 || v.znear>1 || v.zfar<0 || v.zfar>1 ||
    v.originX<0 || v.originY<0 || v.originX>_target.width || v.width>_target.width-v.originX ||
    v.originY>_target.height || v.height>_target.height-v.originY || v.width<=0 || v.height<=0 ||
    v.originX!=std::floor(v.originX) || v.originY!=std::floor(v.originY) || v.width!=std::floor(v.width) || v.height!=std::floor(v.height))
    [NSException raise:NSInvalidArgumentException format:@"Invalid Metal viewport"];
    _draw.x=v.originX;_draw.y=v.originY;_draw.width=v.width;_draw.height=v.height;_draw.znear=v.znear;_draw.zfar=v.zfar;}
- (void)setScissorRect:(MTLScissorRect)r
{if(_ended || r.x>_target.width || r.width>_target.width-r.x || r.y>_target.height || r.height>_target.height-r.y)[NSException raise:NSInvalidArgumentException format:@"Metal scissor outside target"];
    _draw.scissor_x=(uint32_t)r.x;_draw.scissor_y=(uint32_t)r.y;_draw.scissor_width=(uint32_t)r.width;_draw.scissor_height=(uint32_t)r.height;}
- (void)setCullMode:(MTLCullMode)cull{if(_ended || cull>MTLCullModeBack)[NSException raise:NSInvalidArgumentException format:@"Invalid cull state"];_draw.cull=(uint32_t)cull;}
- (void)setFrontFacingWinding:(MTLWinding)winding{if(_ended || winding>MTLWindingCounterClockwise)[NSException raise:NSInvalidArgumentException format:@"Invalid winding state"];_draw.winding=(uint32_t)winding;}
- (void)drawPrimitives:(MTLPrimitiveType)primitive vertexStart:(NSUInteger)start vertexCount:(NSUInteger)count instanceCount:(NSUInteger)instances
{
    if(_ended || !_pipeline || primitive>MTLPrimitiveTypeTriangleStrip || !count || start>1048576 || count>1048576-start || !instances || instances>65535 || count>16777216/instances)
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal draw range/state"];
    if(_pipeline->_depthFormat!=(_depthTarget?_depthTarget.pixelFormat:MTLPixelFormatInvalid))
        [NSException raise:NSInvalidArgumentException format:@"Metal pipeline depth format differs from attachment"];
    if(_pipeline->_stencilFormat!=(_stencilTarget?_stencilTarget.pixelFormat:MTLPixelFormatInvalid))
        [NSException raise:NSInvalidArgumentException format:@"Metal pipeline stencil format differs from attachment"];
    if(_pipeline->_samples!=_samples)[NSException raise:NSInvalidArgumentException format:@"Metal pipeline samples differ from attachments"];
    if(!sweet_depth_stencil_valid(&_depthState))[NSException raise:NSInvalidArgumentException format:@"Invalid depth/stencil state snapshot"];
    _depthPass.state=_depthState;
    _stencilPass.state=_depthState;
    _draw.primitive=(uint32_t)primitive;_draw.first=(uint32_t)start;_draw.count=(uint32_t)count;_draw.instances=(uint32_t)instances;
    @autoreleasepool{NSData *pass=[NSData dataWithBytes:&_pass length:sizeof(_pass)],*draw=[NSData dataWithBytes:&_draw length:sizeof(_draw)];
        if(![_commands appendOperation:@{@"renderPipeline":_pipeline,@"target":_target,@"targets":_targets,@"pass":pass,@"draw":draw,@"memoryless":_memorylessBindings,@"samples":@(_samples),@"sampleMask":@(_sampleMask),
            @"depthTarget":_depthTarget?(id)_depthTarget:[NSNull null],@"depthPass":[NSData dataWithBytes:&_depthPass length:sizeof(_depthPass)],
            @"stencilTarget":_stencilTarget?(id)_stencilTarget:[NSNull null],@"stencilPass":[NSData dataWithBytes:&_stencilPass length:sizeof(_stencilPass)],
            @"vertexBuffers":[NSDictionary dictionaryWithDictionary:_vertexBuffers],@"fragmentBuffers":[NSDictionary dictionaryWithDictionary:_fragmentBuffers],
            @"vertexTextures":[NSDictionary dictionaryWithDictionary:_vertexTextures],@"fragmentTextures":[NSDictionary dictionaryWithDictionary:_fragmentTextures],
            @"vertexSamplers":[NSDictionary dictionaryWithDictionary:_vertexSamplers],@"fragmentSamplers":[NSDictionary dictionaryWithDictionary:_fragmentSamplers]}])
            [NSException raise:NSInternalInconsistencyException format:@"Render command buffer no longer writable"];}
    _drew=YES;_pass.load_action=MTLLoadActionLoad;for(unsigned i=1;i<8;++i)_pass.colors[i-1].load_action=MTLLoadActionLoad;
    if(_depthTarget)_depthPass.load_action=MTLLoadActionLoad;
    if(_stencilTarget)_stencilPass.load_action=MTLLoadActionLoad;
}
- (void)drawPrimitives:(MTLPrimitiveType)p vertexStart:(NSUInteger)start vertexCount:(NSUInteger)count{[self drawPrimitives:p vertexStart:start vertexCount:count instanceCount:1];}
- (void)endEncoding
{
    if(_ended)[NSException raise:NSInternalInconsistencyException format:@"Metal render encoder already ended"];
    bool clear=false;for(unsigned i=0;i<8;++i)if(_pass.color_mask&(1u<<i))clear|=sweet_render_attachment(&_pass,i).load_action==MTLLoadActionClear;
    clear|=_depthTarget && _depthPass.load_action==MTLLoadActionClear;
    clear|=_stencilTarget && _stencilPass.load_action==MTLLoadActionClear;
    if(!_drew && clear){@autoreleasepool{
        if(![_commands appendOperation:@{@"renderClear":@YES,@"target":_target,@"targets":_targets,@"pass":[NSData dataWithBytes:&_pass length:sizeof(_pass)],@"memoryless":_memorylessBindings,@"samples":@(_samples),
            @"depthTarget":_depthTarget?(id)_depthTarget:[NSNull null],@"depthPass":[NSData dataWithBytes:&_depthPass length:sizeof(_depthPass)],
            @"stencilTarget":_stencilTarget?(id)_stencilTarget:[NSNull null],@"stencilPass":[NSData dataWithBytes:&_stencilPass length:sizeof(_stencilPass)]}])
            [NSException raise:NSInternalInconsistencyException format:@"Render clear command buffer not writable"];}}
    for(NSDictionary *resolve in _resolveOperations)if(![_commands appendOperation:resolve])
        [NSException raise:NSInternalInconsistencyException format:@"Metal resolve command buffer not writable"];
    _ended=YES;[_vertexBuffers removeAllObjects];[_fragmentBuffers removeAllObjects];[_vertexTextures removeAllObjects];[_fragmentTextures removeAllObjects];[_vertexSamplers removeAllObjects];[_fragmentSamplers removeAllObjects];[_pipeline release];_pipeline=nil;[_target release];_target=nil;[_targets release];_targets=nil;[_depthTarget release];_depthTarget=nil;[_stencilTarget release];_stencilTarget=nil;[_memorylessBindings release];_memorylessBindings=nil;[_resolveOperations release];_resolveOperations=nil;[_commands finishEncoder];
}
@end
static id<MTLRenderCommandEncoder> create_render_encoder(SweetMTLCommandBuffer *commands,MTLRenderPassDescriptor *descriptor)
{return [[[SweetMTLRenderEncoder alloc] initWithCommands:commands descriptor:descriptor] autorelease];}
static NSError *execute_metal_render(NSDictionary *operation,id<MTLDevice> device,uint32_t *sequence)
{
    if(operation[@"multisampleResolve"])return execute_multisample_resolve(operation,device,sequence);
    if(!memoryless_operation_valid(operation,device))return metal_queue_error(@"Transient attachment snapshot or owner differs");
    auto target=(SweetMTLTexture *)operation[@"target"];NSData *passData=operation[@"pass"],*drawData=operation[@"draw"];
    if(![target isKindOfClass:[SweetMTLTexture class]] || target.device!=device || passData.length!=sizeof(SweetRenderPass))return metal_queue_error(@"Render target/pass owner or ABI invalid");
    SweetRenderPass pass;memcpy(&pass,passData.bytes,sizeof(pass));
    unsigned samples=[operation[@"samples"] unsignedIntValue];if(!sweet_multisample_count(samples) || target.sampleCount!=samples)return metal_queue_error(@"Render sample snapshot differs from original texture");
    NSArray *targets=operation[@"targets"];if(targets.count!=8 || pass.reserved || !pass.color_mask || (pass.color_mask&~255u))return metal_queue_error(@"Render attachment snapshot count/mask invalid");
    for(unsigned i=0;i<8;++i){id object=targets[i];bool absent=object==[NSNull null];if(absent!=( !(pass.color_mask&(1u<<i))))return metal_queue_error(@"Render attachment mask differs from retained targets");if(absent)continue;
        auto texture=(SweetMTLTexture *)object;if(![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device || texture.sampleCount!=samples)return metal_queue_error(@"Render attachment owner or samples invalid");
        auto actual=[texture bindingForIndex:i stage:0];auto saved=sweet_render_attachment(&pass,i);if(memcmp(&actual,&saved.target,sizeof(actual)))return metal_queue_error(@"Render target snapshot differs from retained backing");}
    id retainedDepth=operation[@"depthTarget"];SweetMTLTexture *depth=retainedDepth==[NSNull null]?nil:(SweetMTLTexture *)retainedDepth;
    NSData *depthData=operation[@"depthPass"];SweetRenderDepthPass depthPass={};
    if(!retainedDepth || depthData.length!=sizeof(depthPass))return metal_queue_error(@"Render depth snapshot missing or ABI invalid");
    memcpy(&depthPass,depthData.bytes,sizeof(depthPass));
    if(depth){if(![depth isKindOfClass:[SweetMTLTexture class]] || depth.device!=device || depth.pixelFormat!=MTLPixelFormatDepth32Float || depth.sampleCount!=samples)
            return metal_queue_error(@"Render depth attachment owner or format invalid");
        auto actual=[depth bindingForIndex:0 stage:0];if(memcmp(&actual,&depthPass.target,sizeof(actual)))return metal_queue_error(@"Render depth snapshot differs from original retained backing");}
    else if(depthPass.target.handle)return metal_queue_error(@"Unexpected depth backing without retained attachment");
    id retainedStencil=operation[@"stencilTarget"];SweetMTLTexture *stencil=retainedStencil==[NSNull null]?nil:(SweetMTLTexture *)retainedStencil;
    NSData *stencilData=operation[@"stencilPass"];SweetRenderStencilPass stencilPass={};
    if(!retainedStencil || stencilData.length!=sizeof(stencilPass))return metal_queue_error(@"Render stencil snapshot missing or ABI invalid");
    memcpy(&stencilPass,stencilData.bytes,sizeof(stencilPass));
    if(stencil){if(![stencil isKindOfClass:[SweetMTLTexture class]] || stencil.device!=device || stencil.pixelFormat!=MTLPixelFormatStencil8 || stencil.sampleCount!=samples)
            return metal_queue_error(@"Render stencil attachment owner or format invalid");
        auto actual=[stencil bindingForIndex:0 stage:0];if(memcmp(&actual,&stencilPass.target,sizeof(actual)))return metal_queue_error(@"Render stencil snapshot differs from original retained backing");}
    else if(stencilPass.target.handle)return metal_queue_error(@"Unexpected stencil backing without retained attachment");
    if(operation[@"renderClear"]){int error=samples>1?sweetgl_metal_render_clear_multisample(&pass,depth?&depthPass:nullptr,stencil?&stencilPass:nullptr,samples,sequence):stencil?sweetgl_metal_render_clear_stencil(&pass,depth?&depthPass:nullptr,&stencilPass,sequence):
            depth?sweetgl_metal_render_clear_depth(&pass,&depthPass,sequence):sweetgl_metal_render_clear(&pass,sequence);return error?metal_queue_error(@"GPU render clear failed"):nil;}
    auto pipeline=(SweetMTLRenderPipeline *)operation[@"renderPipeline"];if(![pipeline isKindOfClass:[SweetMTLRenderPipeline class]] || pipeline.device!=device || drawData.length!=sizeof(SweetRenderDraw))return metal_queue_error(@"Render pipeline/draw ABI invalid");
    if(pipeline->_depthFormat!=(depth?depth.pixelFormat:MTLPixelFormatInvalid))return metal_queue_error(@"Render depth pipeline/pass format differs");
    if(pipeline->_stencilFormat!=(stencil?stencil.pixelFormat:MTLPixelFormatInvalid))return metal_queue_error(@"Render stencil pipeline/pass format differs");
    if(pipeline->_samples!=samples)return metal_queue_error(@"Render raster sample count differs from pass");
    SweetRenderDraw draw;memcpy(&draw,drawData.bytes,sizeof(draw));SweetShaderBufferBinding buffers[16]={};SweetShaderTextureBinding textures[32]={};SweetShaderSamplerBinding samplers[16]={};unsigned n=0,t=0,s=0;
    for(unsigned stage=1;stage<=2;++stage){NSDictionary *bindings=operation[stage==1?@"vertexBuffers":@"fragmentBuffers"];
        for(NSNumber *key in bindings){NSDictionary *binding=bindings[key];auto buffer=(SweetMTLBuffer *)binding[@"buffer"];uint64_t offset=[binding[@"offset"] unsignedLongLongValue];
            if(n>=16 || key.unsignedLongLongValue>=8 || ![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=device ||
                offset>=buffer.length)return metal_queue_error(@"Render buffer owner/range invalid");
            for(id object in targets)if(object!=[NSNull null] && buffer->_backing.handle==[(SweetMTLTexture *)object sweetBacking].handle)return metal_queue_error(@"Render buffer aliases a color target");
            if(depth && buffer->_backing.handle==[depth sweetBacking].handle)return metal_queue_error(@"Render buffer aliases depth target");
            if(stencil && buffer->_backing.handle==[stencil sweetBacking].handle)return metal_queue_error(@"Render buffer aliases stencil target");
            buffers[n++]={buffer->_backing.handle,key.unsignedIntValue+(stage==2?8:0),offset,buffer.length-offset};}
        bindings=operation[stage==1?@"vertexTextures":@"fragmentTextures"];for(NSNumber *key in bindings){auto texture=(SweetMTLTexture *)bindings[key];
            if(t>=32 || ![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device || texture.storageMode==MTLStorageModeMemoryless)return metal_queue_error(@"Render texture owner invalid");
            for(id object in targets)if(object!=[NSNull null]){auto other=(SweetMTLTexture *)object;
                if([texture sweetBacking].handle==[other sweetBacking].handle || (texture->_surface && texture->_surface==other->_surface))return metal_queue_error(@"Render texture aliases a color target");}
            if(depth && [texture sweetBacking].handle==[depth sweetBacking].handle)return metal_queue_error(@"Render sampled texture aliases depth target");
            if(stencil && [texture sweetBacking].handle==[stencil sweetBacking].handle)return metal_queue_error(@"Render sampled texture aliases stencil target");
            textures[t++]=[texture bindingForIndex:key.unsignedIntValue stage:stage];}
        bindings=operation[stage==1?@"vertexSamplers":@"fragmentSamplers"];for(NSNumber *key in bindings){auto sampler=(SweetMTLSampler *)bindings[key];
            if(s>=16 || ![sampler isKindOfClass:[SweetMTLSampler class]] || sampler.device!=device)return metal_queue_error(@"Render sampler owner invalid");
            samplers[s]=sampler->_binding;samplers[s].index=key.unsignedIntValue;samplers[s].stage=stage;++s;}}
    int result=samples>1?sweetgl_metal_render_draw_multisample(pipeline->_program,&pass,depth?&depthPass:nullptr,stencil?&stencilPass:nullptr,&draw,buffers,n,textures,t,samplers,s,samples,[operation[@"sampleMask"] unsignedIntValue],sequence):stencil?sweetgl_metal_render_draw_stencil(pipeline->_program,&pass,depth?&depthPass:nullptr,&stencilPass,&draw,buffers,n,textures,t,samplers,s,sequence):
        depth?sweetgl_metal_render_draw_depth(pipeline->_program,&pass,&depthPass,&draw,buffers,n,textures,t,samplers,s,sequence):
        sweetgl_metal_render_draw(pipeline->_program,&pass,&draw,buffers,n,textures,t,samplers,s,sequence);
    return result?metal_queue_error([NSString stringWithFormat:@"AIR render failed result=%d",result]):nil;
}
static BOOL record_metal_mipmaps(SweetMTLCommandBuffer *commands,id<MTLTexture> input)
{
    auto texture=(SweetMTLTexture *)input;bool valid=[texture isKindOfClass:[SweetMTLTexture class]] && texture.device==commands.device;
    fprintf(stderr,"SWEET METAL MIPMAP request class=%s width=%lu height=%lu levels=%lu format=%lu storage=%lu valid=%u\n",input?object_getClassName(input):"nil",
        (unsigned long)(valid?texture.width:0),(unsigned long)(valid?texture.height:0),(unsigned long)(valid?texture.mipmapLevelCount:0),
        (unsigned long)(valid?texture.pixelFormat:0),(unsigned long)(valid?texture.storageMode:0),valid);
    if(!valid || texture.storageMode==MTLStorageModeMemoryless || texture.pixelFormat==MTLPixelFormatDepth32Float || texture.pixelFormat==MTLPixelFormatStencil8)return NO;SweetShaderTextureBinding binding=[texture bindingForIndex:0 stage:0];
    @autoreleasepool{return [commands appendOperation:@{@"mipmapTexture":texture,@"binding":[NSData dataWithBytes:&binding length:sizeof(binding)]}];}
}
static NSError *execute_metal_mipmaps(NSDictionary *operation,id<MTLDevice> device,uint32_t *sequence,uint32_t *operations)
{
    *sequence=*operations=0;auto texture=(SweetMTLTexture *)operation[@"mipmapTexture"];NSData *snapshot=operation[@"binding"];
    if(![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device || snapshot.length!=sizeof(SweetShaderTextureBinding))return metal_queue_error(@"Mipmap texture owner or ABI invalid");
    SweetShaderTextureBinding saved;memcpy(&saved,snapshot.bytes,sizeof(saved));auto actual=[texture bindingForIndex:0 stage:0];
    if(memcmp(&saved,&actual,sizeof(actual)))return metal_queue_error(@"Mipmap backing differs from retained snapshot");
    int result=sweetgl_metal_texture_mipmaps(&actual,sequence);if(result)return metal_queue_error([NSString stringWithFormat:@"GPU mipmaps failed result=%d",result]);
    *operations=actual.mip_count-1;return nil;
}
int SweetMTLRenderPipelineCacheState(id<MTLRenderPipelineState> pipeline)
{
    if(![pipeline isKindOfClass:[SweetMTLRenderPipeline class]])return -EINVAL;
    return sweetgl_metal_program_cache_state(((SweetMTLRenderPipeline *)pipeline)->_program);
}
