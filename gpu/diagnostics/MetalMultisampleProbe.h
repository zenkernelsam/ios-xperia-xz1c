/* SPDX-License-Identifier: BSD-2-Clause; actual original FD540 sample pages. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
#include "AttachmentPageSnapshot.h"
#include "MultisampleProbeValues.h"
static const unsigned multisample_slots[5]={0,8,9,10,11};
typedef struct MultisampleProbeSet {
    id<MTLTexture> textures[5];id<MTLBuffer> buffers[5];
    SweetMTLMultisampleTextureInfo info[5];uint64_t pass[5];unsigned char *bytes[5];
    id<MTLCommandBuffer> commands;
} MultisampleProbeSet;
static void multisample_probe_release_set(MultisampleProbeSet *set)
{
    [set->commands release];for(unsigned i=0;i<5;++i){[set->textures[i] release];[set->buffers[i] release];}memset(set,0,sizeof(*set));
}
static bool multisample_probe_new_set(MultisampleProbeSet *set,id<MTLDevice> device,unsigned samples,bool transient,MultisampleProbeSet *shared)
{
    MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.width=17;d.height=9;d.usage=MTLTextureUsageRenderTarget;
    d.textureType=MTLTextureType2DMultisample;d.sampleCount=samples;d.storageMode=transient?MTLStorageModeMemoryless:MTLStorageModePrivate;
    for(unsigned i=0;i<3;++i){d.pixelFormat=i==0?MTLPixelFormatBGRA8Unorm:i==1?MTLPixelFormatDepth32Float:MTLPixelFormatStencil8;
        set->textures[i]=shared?[shared->textures[i] retain]:[device newTextureWithDescriptor:d];
        if(!transient)set->buffers[i]=SweetMTLRetainMultisampleTextureBuffer(set->textures[i],&set->info[i]);}
    d.sampleCount=1;d.textureType=MTLTextureType2D;d.storageMode=MTLStorageModeShared;
    for(unsigned i=3;i<5;++i){d.pixelFormat=i==3?MTLPixelFormatBGRA8Unorm:MTLPixelFormatDepth32Float;
        id<MTLBuffer> buffer=[device newBufferWithLength:1280 options:0];set->textures[i]=[buffer newTextureWithDescriptor:d offset:64 bytesPerRow:128];[buffer release];
        set->buffers[i]=SweetMTLRetainResolveTextureBuffer(set->textures[i],&set->info[i]);}
    [d release];bool valid=true;
    for(unsigned i=0;i<5;++i){valid=valid && set->textures[i] && set->textures[i].sampleCount==(i<3?samples:1);
        if(i<3 && transient){valid=valid && !set->textures[i].buffer && !set->textures[i].allocatedSize;continue;}
        set->bytes[i]=(unsigned char *)set->buffers[i].contents;
        valid=valid && set->buffers[i] && set->bytes[i] && set->buffers[i].length==multisample_probe_bytes(samples,multisample_slots[i]);
        if(set->bytes[i])memset(set->bytes[i],i==0?0xa5:i==1?0xb6:i==2?0xc7:i==3?0xd8:0xe9,set->buffers[i].length);}
    return valid;
}
static bool multisample_probe_record_pages(MultisampleProbeSet *set,unsigned phase,id<MTLRenderCommandEncoder> encoder,MetalABIEmit emit)
{
    bool valid=true;unsigned samples=multisample_probe_samples(phase);
    for(unsigned i=0;i<5;++i){unsigned slot=multisample_slots[i];
        if(phase>=14 && i<3){SweetMTLMemorylessPassInfo info={};set->buffers[i]=SweetMTLRetainMemorylessPassBuffer(encoder,slot,&info);
            set->pass[i]=info.pass_id;set->info[i]=(SweetMTLMultisampleTextureInfo){info.public_texture,info.gpu_address,info.buffer_bytes,info.offset,
                info.handle,info.width,info.height,info.pitch,info.format,samples,{info.reserved,0}};
            set->bytes[i]=(unsigned char *)set->buffers[i].contents;
            if(set->bytes[i])memset(set->bytes[i],i==0?0xa5:i==1?0xb6:0xc7,set->buffers[i].length);}
        SweetMTLMultisampleTextureInfo info=set->info[i];
        valid=valid && info.public_texture==(uint64_t)(uintptr_t)set->textures[i] && info.handle && info.gpu_address>=UINT64_C(0x40000000) && info.gpu_address<UINT64_C(0x80000000) &&
            !(info.gpu_address&16383) && info.buffer_bytes==multisample_probe_bytes(samples,slot) && info.width==17 && info.height==9 && info.offset==64 &&
            info.pitch==multisample_probe_pitch(samples,slot) && info.format==(i==0 || i==3?80:i==2?253:252) && info.samples==(i<3?samples:1) &&
            !info.reserved[0] && !info.reserved[1] && set->bytes[i] && (phase<14 || i>=3 || set->pass[i]);
        emit("SWEET MULTISAMPLE PAGE phase=%u slot=%u public=%016llx pass=%llu handle=%u GPU=0x%llx bytes=%llu pitch=%u offset=%llu format=%u samples=%u",
            phase,slot,(unsigned long long)info.public_texture,(unsigned long long)set->pass[i],info.handle,(unsigned long long)info.gpu_address,
            (unsigned long long)info.buffer_bytes,info.pitch,(unsigned long long)info.offset,info.format,info.samples);
    }return valid;
}
static bool multisample_probe_draw(id<MTLRenderCommandEncoder> encoder,id<MTLDevice> device,id<MTLRenderPipelineState> pipeline,
    unsigned sample,unsigned state,bool allSamples)
{
    unsigned char rgba[4];multisample_probe_rgba(sample,state,rgba);float z=multisample_probe_depth(sample,state);
    const float positions[12]={-1,1,z,1,3,1,z,1,-1,-3,z,1};
    float color[4];for(unsigned i=0;i<4;++i)color[i]=rgba[i]/255.0f;
    id<MTLBuffer> vertices=[device newBufferWithBytes:positions length:sizeof(positions) options:0],constant=[device newBufferWithBytes:color length:sizeof(color) options:0];
    MTLDepthStencilDescriptor *d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=MTLCompareFunctionAlways;d.depthWriteEnabled=YES;
    d.frontFaceStencil.stencilCompareFunction=MTLCompareFunctionAlways;d.frontFaceStencil.depthStencilPassOperation=MTLStencilOperationReplace;
    id<MTLDepthStencilState> depth=[device newDepthStencilStateWithDescriptor:d];[d release];
    bool valid=vertices && constant && depth && (allSamples || SweetMTLSetDiagnosticSampleMask(encoder,1u<<sample));
    if(valid){[encoder setRenderPipelineState:pipeline];[encoder setDepthStencilState:depth];[encoder setStencilReferenceValue:multisample_probe_stencil(sample,state)];
        [encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder setFragmentBuffer:constant offset:0 atIndex:0];
        [encoder setScissorRect:(MTLScissorRect){3,2,11,5}];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}
    // The queue must retain each distinct original input after caller release.
    [vertices release];[constant release];[depth release];return valid;
}
static bool multisample_probe_encode(MultisampleProbeSet *set,unsigned phase,id<MTLDevice> device,id<MTLCommandQueue> queue,id<MTLRenderPipelineState> pipeline,MetalABIEmit emit)
{
    unsigned samples=multisample_probe_samples(phase),local=phase%7;bool transient=phase>=14,clear=transient || local==0 || local==1 || local==6;
    bool resolve=transient || local>=3;MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];
    pass.colorAttachments[0].texture=set->textures[0];pass.colorAttachments[0].loadAction=clear?MTLLoadActionClear:MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction=resolve?transient?MTLStoreActionMultisampleResolve:MTLStoreActionStoreAndMultisampleResolve:MTLStoreActionStore;
    pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,1,1);pass.colorAttachments[0].resolveTexture=resolve?set->textures[3]:nil;
    pass.depthAttachment.texture=set->textures[1];pass.depthAttachment.loadAction=clear?MTLLoadActionClear:MTLLoadActionLoad;
    pass.depthAttachment.storeAction=pass.colorAttachments[0].storeAction;pass.depthAttachment.clearDepth=0.75;
    pass.depthAttachment.resolveTexture=resolve?set->textures[4]:nil;pass.depthAttachment.depthResolveFilter=(MTLMultisampleDepthResolveFilter)multisample_probe_filter(phase);
    pass.stencilAttachment.texture=set->textures[2];pass.stencilAttachment.loadAction=clear?MTLLoadActionClear:MTLLoadActionLoad;
    pass.stencilAttachment.storeAction=transient?MTLStoreActionDontCare:MTLStoreActionStore;pass.stencilAttachment.clearStencil=17;
    [set->commands release];set->commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[set->commands renderCommandEncoderWithDescriptor:pass];[pass release];
    bool valid=encoder && multisample_probe_record_pages(set,phase,encoder,emit);
    if(valid && (transient || local==1)){for(unsigned sample=0;sample<samples && valid;++sample)valid=multisample_probe_draw(encoder,device,pipeline,sample,1,false);}
    if(valid && (local==2 && !transient || transient && (phase&1)))valid=multisample_probe_draw(encoder,device,pipeline,0,2,false);
    if(valid && local==6 && !transient)valid=multisample_probe_draw(encoder,device,pipeline,0,3,true);
    if(encoder)[encoder endEncoding];return valid;
}
static bool multisample_probe_check(MultisampleProbeSet *set,unsigned phase,uint32_t *previous,MetalABIEmit emit)
{
    uint32_t sequence=0,operations=0;unsigned samples=multisample_probe_samples(phase),checked=0,mismatches=0,guards=0;unsigned char expected[3008];uint64_t hashes[5]={0};
    bool valid=set->commands.status==MTLCommandBufferStatusCompleted && !set->commands.error && SweetMTLCommandResult(set->commands,&sequence,&operations) &&
        sequence>*previous && operations==multisample_probe_operations(phase);
    for(unsigned i=0;i<5;++i){unsigned slot=multisample_slots[i],size=multisample_probe_bytes(samples,slot);checked+=size;guards+=size-17*9*(slot==9?1:4)*(i<3?samples:1);
        valid=valid && set->buffers[i].contents==set->bytes[i] && set->bytes[i];
        multisample_probe_expected(phase,slot,expected);
        if(set->bytes[i]){for(unsigned byte=0;byte<size;++byte)mismatches+=set->bytes[i][byte]!=expected[byte];hashes[i]=multisample_probe_hash(set->bytes[i],size);}}
    if(valid && mismatches)for(unsigned i=0;i<5;++i)attachment_page_snapshot(emit,"msaa",phase,multisample_slots[i],set->bytes[i],
        multisample_probe_bytes(samples,multisample_slots[i]),set->buffers[i].gpuAddress,sequence);
    valid=valid && !mismatches;
    emit("SWEET MULTISAMPLE PHASE index=%u valid=%u sequence=%u operations=%u checked_bytes=%u guard_bytes=%u mismatches=%u original_pages=1",
        phase,valid,sequence,operations,checked,guards,mismatches);
    emit("SWEET MULTISAMPLE SOURCE index=%u color=%016llx depth=%016llx stencil=%016llx",phase,(unsigned long long)hashes[0],(unsigned long long)hashes[1],(unsigned long long)hashes[2]);
    emit("SWEET MULTISAMPLE RESOLVED index=%u color=%016llx depth=%016llx",phase,(unsigned long long)hashes[3],(unsigned long long)hashes[4]);
    if(valid)*previous=sequence;return valid;
}
static id<MTLRenderPipelineState> multisample_probe_pipeline(id<MTLDevice> device,id<MTLLibrary> library,unsigned samples,NSError **error)
{
    id<MTLFunction> vertex=[library newFunctionWithName:@"multisample_position"],fragment=[library newFunctionWithName:@"multisample_color"];
    MTLRenderPipelineDescriptor *d=[MTLRenderPipelineDescriptor new];d.vertexFunction=vertex;d.fragmentFunction=fragment;d.rasterSampleCount=samples;
    d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;d.stencilAttachmentPixelFormat=MTLPixelFormatStencil8;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:d error:error];[d release];[vertex release];[fragment release];return pipeline;
}
static bool metal_multisample_probe(MetalABIEmit emit)
{
    emit("SWEET MULTISAMPLE BEGIN phases=18 width=17 height=9 samples=2,4 source_samples=original resolve=GPU_shader");
    unsigned char digest[32];CC_SHA256(sweet_air_multisample_fixture,(CC_LONG)sizeof(sweet_air_multisample_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET MULTISAMPLE INPUT fixture_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);dispatch_data_t source=dispatch_data_create(sweet_air_multisample_fixture,sizeof(sweet_air_multisample_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:source error:&error];dispatch_release(source);id<MTLCommandQueue> queue=[device newCommandQueue],other=[device newCommandQueue];
    bool valid=device && library && queue && other;unsigned completed=0;uint32_t previous=0;
    for(unsigned group=0;group<2 && valid;++group){@autoreleasepool{unsigned samples=group?4:2;MultisampleProbeSet set={0};
        id<MTLRenderPipelineState> pipeline=multisample_probe_pipeline(device,library,samples,&error);valid=pipeline && multisample_probe_new_set(&set,device,samples,false,NULL);
        for(unsigned local=0;local<7 && valid;++local){unsigned phase=group*7+local;valid=multisample_probe_encode(&set,phase,device,queue,pipeline,emit);
            if(valid){[set.commands commit];[set.commands waitUntilCompleted];valid=multisample_probe_check(&set,phase,&previous,emit);}if(valid)++completed;}
        multisample_probe_release_set(&set);[pipeline release];}}
    for(unsigned group=0;group<2 && valid;++group){@autoreleasepool{unsigned samples=group?4:2,phase=14+group*2;MultisampleProbeSet a={0},b={0};
        id<MTLRenderPipelineState> pipeline=multisample_probe_pipeline(device,library,samples,&error);
        valid=pipeline && multisample_probe_new_set(&a,device,samples,true,NULL) && multisample_probe_new_set(&b,device,samples,true,&a) &&
            multisample_probe_encode(&a,phase,device,queue,pipeline,emit) && multisample_probe_encode(&b,phase+1,device,other,pipeline,emit);
        for(unsigned i=0;i<3 && valid;++i)valid=a.info[i].public_texture==b.info[i].public_texture && a.info[i].handle!=b.info[i].handle &&
            a.info[i].gpu_address!=b.info[i].gpu_address && a.pass[i] && b.pass[i] && a.pass[i]!=b.pass[i];
        emit("SWEET MULTISAMPLE ISOLATION samples=%u valid=%u two_queues=1 original_public_shared=1 private_pages_differ=1 encoders_before_commit=1",samples,valid);
        if(valid){[a.commands commit];[b.commands commit];[a.commands waitUntilCompleted];[b.commands waitUntilCompleted];
            uint32_t own=previous;valid=multisample_probe_check(&a,phase,&own,emit);if(valid)++completed;
            own=previous;bool second=multisample_probe_check(&b,phase+1,&own,emit);if(second)++completed;valid=second && valid;}
        multisample_probe_release_set(&a);multisample_probe_release_set(&b);[pipeline release];}}
    [queue release];[other release];[library release];NSUInteger final=device.currentAllocatedSize;valid=valid && completed==18 && !final;
    emit("SWEET MULTISAMPLE END result=%u phases=%u checked_bytes=151488 guard_bytes=55098 final_allocated=%lu GPU_only=1 original_samples=1",valid?0:1,completed,(unsigned long)final);
    [device release];return valid;
}
