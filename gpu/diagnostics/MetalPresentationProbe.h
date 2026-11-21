/* SPDX-License-Identifier: BSD-2-Clause; authored pixels in real native drawables.
 * A successful GPU-to-drawable handoff still needs independent UI/optical proof. */
#pragma once
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CATransaction.h>
#include <CommonCrypto/CommonDigest.h>
#include <dlfcn.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include "../metal/NativePresentation16.h"
#include "../sweetgl/MetalBackingAPI.h"
@protocol SweetPresentationPageObserve
- (SweetMetalBacking)sweetBacking;
- (BOOL)isDrawable;
@end
static bool presentation_image_uuid(Class owner,const char *selector,const unsigned char uuid[16])
{
    Method method=owner?class_getInstanceMethod(owner,sel_registerName(selector)):NULL;Dl_info info={};
    if(!method || !dladdr((const void *)method_getImplementation(method),&info) || !info.dli_fbase)return false;
    const struct mach_header_64 *header=(const struct mach_header_64 *)info.dli_fbase;
    if(header->magic!=MH_MAGIC_64 || header->ncmds>4096 || header->sizeofcmds>1u<<20)return false;
    const unsigned char *at=(const unsigned char *)(header+1),*end=at+header->sizeofcmds;unsigned found=0;
    for(unsigned i=0;i<header->ncmds;++i){if(at>end || (size_t)(end-at)<sizeof(struct load_command))return false;
        const struct load_command *cmd=(const struct load_command *)at;if(cmd->cmdsize<sizeof(*cmd) || cmd->cmdsize>(size_t)(end-at))return false;
        if(cmd->cmd==LC_UUID){if(cmd->cmdsize!=sizeof(struct uuid_command) || memcmp(((const struct uuid_command *)cmd)->uuid,uuid,16))return false;++found;}at+=cmd->cmdsize;
    }return found==1 && at==end;
}
static bool presentation_original_objects(MetalABIEmit emit)
{
    const unsigned char quartz[16]={0x9e,0x40,0xb2,0xeb,0x26,0x0c,0x3a,0xf1,0xae,0x45,0xa1,0xa5,0x51,0xa8,0xc1,0xb9};
    bool valid=presentation_image_uuid(objc_lookUpClass("CAMetalLayer"),"nextDrawable",quartz) && presentation_image_uuid(objc_lookUpClass("CAMetalDrawable"),"present",quartz);
    emit("SWEET PRESENT PROBE ABI valid=%u QuartzCore_uuid=9e40b2eb260c3af1ae45a1a551a8c1b9 original_classes=1",valid);return valid;
}
static uint64_t presentation_probe_hash(const unsigned char *bytes,unsigned count)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)h=(h^bytes[i])*UINT64_C(1099511628211);return h;}
static uint64_t presentation_probe_clock(void)
{struct timespec time={0};return clock_gettime(CLOCK_MONOTONIC,&time)?0:(uint64_t)time.tv_sec*UINT64_C(1000000000)+(uint64_t)time.tv_nsec;}
static bool metal_presentation_probe(MetalABIEmit emit)
{
    emit("SWEET PRESENT PROBE BEGIN phases=5 width=17 height=9 original_CAMetalLayer=1 original_drawable=1");
    unsigned char digest[32];CC_SHA256(sweet_air_render_fixture,(CC_LONG)sizeof(sweet_air_render_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET PRESENT PROBE INPUT fixture_sha256=%s",hex);
    bool valid=presentation_original_objects(emit);NSError *error=nil;id<MTLDevice> device=valid?SweetMTLCreateDevice(&error):nil;
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:pd error:&error];[pd release];id<MTLCommandQueue> queue=[device newCommandQueue];
    valid=valid && device && library && pipeline && queue;unsigned completed=0;uint32_t previous=0;
    for(unsigned phase=0;phase<5 && valid;++phase){@autoreleasepool{
        // This probe creates its own native layers on a background thread.
        // Close their CA transaction explicitly instead of relying on a UI
        // run-loop boundary that this thread does not own. Presentation calls
        // and original time/options are unchanged; no completion is fabricated.
        [CATransaction begin];emit("SWEET PRESENT PROBE TX phase=%u begin=1",phase);
        CAMetalLayer *layer=[CAMetalLayer new];layer.device=device;layer.pixelFormat=MTLPixelFormatBGRA8Unorm;layer.framebufferOnly=YES;
        layer.drawableSize=CGSizeMake(17,9);layer.allowsNextDrawableTimeout=YES;layer.maximumDrawableCount=2;layer.presentsWithTransaction=NO;
        id<CAMetalDrawable> drawable=[[layer nextDrawable] retain];id<MTLTexture> texture=[drawable.texture retain];IOSurfaceRef surface=texture.iosurface;
        SweetMetalBacking backing=texture?[(id<SweetPresentationPageObserve>)texture sweetBacking]:(SweetMetalBacking){0};
        unsigned char *origin=surface?IOSurfaceGetBaseAddress(surface):NULL;NSUInteger pitch=surface?IOSurfaceGetBytesPerRow(surface):0;NSUInteger bytes=pitch*9;
        valid=drawable && [drawable isKindOfClass:objc_lookUpClass("CAMetalDrawable")] && [texture isKindOfClass:objc_lookUpClass("SweetMTLTexture")] && texture.device==device &&
            [(id<SweetPresentationPageObserve>)texture isDrawable] && texture.framebufferOnly && texture.sampleCount==1 && texture.width==17 && texture.height==9 && texture.pixelFormat==MTLPixelFormatBGRA8Unorm &&
            surface && origin && pitch>=68 && !(pitch&63) && pitch<=4096 && backing.handle && !backing.reserved && backing.contents==origin && backing.logical_bytes>=bytes &&
            IOSurfaceGetBytesPerElement(surface)==4 && IOSurfaceGetWidth(surface)==17 && IOSurfaceGetHeight(surface)==9 && !IOSurfaceGetPlaneCount(surface);
        emit("SWEET PRESENT PROBE PAGE phase=%u drawable=%016llx surface=%u handle=%u GPU=0x%llx pitch=%lu bytes=%lu framebuffer_only=%u usage=%lu valid=%u",
            phase,(unsigned long long)(uintptr_t)drawable,surface?IOSurfaceGetID(surface):0,backing.handle,(unsigned long long)backing.gpu_address,(unsigned long)pitch,(unsigned long)bytes,texture.framebufferOnly,(unsigned long)texture.usage,valid);
        bool locked=false;
        if(valid){locked=IOSurfaceLock(surface,0,NULL)==KERN_SUCCESS;valid=locked;if(valid)memset(origin,0xa5,bytes);if(locked)valid=IOSurfaceUnlock(surface,0,NULL)==KERN_SUCCESS && valid;}
        unsigned char read[68]={0};bool rejected=false;if(valid){@try{[texture getBytes:read bytesPerRow:68 fromRegion:MTLRegionMake2D(0,0,17,1) mipmapLevel:0];}@catch(NSException *exception){(void)exception;rejected=true;}valid=rejected;}
        emit("SWEET PRESENT PROBE CPU phase=%u framebuffer_read_rejected=%u",phase,rejected);
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=texture;
        pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,1,1);
        id<MTLRenderCommandEncoder> encoder=valid?[commands renderCommandEncoderWithDescriptor:pass]:nil;[pass release];valid=valid && encoder;
        const float positions[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};id<MTLBuffer> vertices=[device newBufferWithBytes:positions length:sizeof(positions) options:0];
        if(valid){[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];}
        [vertices release];unsigned mode=phase<4?phase:0;double when=phase==1?CACurrentMediaTime():phase==2?0.0625:0;uint64_t whenBits=0;memcpy(&whenBits,&when,8);
        if(valid){if(phase==0)[commands presentDrawable:drawable];else if(phase==1)[commands presentDrawable:drawable atTime:when];else if(phase==2)[commands presentDrawable:drawable afterMinimumDuration:when];
            else if(phase==3){NSDictionary *options=@{@"presentationMode":@0,@"presentTimeInterval":@0};[(id<SweetNativeCommandPresent16>)commands presentDrawable:drawable options:options];}}
        emit("SWEET PRESENT PROBE REQUEST phase=%u mode=%u path=%s time_bits=%016llx",phase,mode,phase==4?"transaction_wait":"command",(unsigned long long)whenBits);
        if(valid){[commands commit];if(phase==4){[commands waitUntilScheduled];[drawable present];emit("SWEET PRESENT PROBE DIRECT phase=4 original_wait_then_present=1");}[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && commands.status==MTLCommandBufferStatusCompleted && !commands.error && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1;
        unsigned mismatch=0,guards=0;uint64_t hash=0,paddingHash=0,pixelsHash=UINT64_C(14695981039346656037);
        if(valid){locked=IOSurfaceLock(surface,kIOSurfaceLockReadOnly,NULL)==KERN_SUCCESS;valid=locked;
            if(valid){paddingHash=UINT64_C(14695981039346656037);for(unsigned y=0;y<9;++y)for(unsigned at=0;at<pitch;++at){unsigned offset=y*pitch+at;
                if(at<68){unsigned x=at/4,c=at%4;bool box=x>=3 && x<14 && y>=2 && y<7;const unsigned char yes[]={191,128,64,255},no[]={255,0,255,255};mismatch+=origin[offset]!=(box?yes[c]:no[c]);pixelsHash=(pixelsHash^origin[offset])*UINT64_C(1099511628211);}
                else{++guards;mismatch+=origin[offset]!=0xa5;paddingHash=(paddingHash^origin[offset])*UINT64_C(1099511628211);}}
                hash=presentation_probe_hash(origin,bytes);}if(locked)valid=IOSurfaceUnlock(surface,kIOSurfaceLockReadOnly,NULL)==KERN_SUCCESS && valid;}
        valid=valid && !mismatch;emit("SWEET PRESENT PROBE PHASE index=%u valid=%u sequence=%u operations=%u checked_pixels=153 guard_bytes=%u mismatches=%u",phase,valid,sequence,operations,guards,mismatch);
        emit("SWEET PRESENT PROBE HASH index=%u raw=%016llx pixels=%016llx padding=%016llx",phase,(unsigned long long)hash,(unsigned long long)pixelsHash,(unsigned long long)paddingHash);
        // Do not call didPresentAtTime: or manufacture handlers. This telemetry
        // is the native object's current value, separate from GPU acceptance.
        double presented=drawable.presentedTime;uint64_t presentedBits=0;memcpy(&presentedBits,&presented,8);
        emit("SWEET PRESENT PROBE NATIVE phase=%u presented_time_bits=%016llx optical_unverified=1",phase,(unsigned long long)presentedBits);
        if(valid){++completed;previous=sequence;}[commands release];[texture release];[drawable release];[layer release];
        [CATransaction commit];[CATransaction flush];
    }
        NSUInteger remaining=device.currentAllocatedSize;
        emit("SWEET PRESENT PROBE TX phase=%u commit=1 flush=1 pool_drained=1 allocated=%lu",phase,(unsigned long)remaining);
    }
    [queue release];[pipeline release];[vertex release];[fragment release];[library release];NSUInteger final=device.currentAllocatedSize;valid=valid && completed==5 && !final;
    emit("SWEET PRESENT PROBE END result=%u phases=%u checked_pixels=765 final_allocated=%lu original_drawables=1 display_completion_unverified=1",valid?0:1,completed,(unsigned long)final);
    [device release];return valid;
}
