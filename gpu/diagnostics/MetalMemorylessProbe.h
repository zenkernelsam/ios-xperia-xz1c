/* Author-only original transient pass pages, guard bytes and queue isolation. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
#include "AttachmentPageSnapshot.h"
enum { MEMORYLESS_PHASES=8, MEMORYLESS_COLOR_BYTES=1280, MEMORYLESS_DEPTH_BYTES=1280, MEMORYLESS_STENCIL_BYTES=704 };
static uint64_t memoryless_hash(const unsigned char *bytes,unsigned count)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)h=(h^bytes[i])*UINT64_C(1099511628211);return h;}
static void memoryless_expected(unsigned phase,unsigned char color[1280],unsigned char depth[1280],unsigned char stencil[704])
{
    memset(color,0xa5,1280);memset(depth,0xb6,1280);memset(stencil,0xc7,704);
    bool accepted=phase!=1 && phase!=7;
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){bool box=x>=3 && x<14 && y>=2 && y<7;unsigned at=64+y*128+x*4;
        const unsigned char yes[]={191,128,64,255},no[]={255,0,255,255},cyan[]={255,255,0,255};
        memcpy(color+at,phase==5?cyan:box && accepted?yes:no,4);
        float z=phase==1 || phase==7?0.125f:box?0.25f:0.75f;memcpy(depth+at,&z,4);
        stencil[64+y*64+x]=phase==2?box?18:17:phase==7?box?126:129:box?phase==3?43:42:phase==3?127:17;
    }
}
typedef struct MemorylessProbePass {
    id<MTLCommandBuffer> commands;id<MTLBuffer> color,depth,stencil;
    SweetMTLMemorylessPassInfo colorInfo,depthInfo,stencilInfo;
    unsigned char *colorBytes,*depthBytes,*stencilBytes;unsigned expectedOperations;
} MemorylessProbePass;
static bool memoryless_probe_pages(MemorylessProbePass *pass,unsigned phase,id<MTLDevice> device,id<MTLCommandQueue> queue,
    id<MTLRenderPipelineState> pipeline,id<MTLTexture> color,id<MTLTexture> depth,id<MTLTexture> stencil,id<MTLBuffer> positions,MetalABIEmit emit)
{
    MTLRenderPassDescriptor *descriptor=[MTLRenderPassDescriptor new];descriptor.colorAttachments[0].texture=color;
    descriptor.colorAttachments[0].loadAction=MTLLoadActionClear;descriptor.colorAttachments[0].storeAction=phase==4 || phase==5?MTLStoreActionDontCare:MTLStoreActionStore;
    descriptor.colorAttachments[0].clearColor=phase==5?MTLClearColorMake(0,1,1,1):MTLClearColorMake(1,0,1,1);
    descriptor.depthAttachment.texture=depth;descriptor.depthAttachment.loadAction=MTLLoadActionClear;descriptor.depthAttachment.storeAction=MTLStoreActionDontCare;
    descriptor.depthAttachment.clearDepth=phase==1 || phase==7?0.125:0.75;
    descriptor.stencilAttachment.texture=stencil;descriptor.stencilAttachment.loadAction=MTLLoadActionClear;descriptor.stencilAttachment.storeAction=MTLStoreActionDontCare;
    descriptor.stencilAttachment.clearStencil=phase==3?127:phase==7?129:17;
    pass->commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[pass->commands renderCommandEncoderWithDescriptor:descriptor];[descriptor release];
    if(!encoder)return false;
    if(phase==4 || phase==5)pass->color=SweetMTLRetainMemorylessPassBuffer(encoder,0,&pass->colorInfo);
    else pass->color=[color.buffer retain];
    if(depth)pass->depth=SweetMTLRetainMemorylessPassBuffer(encoder,8,&pass->depthInfo);
    if(stencil)pass->stencil=SweetMTLRetainMemorylessPassBuffer(encoder,9,&pass->stencilInfo);
    bool valid=pass->color && pass->color.length==1280 && (!depth || (pass->depth && pass->depth.length==1280)) &&
        (!stencil || (pass->stencil && pass->stencil.length==704));
    pass->colorBytes=(unsigned char *)pass->color.contents;pass->depthBytes=(unsigned char *)pass->depth.contents;pass->stencilBytes=(unsigned char *)pass->stencil.contents;
    valid=valid && pass->colorBytes && (!depth || pass->depthBytes) && (!stencil || pass->stencilBytes);
    const unsigned slots[3]={0,8,9};
    for(unsigned i=0;i<3;++i){unsigned slot=slots[i];SweetMTLMemorylessPassInfo info=slot==0?pass->colorInfo:slot==8?pass->depthInfo:pass->stencilInfo;if(!info.handle)continue;
        valid=valid && info.pass_id && info.width==17 && info.height==9 && info.offset==64 && !info.reserved && !(info.gpu_address&16383);
        emit("SWEET MEMORYLESS PAGE phase=%u slot=%u pass=%llu public=%016llx handle=%u GPU=0x%llx bytes=%llu pitch=%u offset=%u format=%u",
            phase,slot,(unsigned long long)info.pass_id,(unsigned long long)info.public_texture,info.handle,(unsigned long long)info.gpu_address,
            (unsigned long long)info.buffer_bytes,info.pitch,info.offset,info.format);
    }
    if(!valid){[encoder endEncoding];return false;}
    memset(pass->colorBytes,0xa5,1280);if(depth)memset(pass->depthBytes,0xb6,1280);if(stencil)memset(pass->stencilBytes,0xc7,704);
    pass->expectedOperations=phase==0 || phase==2 || phase==3?2:1;
    if(phase!=5){float *bytes=(float *)positions.contents;if(!bytes){[encoder endEncoding];return false;}
        float z=phase==1 || phase==7?0.5f:0.25f;for(unsigned i=0;i<3;++i)bytes[i*4+2]=z;
        MTLDepthStencilDescriptor *stateDescriptor=[MTLDepthStencilDescriptor new];stateDescriptor.depthCompareFunction=depth?MTLCompareFunctionLess:MTLCompareFunctionAlways;stateDescriptor.depthWriteEnabled=depth!=nil;
        stateDescriptor.frontFaceStencil.stencilCompareFunction=phase==2?MTLCompareFunctionEqual:MTLCompareFunctionAlways;
        stateDescriptor.frontFaceStencil.depthStencilPassOperation=phase==2?MTLStencilOperationIncrementWrap:MTLStencilOperationReplace;
        stateDescriptor.frontFaceStencil.depthFailureOperation=phase==7?MTLStencilOperationInvert:phase==3?MTLStencilOperationIncrementClamp:MTLStencilOperationKeep;
        id<MTLDepthStencilState> state=[device newDepthStencilStateWithDescriptor:stateDescriptor];[stateDescriptor release];
        if(!state){[encoder endEncoding];return false;}[encoder setRenderPipelineState:pipeline];[encoder setDepthStencilState:state];[state release];
        [encoder setVertexBuffer:positions offset:0 atIndex:0];[encoder setStencilReferenceValue:phase==2?17:42];
        [encoder setScissorRect:(MTLScissorRect){3,2,11,5}];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        if(pass->expectedOperations==2){
            // A second position buffer retains the first draw's original input.
            const float later[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};id<MTLBuffer> second=[device newBufferWithBytes:later length:sizeof(later) options:0];
            if(!second){[encoder endEncoding];return false;}[encoder setVertexBuffer:second offset:0 atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[second release];
        }}
    [encoder endEncoding];return true;
}
static bool memoryless_probe_check(MemorylessProbePass *pass,unsigned phase,uint32_t *previous,MetalABIEmit emit)
{
    uint32_t sequence=0,operations=0;bool valid=pass->commands.status==MTLCommandBufferStatusCompleted && !pass->commands.error &&
        SweetMTLCommandResult(pass->commands,&sequence,&operations) && sequence>*previous && operations==pass->expectedOperations && pass->color.contents==pass->colorBytes &&
        (!pass->depth || pass->depth.contents==pass->depthBytes) && (!pass->stencil || pass->stencil.contents==pass->stencilBytes);
    unsigned mismatch=0,checked=1280+(pass->depth?1280:0)+(pass->stencil?704:0);unsigned char color[1280],depth[1280],stencil[704];
    memoryless_expected(phase,color,depth,stencil);
    if(valid){for(unsigned i=0;i<1280;++i)mismatch+=pass->colorBytes[i]!=color[i];if(pass->depth)for(unsigned i=0;i<1280;++i)mismatch+=pass->depthBytes[i]!=depth[i];
        if(pass->stencil)for(unsigned i=0;i<704;++i)mismatch+=pass->stencilBytes[i]!=stencil[i];}
    if(valid && mismatch){
        attachment_page_snapshot(emit,"memoryless",phase,0,pass->colorBytes,1280,pass->color.gpuAddress,sequence);
        if(pass->depth)attachment_page_snapshot(emit,"memoryless",phase,8,pass->depthBytes,1280,pass->depth.gpuAddress,sequence);
        if(pass->stencil)attachment_page_snapshot(emit,"memoryless",phase,9,pass->stencilBytes,704,pass->stencil.gpuAddress,sequence);
    }
    valid=valid && !mismatch;
    emit("SWEET MEMORYLESS PHASE index=%u valid=%u sequence=%u operations=%u checked_bytes=%u mismatches=%u original_pass_pages=1",
        phase,valid,sequence,operations,checked,mismatch);
    emit("SWEET MEMORYLESS HASH index=%u color=%016llx depth=%016llx stencil=%016llx",
        phase,(unsigned long long)(valid?memoryless_hash(pass->colorBytes,1280):0),(unsigned long long)(valid && pass->depth?memoryless_hash(pass->depthBytes,1280):0),
        (unsigned long long)(valid && pass->stencil?memoryless_hash(pass->stencilBytes,704):0));
    if(valid)*previous=sequence;return valid;
}
static void memoryless_probe_release(MemorylessProbePass *pass)
{[pass->commands release];[pass->color release];[pass->depth release];[pass->stencil release];memset(pass,0,sizeof(*pass));}
static bool metal_memoryless_probe(MetalABIEmit emit)
{
    emit("SWEET MEMORYLESS BEGIN phases=8 width=17 height=9 color_bytes=1280 depth_bytes=1280 stencil_bytes=704 public_CPU_backing=0");
    unsigned char digest[32];CC_SHA256(sweet_air_render_fixture,(CC_LONG)sizeof(sweet_air_render_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET MEMORYLESS INPUT fixture_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.width=17;d.height=9;d.usage=MTLTextureUsageRenderTarget;d.storageMode=MTLStorageModeMemoryless;
    d.pixelFormat=MTLPixelFormatDepth32Float;id<MTLTexture> depth=[device newTextureWithDescriptor:d];d.pixelFormat=MTLPixelFormatStencil8;id<MTLTexture> stencil=[device newTextureWithDescriptor:d];
    d.pixelFormat=MTLPixelFormatBGRA8Unorm;id<MTLTexture> memoryColor=[device newTextureWithDescriptor:d];[d release];
    bool valid=device && depth && stencil && memoryColor && !device.currentAllocatedSize && !depth.buffer && !stencil.buffer && !memoryColor.buffer &&
        depth.storageMode==MTLStorageModeMemoryless && depth.framebufferOnly;
    emit("SWEET MEMORYLESS PUBLIC valid=%u allocated=%lu depth=%016llx stencil=%016llx color=%016llx backing_exposed=0",
        valid,(unsigned long)device.currentAllocatedSize,(unsigned long long)(uintptr_t)depth,(unsigned long long)(uintptr_t)stencil,(unsigned long long)(uintptr_t)memoryColor);
    id<MTLBuffer> cb=[device newBufferWithLength:1280 options:0],cb2=[device newBufferWithLength:1280 options:0];d=[MTLTextureDescriptor new];d.width=17;d.height=9;
    d.usage=MTLTextureUsageRenderTarget;d.pixelFormat=MTLPixelFormatBGRA8Unorm;d.storageMode=MTLStorageModeShared;
    id<MTLTexture> color=[cb newTextureWithDescriptor:d offset:64 bytesPerRow:128],color2=[cb2 newTextureWithDescriptor:d offset:64 bytesPerRow:128];[d release];
    const float data[12]={-1,1,0.25f,1,3,1,0.25f,1,-1,-3,0.25f,1};id<MTLBuffer> positions=[device newBufferWithBytes:data length:sizeof(data) options:0];
    id<MTLCommandQueue> queue=[device newCommandQueue],queue2=[device newCommandQueue];
    dispatch_data_t source=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:source error:&error];dispatch_release(source);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    id<MTLRenderPipelineState> pipelines[4]={nil,nil,nil,nil};for(unsigned i=0;i<4;++i){MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;
        pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;pd.depthAttachmentPixelFormat=i==1 || i==3?MTLPixelFormatDepth32Float:MTLPixelFormatInvalid;
        pd.stencilAttachmentPixelFormat=i>=2?MTLPixelFormatStencil8:MTLPixelFormatInvalid;pipelines[i]=[device newRenderPipelineStateWithDescriptor:pd error:&error];[pd release];valid=valid && pipelines[i];}
    valid=valid && color && color2 && positions && queue && queue2 && library;unsigned completed=0;uint32_t previous=0;
    for(unsigned phase=0;phase<6 && valid;++phase){@autoreleasepool{MemorylessProbePass pass={0};unsigned index=phase<2?1:phase==2?2:phase==3?3:0;
        valid=memoryless_probe_pages(&pass,phase,device,queue,pipelines[index],phase>=4?memoryColor:color,phase<2 || phase==3?depth:nil,phase==2 || phase==3?stencil:nil,positions,emit);
        if(valid){[pass.commands commit];[pass.commands waitUntilCompleted];valid=memoryless_probe_check(&pass,phase,&previous,emit);}if(valid)++completed;memoryless_probe_release(&pass);}}
    if(valid){@autoreleasepool{MemorylessProbePass a={0},b={0};
        id<MTLBuffer> other=[device newBufferWithBytes:data length:sizeof(data) options:0];
        valid=other && memoryless_probe_pages(&a,6,device,queue,pipelines[3],color,depth,stencil,positions,emit) &&
            memoryless_probe_pages(&b,7,device,queue2,pipelines[3],color2,depth,stencil,other,emit);
        valid=valid && a.depthInfo.handle!=b.depthInfo.handle && a.depthInfo.gpu_address!=b.depthInfo.gpu_address &&
            a.stencilInfo.handle!=b.stencilInfo.handle && a.stencilInfo.gpu_address!=b.stencilInfo.gpu_address && a.depthInfo.pass_id!=b.depthInfo.pass_id;
        emit("SWEET MEMORYLESS ISOLATION valid=%u two_queues=1 encoders_materialized_before_commit=1 different_depth_and_stencil_pages=1",valid);
        if(valid){[a.commands commit];[b.commands commit];[a.commands waitUntilCompleted];[b.commands waitUntilCompleted];
            // Separate queues may finish out of order; each owns a valid fence.
            uint32_t own=0;valid=memoryless_probe_check(&a,6,&own,emit);if(valid)++completed;own=0;
            valid=memoryless_probe_check(&b,7,&own,emit) && valid;if(valid)++completed;}
        memoryless_probe_release(&a);memoryless_probe_release(&b);[other release];}}
    for(unsigned i=0;i<4;++i)[pipelines[i] release];[vertex release];[fragment release];[library release];[positions release];[color release];[color2 release];
    [cb release];[cb2 release];[queue release];[queue2 release];[depth release];[stencil release];[memoryColor release];
    NSUInteger final=device.currentAllocatedSize;valid=valid && completed==8 && !final;
    emit("SWEET MEMORYLESS END result=%u phases=%u checked_bytes=19456 guard_bytes=10888 final_allocated=%lu GPU_only=1 transient_pages=1",valid?0:1,completed,(unsigned long)final);
    [device release];return valid;
}
