// SPDX-License-Identifier: BSD-2-Clause
// Native Metal objects over the existing linear resource and Mesa contract.
#include "MetalTextureRules.h"
#include <cmath>
#include "../sweetgl/XRColourShader.h"
#include <IOSurface/IOSurfaceRef.h>
@interface SweetMTLTexture : NSObject <MTLTexture> {
@public SweetMTLBuffer *_buffer;SweetTextureLayout _layout;MTLPixelFormat _format;MTLTextureUsage _usage;NSString *_label;
    IOSurfaceRef _surface;SweetMTLDevice *_surfaceOwner;SweetMetalBacking _surfaceBacking;MTLResourceOptions _surfaceOptions;
}
- (instancetype)initWithBuffer:(SweetMTLBuffer *)buffer descriptor:(MTLTextureDescriptor *)descriptor offset:(NSUInteger)offset pitch:(NSUInteger)pitch;
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)descriptor surface:(IOSurfaceRef)surface plane:(NSUInteger)plane;
- (SweetMetalBacking)sweetBacking;
- (void *)synchronizedContents;
- (SweetShaderTextureBinding)bindingForIndex:(unsigned)index stage:(unsigned)stage;
@end
@interface SweetMTLSampler : NSObject <MTLSamplerState> {
@public SweetMTLDevice *_owner;SweetShaderSamplerBinding _binding;NSString *_label;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLSamplerDescriptor *)descriptor;
@end
@interface SweetMTLRenderPipeline : NSObject <MTLRenderPipelineState> {
@public SweetMTLDevice *_owner;SweetMTLFunction *_vertex,*_fragment;SweetMetalProgram *_program;
    MTLRenderPipelineDescriptor *_descriptor;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLRenderPipelineDescriptor *)descriptor error:(NSError **)error;
@end
@interface SweetMTLRenderEncoder : NSObject <MTLRenderCommandEncoder> {
    SweetMTLCommandBuffer *_commands;SweetMTLTexture *_target;SweetMTLRenderPipeline *_pipeline;
    NSMutableDictionary *_vertexBuffers,*_fragmentBuffers,*_vertexTextures,*_fragmentTextures,*_vertexSamplers,*_fragmentSamplers;
    SweetRenderPass _pass;SweetRenderDraw _draw;BOOL _ended,_drew;NSString *_label;
}
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands descriptor:(MTLRenderPassDescriptor *)descriptor;
@end

@implementation SweetMTLTexture
- (instancetype)initWithBuffer:(SweetMTLBuffer *)buffer descriptor:(MTLTextureDescriptor *)d offset:(NSUInteger)offset pitch:(NSUInteger)pitch
{
    if(!(self=[super init]))return nil;
    if(![buffer isKindOfClass:[SweetMTLBuffer class]] || !d || d.textureType!=MTLTextureType2D || d.depth!=1 || d.arrayLength!=1 || d.mipmapLevelCount!=1 ||
        d.sampleCount!=1 || !sweet_metal_shared_options(d.resourceOptions) || d.storageMode!=MTLStorageModeShared ||
        (d.usage&~(MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget)) ||
        !sweet_texture_layout(d.width,d.height,(uint32_t)d.pixelFormat,offset,pitch,buffer.length,_layout)){
        [self release];return nil;}
    _buffer=[buffer retain];_format=d.pixelFormat;_usage=d.usage;return self;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLTextureDescriptor *)d surface:(IOSurfaceRef)surface plane:(NSUInteger)plane
{
    if(!(self=[super init]))return nil;
    if(surface)_surface=(IOSurfaceRef)CFRetain(surface);
    unsigned pixel=d?sweet_texture_pixel_bytes((uint32_t)d.pixelFormat):0;bool valid=owner && d && surface && !plane && !IOSurfaceGetPlaneCount(surface) &&
        d.textureType==MTLTextureType2D && d.depth==1 && d.arrayLength==1 && d.mipmapLevelCount==1 && d.sampleCount==1 &&
        (d.storageMode==MTLStorageModeShared || d.storageMode==MTLStorageModePrivate) &&
        sweet_metal_shared_options(d.resourceOptions&~MTLResourceStorageModeMask) &&
        !(d.usage&~(MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget)) && pixel && d.width==IOSurfaceGetWidth(surface) && d.height==IOSurfaceGetHeight(surface) &&
        IOSurfaceGetBytesPerElement(surface)==pixel && d.width && d.height && IOSurfaceGetBaseAddress(surface) &&
        !((uintptr_t)IOSurfaceGetBaseAddress(surface)&63) && !(IOSurfaceGetBytesPerRow(surface)&63);
    if(valid){uint64_t pitch=IOSurfaceGetBytesPerRow(surface),capacity=IOSurfaceGetAllocSize(surface);
        valid=sweet_texture_layout(d.width,d.height,(uint32_t)d.pixelFormat,0,pitch,capacity,_layout);
        if(valid && (d.usage&MTLTextureUsageRenderTarget)){auto lock=IOSurfaceLock(surface,0,nullptr);valid=lock==KERN_SUCCESS;
            if(valid)valid=IOSurfaceUnlock(surface,0,nullptr)==KERN_SUCCESS;}
        if(valid)valid=sweetgl_metal_surface_import(IOSurfaceGetID(surface),(uintptr_t)IOSurfaceGetBaseAddress(surface),_layout.bytes,&_surfaceBacking)==0 &&
            sweet_metal_surface_backing_valid(&_surfaceBacking);}
    if(!valid){if(_surfaceBacking.handle)(void)sweetgl_metal_buffer_release(_surfaceBacking.handle);memset(&_surfaceBacking,0,sizeof(_surfaceBacking));
        fprintf(stderr,"SWEET METAL IOSURFACE rejected id=%u plane=%lu format=%lu width=%lu height=%lu\n",surface?IOSurfaceGetID(surface):0,(unsigned long)plane,(unsigned long)(d?d.pixelFormat:0),(unsigned long)(d?d.width:0),(unsigned long)(d?d.height:0));[self release];return nil;}
    _surfaceOwner=[owner retain];_format=d.pixelFormat;_usage=d.usage;_surfaceOptions=d.resourceOptions&~MTLResourceCPUCacheModeMask;
    [_surfaceOwner->_resourceLock lock];_surfaceOwner->_allocatedBytes+=_surfaceBacking.allocated_bytes;[_surfaceOwner->_resourceLock unlock];
    return self;
}
- (void)dealloc{if(_surfaceBacking.handle){int status=sweetgl_metal_buffer_release(_surfaceBacking.handle);
        if(!status){[_surfaceOwner->_resourceLock lock];_surfaceOwner->_allocatedBytes-=_surfaceBacking.allocated_bytes;[_surfaceOwner->_resourceLock unlock];}
        else fprintf(stderr,"SWEET METAL IOSURFACE release failed handle=%u status=%d\n",_surfaceBacking.handle,status);}
    if(_surface)CFRelease(_surface);[_surfaceOwner release];[_buffer release];[_label release];[super dealloc];}
- (SweetMetalBacking)sweetBacking{return _surface?_surfaceBacking:_buffer->_backing;}
- (void *)synchronizedContents{return _surface?(sweetgl_metal_buffer_synchronize(_surfaceBacking.handle)?nullptr:_surfaceBacking.contents):_buffer.contents;}
- (id<MTLDevice>)device{return _surface?_surfaceOwner:_buffer.device;}
- (id<MTLBuffer>)buffer{return _buffer;}
- (IOSurfaceRef)iosurface{return _surface;}
- (NSUInteger)iosurfacePlane{return 0;}
- (NSUInteger)bufferOffset{return _layout.offset;}
- (NSUInteger)bufferBytesPerRow{return _layout.pitch;}
- (NSUInteger)width{return _layout.width;}
- (NSUInteger)height{return _layout.height;}
- (NSUInteger)depth{return 1;}
- (NSUInteger)arrayLength{return 1;}
- (NSUInteger)mipmapLevelCount{return 1;}
- (NSUInteger)sampleCount{return 1;}
- (MTLTextureType)textureType{return MTLTextureType2D;}
- (MTLPixelFormat)pixelFormat{return _format;}
- (MTLTextureUsage)usage{return _usage;}
- (MTLStorageMode)storageMode{return _surface?(MTLStorageMode)((_surfaceOptions&MTLResourceStorageModeMask)>>MTLResourceStorageModeShift):MTLStorageModeShared;}
- (MTLCPUCacheMode)cpuCacheMode{return MTLCPUCacheModeDefaultCache;}
- (MTLHazardTrackingMode)hazardTrackingMode{return _surface?MTLHazardTrackingModeTracked:_buffer.hazardTrackingMode;}
- (MTLResourceOptions)resourceOptions{return _surface?_surfaceOptions:_buffer.resourceOptions;}
- (id<MTLHeap>)heap{return nil;}
- (NSUInteger)heapOffset{return 0;}
- (id<MTLTexture>)parentTexture{return nil;}
- (BOOL)isAliasable{return NO;}
- (BOOL)allowGPUOptimizedContents{return NO;}
- (NSUInteger)allocatedSize{return _surface?_surfaceBacking.allocated_bytes:_buffer.allocatedSize;}
- (BOOL)framebufferOnly{return NO;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)replaceRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level withBytes:(const void *)bytes bytesPerRow:(NSUInteger)pitch
{
    uint64_t offset=0,span=0;
    if(self.storageMode!=MTLStorageModeShared || level || region.origin.z || region.size.depth!=1 || !sweet_texture_region(_layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (span && (!bytes || span>UINTPTR_MAX-(uintptr_t)bytes)))[NSException raise:NSInvalidArgumentException format:@"Invalid Metal texture upload region/stride"];
    if(!span)return;unsigned char *target=(unsigned char *)[self synchronizedContents];if(!target)[NSException raise:NSInternalInconsistencyException format:@"Texture backing synchronization failed"];
    if(_surface && IOSurfaceLock(_surface,0,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface write lock failed"];
    for(NSUInteger row=0;row<region.size.height;++row)memcpy(target+offset+row*_layout.pitch,(const unsigned char *)bytes+row*pitch,region.size.width*_layout.bytes_per_pixel);
    if(_surface && IOSurfaceUnlock(_surface,0,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface write unlock failed"];
}
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)pitch fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level
{
    uint64_t offset=0,span=0;
    if(self.storageMode!=MTLStorageModeShared || level || region.origin.z || region.size.depth!=1 || !sweet_texture_region(_layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (span && (!bytes || span>UINTPTR_MAX-(uintptr_t)bytes)))[NSException raise:NSInvalidArgumentException format:@"Invalid Metal texture read region/stride"];
    if(!span)return;const unsigned char *source=(const unsigned char *)[self synchronizedContents];if(!source)[NSException raise:NSInternalInconsistencyException format:@"Texture backing synchronization failed"];
    if(_surface && IOSurfaceLock(_surface,kIOSurfaceLockReadOnly,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface read lock failed"];
    for(NSUInteger row=0;row<region.size.height;++row)memcpy((unsigned char *)bytes+row*pitch,source+offset+row*_layout.pitch,region.size.width*_layout.bytes_per_pixel);
    if(_surface && IOSurfaceUnlock(_surface,kIOSurfaceLockReadOnly,nullptr)!=KERN_SUCCESS)[NSException raise:NSInternalInconsistencyException format:@"Original IOSurface read unlock failed"];
}
- (void)replaceRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice withBytes:(const void *)bytes bytesPerRow:(NSUInteger)pitch bytesPerImage:(NSUInteger)imagePitch
{
    uint64_t offset=0,span=0;if(slice || !sweet_texture_region(_layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (imagePitch && imagePitch<span))[NSException raise:NSInvalidArgumentException format:@"Invalid 2D texture slice/image stride"];
    [self replaceRegion:region mipmapLevel:level withBytes:bytes bytesPerRow:pitch];
}
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)pitch bytesPerImage:(NSUInteger)imagePitch fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice
{
    uint64_t offset=0,span=0;if(slice || !sweet_texture_region(_layout,region.origin.x,region.origin.y,region.size.width,region.size.height,pitch,offset,span) ||
        (imagePitch && imagePitch<span))[NSException raise:NSInvalidArgumentException format:@"Invalid 2D texture slice/image stride"];
    [self getBytes:bytes bytesPerRow:pitch fromRegion:region mipmapLevel:level];
}
- (void)doesNotRecognizeSelector:(SEL)selector
{fprintf(stderr,"SWEET METAL missing class=texture selector=%s original_exception=1\n",sel_getName(selector));[super doesNotRecognizeSelector:selector];}
- (SweetShaderTextureBinding)bindingForIndex:(unsigned)index stage:(unsigned)stage
{return {[self sweetBacking].handle,index,stage,_layout.width,_layout.height,_layout.pitch,(uint32_t)_format,(uint32_t)_usage,_layout.offset};}
@end

@implementation SweetMTLSampler
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLSamplerDescriptor *)d
{
    if(!(self=[super init]))return nil;
    if(!owner || !d || d.minFilter>MTLSamplerMinMagFilterLinear || d.magFilter>MTLSamplerMinMagFilterLinear || d.mipFilter!=MTLSamplerMipFilterNotMipmapped ||
        (d.sAddressMode!=MTLSamplerAddressModeClampToEdge && d.sAddressMode!=MTLSamplerAddressModeRepeat && d.sAddressMode!=MTLSamplerAddressModeMirrorRepeat) ||
        (d.tAddressMode!=MTLSamplerAddressModeClampToEdge && d.tAddressMode!=MTLSamplerAddressModeRepeat && d.tAddressMode!=MTLSamplerAddressModeMirrorRepeat) ||
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
    NSData *source=function->_library->_entries[function->_name];std::vector<GPU::Compiler::Entry> entries;
    if(!source || !GPU::Compiler::lowerAIR(source.bytes,source.length,entries,error,function->_name.UTF8String,function->_constants) || entries.size()!=1)return false;
    entry=std::move(entries[0]);return true;
}
@implementation SweetMTLRenderPipeline
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLRenderPipelineDescriptor *)d error:(NSError **)error
{
    if(!(self=[super init]))return nil;std::string diagnostic;
    if(!owner || !d){if(error)*error=metal_compile_error("Render descriptor and device required");[self release];return nil;}
    auto vertex=(SweetMTLFunction *)d.vertexFunction,fragment=(SweetMTLFunction *)d.fragmentFunction;
    auto color=d.colorAttachments[0];bool valid=owner && d && vertex.device==owner && fragment.device==owner && d.rasterSampleCount==1 &&
        d.depthAttachmentPixelFormat==MTLPixelFormatInvalid && d.stencilAttachmentPixelFormat==MTLPixelFormatInvalid &&
        !d.alphaToCoverageEnabled && !d.alphaToOneEnabled && d.rasterizationEnabled && !d.vertexDescriptor.layouts[0].stride &&
        sweet_texture_pixel_bytes((uint32_t)color.pixelFormat);
    for(unsigned i=1;i<8 && valid;++i)valid=d.colorAttachments[i].pixelFormat==MTLPixelFormatInvalid;
    GPU::Compiler::Entry vs,fs;std::string vertex_source,fragment_source;
    if(valid)valid=render_entry(vertex,1,vs,diagnostic) && render_entry(fragment,2,fs,diagnostic);
    if(valid)valid=!(vs.reflection.attributeMask || fs.reflection.framebufferMask || vs.reflection.writeMask) &&
        SweetAIR::graphicsESSL(vs.glsl,1,vs.reflection,vertex_source,diagnostic) && SweetAIR::graphicsESSL(fs.glsl,2,fs.reflection,fragment_source,diagnostic);
    if(valid)for(unsigned i=0;i<fs.reflection.varyingCount;++i){bool found=false;auto &input=fs.reflection.varyings[i];
        for(unsigned j=0;j<vs.reflection.varyingCount;++j){auto &output=vs.reflection.varyings[j];if(!memcmp(input.semantic,output.semantic,sizeof(input.semantic)) && input.components==output.components && input.mode==output.mode){found=true;break;}}
        if(!found){valid=false;diagnostic="AIR vertex/fragment varyings disagree";break;}}
    SweetRenderPipelineConfig config={};config.format=(uint32_t)color.pixelFormat;config.vertex_buffers=vs.reflection.bufferMask;
    config.fragment_buffers=fs.reflection.bufferMask;config.fragment_writes=fs.reflection.writeMask;config.vertex_texture_slots=vs.reflection.textureBindings;config.fragment_texture_slots=fs.reflection.textureBindings;
    config.blend_enabled=color.blendingEnabled;config.source_rgb=(uint32_t)color.sourceRGBBlendFactor;config.destination_rgb=(uint32_t)color.destinationRGBBlendFactor;config.operation_rgb=(uint32_t)color.rgbBlendOperation;
    config.source_alpha=(uint32_t)color.sourceAlphaBlendFactor;config.destination_alpha=(uint32_t)color.destinationAlphaBlendFactor;config.operation_alpha=(uint32_t)color.alphaBlendOperation;config.write_mask=(uint32_t)color.writeMask;
    if(config.source_rgb>14 || config.destination_rgb>14 || config.source_alpha>14 || config.destination_alpha>14 || config.operation_rgb>4 || config.operation_alpha>4 || config.write_mask>15)valid=false;
    if(valid && config.format>=552 && config.format<=555){
        if(config.blend_enabled || config.write_mask!=15){valid=false;diagnostic="XR render blending/partial-channel writes require a floating-point shadow";}
        const std::string output="layout(location=0) out vec4 _qcolor;";auto at=fragment_source.find(output);
        if(at==std::string::npos){valid=false;diagnostic="XR render requires a single actual AIR vec4 color output";}
        if(valid){fragment_source.replace(at,output.size(),"vec4 _qcolor;\nlayout(location=0) out highp uvec4 _smtl_xr_output;");
            auto main=fragment_source.find("void main() {");fragment_source.insert(main,sweet_xr_shader);
            fragment_source.insert(fragment_source.rfind('}'),"_smtl_xr_output = _smtl_xr_pack(_qcolor,"+std::to_string(config.format)+"u);\n");}}
    char message[4096]={0};if(valid)_program=sweetgl_metal_render_create(vertex_source.c_str(),fragment_source.c_str(),&config,message,sizeof(message));
    if(!_program){if(diagnostic.empty())diagnostic=message;if(error)*error=metal_compile_error(diagnostic);[self release];return nil;}
    _owner=[owner retain];_vertex=[vertex retain];_fragment=[fragment retain];_descriptor=[d copy];return self;
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
    if(!(self=[super init]))return nil;auto target=(SweetMTLTexture *)d.colorAttachments[0].texture;
    bool valid=d && [target isKindOfClass:[SweetMTLTexture class]] && target.device==commands.device && (target.usage&MTLTextureUsageRenderTarget) &&
        !d.depthAttachment.texture && !d.stencilAttachment.texture && !d.colorAttachments[0].resolveTexture && d.colorAttachments[0].level==0 &&
        d.colorAttachments[0].slice==0 && d.colorAttachments[0].depthPlane==0 && d.colorAttachments[0].loadAction<=MTLLoadActionClear &&
        d.colorAttachments[0].storeAction<=MTLStoreActionStore;
    for(unsigned i=1;i<8 && valid;++i)valid=d.colorAttachments[i].texture==nil;
    if(!valid){[self release];return nil;}_commands=[commands retain];_target=[target retain];
    auto attachment=d.colorAttachments[0];_pass.target=[target bindingForIndex:0 stage:0];_pass.load_action=(uint32_t)attachment.loadAction;_pass.store_action=(uint32_t)attachment.storeAction;
    _pass.clear[0]=attachment.clearColor.red;_pass.clear[1]=attachment.clearColor.green;_pass.clear[2]=attachment.clearColor.blue;_pass.clear[3]=attachment.clearColor.alpha;
    _draw={(uint32_t)MTLPrimitiveTypeTriangle,0,0,1,(uint32_t)MTLCullModeNone,(uint32_t)MTLWindingClockwise,0,0,(double)target.width,(double)target.height,0,1,0,0,(uint32_t)target.width,(uint32_t)target.height};
    _vertexBuffers=[NSMutableDictionary new];_fragmentBuffers=[NSMutableDictionary new];_vertexTextures=[NSMutableDictionary new];_fragmentTextures=[NSMutableDictionary new];_vertexSamplers=[NSMutableDictionary new];_fragmentSamplers=[NSMutableDictionary new];return self;
}
- (void)dealloc{[_commands release];[_target release];[_pipeline release];[_vertexBuffers release];[_fragmentBuffers release];[_vertexTextures release];[_fragmentTextures release];[_vertexSamplers release];[_fragmentSamplers release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _commands.device;}
- (void)doesNotRecognizeSelector:(SEL)selector
{fprintf(stderr,"SWEET METAL missing class=render_encoder selector=%s original_exception=1\n",sel_getName(selector));[super doesNotRecognizeSelector:selector];}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)setRenderPipelineState:(id<MTLRenderPipelineState>)pipeline
{if(_ended || ![pipeline isKindOfClass:[SweetMTLRenderPipeline class]] || pipeline.device!=self.device)[NSException raise:NSInvalidArgumentException format:@"Invalid Metal render pipeline owner"];
    [pipeline retain];[_pipeline release];_pipeline=(SweetMTLRenderPipeline *)pipeline;}
- (void)setBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset index:(NSUInteger)index dictionary:(NSMutableDictionary *)dictionary
{
    if(_ended || index>=8 || (buffer && (![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=self.device || offset>=buffer.length || (offset&3))))
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal render buffer range/owner"];
    @autoreleasepool{if(buffer)dictionary[@(index)]=@{@"buffer":buffer,@"offset":@(offset)};else [dictionary removeObjectForKey:@(index)];}
}
- (void)setVertexBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index{[self setBuffer:buffer offset:offset index:index dictionary:_vertexBuffers];}
- (void)setFragmentBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index{[self setBuffer:buffer offset:offset index:index dictionary:_fragmentBuffers];}
- (void)setTexture:(id<MTLTexture>)texture index:(NSUInteger)index dictionary:(NSMutableDictionary *)dictionary
{if(_ended || index>=16 || (texture && (![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=self.device || !(texture.usage&MTLTextureUsageShaderRead))))
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
    _draw.primitive=(uint32_t)primitive;_draw.first=(uint32_t)start;_draw.count=(uint32_t)count;_draw.instances=(uint32_t)instances;
    @autoreleasepool{NSData *pass=[NSData dataWithBytes:&_pass length:sizeof(_pass)],*draw=[NSData dataWithBytes:&_draw length:sizeof(_draw)];
        if(![_commands appendOperation:@{@"renderPipeline":_pipeline,@"target":_target,@"pass":pass,@"draw":draw,
            @"vertexBuffers":[NSDictionary dictionaryWithDictionary:_vertexBuffers],@"fragmentBuffers":[NSDictionary dictionaryWithDictionary:_fragmentBuffers],
            @"vertexTextures":[NSDictionary dictionaryWithDictionary:_vertexTextures],@"fragmentTextures":[NSDictionary dictionaryWithDictionary:_fragmentTextures],
            @"vertexSamplers":[NSDictionary dictionaryWithDictionary:_vertexSamplers],@"fragmentSamplers":[NSDictionary dictionaryWithDictionary:_fragmentSamplers]}])
            [NSException raise:NSInternalInconsistencyException format:@"Render command buffer no longer writable"];}
    _drew=YES;_pass.load_action=MTLLoadActionLoad;
}
- (void)drawPrimitives:(MTLPrimitiveType)p vertexStart:(NSUInteger)start vertexCount:(NSUInteger)count{[self drawPrimitives:p vertexStart:start vertexCount:count instanceCount:1];}
- (void)endEncoding
{
    if(_ended)[NSException raise:NSInternalInconsistencyException format:@"Metal render encoder already ended"];
    if(!_drew && _pass.load_action==MTLLoadActionClear){@autoreleasepool{
        if(![_commands appendOperation:@{@"renderClear":@YES,@"target":_target,@"pass":[NSData dataWithBytes:&_pass length:sizeof(_pass)]}])
            [NSException raise:NSInternalInconsistencyException format:@"Render clear command buffer not writable"];}}
    _ended=YES;[_vertexBuffers removeAllObjects];[_fragmentBuffers removeAllObjects];[_vertexTextures removeAllObjects];[_fragmentTextures removeAllObjects];[_vertexSamplers removeAllObjects];[_fragmentSamplers removeAllObjects];[_pipeline release];_pipeline=nil;[_target release];_target=nil;[_commands finishEncoder];
}
@end
static id<MTLRenderCommandEncoder> create_render_encoder(SweetMTLCommandBuffer *commands,MTLRenderPassDescriptor *descriptor)
{return [[[SweetMTLRenderEncoder alloc] initWithCommands:commands descriptor:descriptor] autorelease];}
static NSError *execute_metal_render(NSDictionary *operation,id<MTLDevice> device,uint32_t *sequence)
{
    auto target=(SweetMTLTexture *)operation[@"target"];NSData *passData=operation[@"pass"],*drawData=operation[@"draw"];
    if(![target isKindOfClass:[SweetMTLTexture class]] || target.device!=device || passData.length!=sizeof(SweetRenderPass))return metal_queue_error(@"Render target/pass owner or ABI invalid");
    SweetRenderPass pass;memcpy(&pass,passData.bytes,sizeof(pass));
    auto actual=[target bindingForIndex:0 stage:0];if(memcmp(&actual,&pass.target,sizeof(actual)))return metal_queue_error(@"Render target snapshot differs from retained backing");
    if(operation[@"renderClear"]){int error=sweetgl_metal_render_clear(&pass,sequence);return error?metal_queue_error(@"GPU render clear failed"):nil;}
    auto pipeline=(SweetMTLRenderPipeline *)operation[@"renderPipeline"];if(![pipeline isKindOfClass:[SweetMTLRenderPipeline class]] || pipeline.device!=device || drawData.length!=sizeof(SweetRenderDraw))return metal_queue_error(@"Render pipeline/draw ABI invalid");
    SweetRenderDraw draw;memcpy(&draw,drawData.bytes,sizeof(draw));SweetShaderBufferBinding buffers[16]={};SweetShaderTextureBinding textures[32]={};SweetShaderSamplerBinding samplers[16]={};unsigned n=0,t=0,s=0;
    for(unsigned stage=1;stage<=2;++stage){NSDictionary *bindings=operation[stage==1?@"vertexBuffers":@"fragmentBuffers"];
        for(NSNumber *key in bindings){NSDictionary *binding=bindings[key];auto buffer=(SweetMTLBuffer *)binding[@"buffer"];uint64_t offset=[binding[@"offset"] unsignedLongLongValue];
            if(n>=16 || key.unsignedLongLongValue>=8 || ![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=device ||
                offset>=buffer.length || buffer->_backing.handle==[target sweetBacking].handle)return metal_queue_error(@"Render buffer owner/range/target alias invalid");
            buffers[n++]={buffer->_backing.handle,key.unsignedIntValue+(stage==2?8:0),offset,buffer.length-offset};}
        bindings=operation[stage==1?@"vertexTextures":@"fragmentTextures"];for(NSNumber *key in bindings){auto texture=(SweetMTLTexture *)bindings[key];
            if(t>=32 || ![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device || [texture sweetBacking].handle==[target sweetBacking].handle ||
                (texture->_surface && texture->_surface==target->_surface))return metal_queue_error(@"Render texture alias/owner invalid");
            textures[t++]=[texture bindingForIndex:key.unsignedIntValue stage:stage];}
        bindings=operation[stage==1?@"vertexSamplers":@"fragmentSamplers"];for(NSNumber *key in bindings){auto sampler=(SweetMTLSampler *)bindings[key];
            if(s>=16 || ![sampler isKindOfClass:[SweetMTLSampler class]] || sampler.device!=device)return metal_queue_error(@"Render sampler owner invalid");
            samplers[s]=sampler->_binding;samplers[s].index=key.unsignedIntValue;samplers[s].stage=stage;++s;}}
    int result=sweetgl_metal_render_draw(pipeline->_program,&pass,&draw,buffers,n,textures,t,samplers,s,sequence);
    return result?metal_queue_error([NSString stringWithFormat:@"AIR render failed result=%d",result]):nil;
}
