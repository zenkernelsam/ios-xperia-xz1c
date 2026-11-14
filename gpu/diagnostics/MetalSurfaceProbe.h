/* Native IOSurface pages through Metal render/sample, including row padding. */
#pragma once
#include <IOSurface/IOSurfaceRef.h>
#include <mach/mach.h>
#include "../sweetgl/MetalBackingAPI.h"
@protocol SweetSurfaceBackingObserve
- (SweetMetalBacking)sweetBacking;
@end
static void surface_number(CFMutableDictionaryRef d,CFStringRef key,uint64_t v)
{int64_t value=(int64_t)v;CFNumberRef n=CFNumberCreate(NULL,kCFNumberSInt64Type,&value);CFDictionarySetValue(d,key,n);CFRelease(n);}
static void surface_probe_pixel(unsigned x,unsigned y,unsigned phase,unsigned char bgra[4])
{bool solid=phase && x>=3 && x<14 && y>=2 && y<7;bgra[0]=solid?191:0;bgra[1]=solid?128:0;bgra[2]=solid?64:255;bgra[3]=255;}
static unsigned surface_probe_check(const unsigned char *bytes,unsigned phase,bool output,uint64_t *hash)
{
    unsigned size=output?1344:1152,mismatch=0;*hash=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<size;++i){unsigned char expected=output?0xe7:0xa5;unsigned offset=output?128:0;
        if(i>=offset && i<offset+1152 && (!output || phase==2)){unsigned row=(i-offset)/128,column=(i-offset)%128;
            if(column<68){unsigned char pixel[4];surface_probe_pixel(column/4,row,phase,pixel);expected=pixel[column%4];}}
        mismatch+=bytes[i]!=expected;*hash=(*hash^bytes[i])*UINT64_C(1099511628211);
    }return mismatch;
}
static bool metal_surface_probe(MetalABIEmit emit)
{
    emit("SWEET METAL SURFACE begin cases=2 phases=3 width=17 height=9 pitch=128 original_IOSurface=1 caller_release=1");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    id<MTLFunction> sampleVertex=[library newFunctionWithName:@"texture_position"],sampleFragment=[library newFunctionWithName:@"textured"];
    MTLRenderPipelineDescriptor *pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=vertex;pipeline.fragmentFunction=fragment;
    pipeline.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> solid=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
    pipeline.vertexFunction=sampleVertex;pipeline.fragmentFunction=sampleFragment;id<MTLRenderPipelineState> sampled=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];[pipeline release];
    const float positions[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};
    const float coordinates[24]={-1,1,0.5f,1,0,0,0,0,3,1,0.5f,1,34,0,0,0,-1,-3,0.5f,1,0,18,0,0};
    id<MTLBuffer> vertices=[device newBufferWithBytes:positions length:sizeof(positions) options:0],coordinatesBuffer=[device newBufferWithBytes:coordinates length:sizeof(coordinates) options:0];
    id<MTLCommandQueue> queue=[device newCommandQueue];MTLSamplerDescriptor *filter=[MTLSamplerDescriptor new];id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:filter];[filter release];
    bool valid=device && library && solid && sampled && vertices && coordinatesBuffer && queue && sampler;unsigned passed=0;
    for(unsigned offset=64;offset<=128 && valid;offset+=64){@autoreleasepool{
        vm_address_t caller=0;IOSurfaceRef surface=NULL;bool callerReleased=false,surfaceReleased=false;
        valid=vm_allocate(mach_task_self(),&caller,16384,VM_FLAGS_ANYWHERE)==KERN_SUCCESS;
        if(valid){memset((void *)(uintptr_t)caller,0xa5,16384);CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
            surface_number(d,CFSTR("IOSurfaceWidth"),17);surface_number(d,CFSTR("IOSurfaceHeight"),9);surface_number(d,CFSTR("IOSurfaceBytesPerElement"),4);
            surface_number(d,CFSTR("IOSurfaceBytesPerRow"),128);surface_number(d,CFSTR("IOSurfaceAllocSize"),1152);surface_number(d,CFSTR("IOSurfacePixelFormat"),0x42475241);
            surface_number(d,CFSTR("IOSurfaceAddress"),caller+offset);surface=IOSurfaceCreate(d);CFRelease(d);valid=surface!=NULL;}
        unsigned char *original=surface?IOSurfaceGetBaseAddress(surface):NULL;uint32_t identity=surface?IOSurfaceGetID(surface):0;
        MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=17;d.height=9;d.storageMode=MTLStorageModeShared;
        d.pixelFormat=MTLPixelFormatBGRA8Unorm;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
        id<MTLTexture> target=surface?[device newTextureWithDescriptor:d iosurface:surface plane:0]:nil;
        id<MTLBuffer> output=[device newBufferWithLength:1344 options:0];id<MTLTexture> destination=[output newTextureWithDescriptor:d offset:128 bytesPerRow:128];[d release];
        unsigned char *outputOriginal=output.contents;SweetMetalBacking backing={0};if(target)backing=[(id<SweetSurfaceBackingObserve>)target sweetBacking];
        valid=valid && original && target && output && destination && outputOriginal && target.iosurface==surface && !target.buffer &&
            backing.contents==original && backing.logical_bytes==1152 && backing.allocated_bytes==1152 && backing.handle && !backing.reserved &&
            ((uintptr_t)original&16383)==offset && (backing.gpu_address&16383)==offset;
        emit("SWEET METAL SURFACE backing offset=%u id=%u handle=%u gpu=%#llx cpu=%#llx bytes=%llu original_origin=1 valid=%u",offset,identity,backing.handle,
            (unsigned long long)backing.gpu_address,(unsigned long long)(uintptr_t)original,(unsigned long long)backing.logical_bytes,valid);
        if(valid)memset(outputOriginal,0xe7,1344);uint32_t previous=0;
        for(unsigned phase=0;phase<3 && valid;++phase){
            if(phase==1){callerReleased=vm_deallocate(mach_task_self(),caller,16384)==KERN_SUCCESS;if(callerReleased)caller=0;
                CFRelease(surface);surface=NULL;surfaceReleased=true;valid=callerReleased && target.iosurface && IOSurfaceGetBaseAddress(target.iosurface)==original;
                emit("SWEET METAL SURFACE retained offset=%u caller_unmapped=%u caller_CF_released=%u texture_holds_surface=%u",offset,callerReleased,surfaceReleased,valid);}
            MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=phase==2?destination:target;
            pass.colorAttachments[0].loadAction=phase==0?MTLLoadActionClear:MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,0,1);
            id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=valid && encoder && commands;
            if(valid && phase){[encoder setRenderPipelineState:phase==2?sampled:solid];[encoder setVertexBuffer:phase==2?coordinatesBuffer:vertices offset:0 atIndex:0];
                if(phase==1)[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];else {[encoder setFragmentTexture:target atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];}
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}
            if(valid){[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
            uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 && commands.status==MTLCommandBufferStatusCompleted && !commands.error;previous=sequence;
            uint64_t originalHash=0,outputHash=0;unsigned mismatch=0;bool locked=false;
            if(valid){locked=IOSurfaceLock(target.iosurface,kIOSurfaceLockReadOnly,NULL)==KERN_SUCCESS;valid=locked && output.contents==outputOriginal;
                if(valid)mismatch=surface_probe_check(original,phase,false,&originalHash)+surface_probe_check(outputOriginal,phase,true,&outputHash);
                if(locked)valid=IOSurfaceUnlock(target.iosurface,kIOSurfaceLockReadOnly,NULL)==KERN_SUCCESS && valid;}
            valid=valid && !mismatch;
            emit("SWEET METAL SURFACE phase offset=%u phase=%u sequence=%u operations=%u status=%lu error=%u mismatches=%u raw_hash=%016llx output_hash=%016llx original_pages=1 valid=%u",
                offset,phase,sequence,operations,(unsigned long)commands.status,commands.error!=nil,mismatch,(unsigned long long)originalHash,(unsigned long long)outputHash,valid);
            if(!valid && commands.error)emit("SWEET METAL SURFACE failure offset=%u phase=%u reason=%s",offset,phase,commands.error.localizedDescription.UTF8String);[commands release];
        }
        if(valid)++passed;[target release];[destination release];[output release];if(surface)CFRelease(surface);if(caller)vm_deallocate(mach_task_self(),caller,16384);
    }}
    [sampler release];[queue release];[vertices release];[coordinatesBuffer release];[solid release];[sampled release];[vertex release];[fragment release];[sampleVertex release];[sampleFragment release];[library release];
    NSUInteger final=device.currentAllocatedSize;valid=valid && passed==2 && final==0;
    emit("SWEET METAL SURFACE END result=%u cases=%u phases=6 checked_bytes=14976 row_padding=6480 final_allocated=%lu original_surface_GPU=1",valid?0:1,passed,(unsigned long)final);
    [device release];return valid;
}
