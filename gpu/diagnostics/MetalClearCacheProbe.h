/* Repeated Clear on eight original padded attachments with independent colours. */
#pragma once
#include "ClearCacheProbeValues.h"
#include <CommonCrypto/CommonDigest.h>
static bool metal_clear_cache_probe(MetalABIEmit emit)
{
    emit("SWEET CLEAR CACHE begin phases=2 targets=8 width=17 height=9 pitch=128 offset=64 original_pages=1");
    unsigned char digest[32];CC_SHA256(sweet_air_mrt_fixture,(CC_LONG)sizeof(sweet_air_mrt_fixture),digest);
    char hex[65]={0};for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);
    emit("SWEET CLEAR CACHE input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_mrt_fixture,sizeof(sweet_air_mrt_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"mrt_position"],fragment=[library newFunctionWithName:@"mrt_eight"];
    const float positions[12]={-1,1,.5f,1,3,1,.5f,1,-1,-3,.5f,1};
    id<MTLBuffer> vertices=[device newBufferWithBytes:positions length:sizeof(positions) options:0];
    id<MTLCommandQueue> queue=[device newCommandQueue];MTLRenderPipelineDescriptor *descriptor=[MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction=vertex;descriptor.fragmentFunction=fragment;
    id<MTLBuffer> raw[8]={nil};id<MTLTexture> targets[8]={nil};unsigned char *original[8]={NULL};
    bool valid=device && library && vertex && fragment && vertices && queue;
    for(unsigned index=0;index<8;++index){unsigned format=index&1?70:80;
        descriptor.colorAttachments[index].pixelFormat=(MTLPixelFormat)format;
        raw[index]=[device newBufferWithLength:1280 options:0];
        MTLTextureDescriptor *texture=[MTLTextureDescriptor new];texture.width=17;texture.height=9;texture.pixelFormat=(MTLPixelFormat)format;
        texture.storageMode=MTLStorageModeShared;texture.usage=MTLTextureUsageRenderTarget;
        targets[index]=[raw[index] newTextureWithDescriptor:texture offset:64 bytesPerRow:128];[texture release];
        original[index]=raw[index].contents;valid=valid && raw[index] && targets[index] && original[index] && raw[index].allocatedSize==16384;
        if(original[index])memset(original[index],0xa0+index,16384);
    }
    id<MTLRenderPipelineState> pipeline=valid?[device newRenderPipelineStateWithDescriptor:descriptor error:&error]:nil;
    [descriptor release];valid=valid && pipeline;uint32_t previous=0;unsigned passed=0;
    for(unsigned phase=0;phase<2 && valid;++phase){@autoreleasepool{
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];
        for(unsigned index=0;index<8;++index){MTLRenderPassColorAttachmentDescriptor *a=pass.colorAttachments[index];a.texture=targets[index];
            a.loadAction=MTLLoadActionClear;a.storeAction=MTLStoreActionStore;
            a.clearColor=MTLClearColorMake((phase^(index&1))?1:0,phase?0:1,phase?1:0,1);}
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];
        valid=commands && encoder;if(valid){[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];
            [encoder setScissorRect:(MTLScissorRect){3,2,11,5}];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error;previous=sequence;unsigned mismatches=0;
        for(unsigned index=0;index<8 && valid;++index){valid=raw[index].contents==original[index];uint64_t hash=UINT64_C(14695981039346656037);
            for(unsigned at=0;at<16384;++at){unsigned wanted=0xa0+index;if(at>=64 && at<64+128*9){unsigned y=(at-64)/128,x=(at-64)%128;
                    if(x<17*4)wanted=clear_cache_pixel_bytes(index,phase,x/4,y,x%4);}mismatches+=original[index][at]!=wanted;hash=(hash^original[index][at])*UINT64_C(1099511628211);}
            emit("SWEET CLEAR CACHE buffer phase=%u index=%u hash=%016llx original_pages=%u",phase,index,(unsigned long long)hash,raw[index].contents==original[index]);}
        valid=valid && !mismatches;emit("SWEET CLEAR CACHE phase=%u sequence=%u operations=%u checked_bytes=131072 mismatches=%u status=%lu error=%u valid=%u",
            phase,sequence,operations,mismatches,(unsigned long)commands.status,commands.error!=nil,valid);
        if(valid)++passed;[commands release];
    }}
    [pipeline release];[vertices release];[queue release];[vertex release];[fragment release];[library release];
    for(unsigned index=0;index<8;++index){[targets[index] release];[raw[index] release];}
    NSUInteger final=device.currentAllocatedSize;valid=valid && passed==2 && !final;
    emit("SWEET CLEAR CACHE END result=%u phases=%u targets=8 checked_bytes=262144 final_allocated=%lu original_clear=1 GPU_only=1",valid?0:1,passed,(unsigned long)final);
    [device release];return valid;
}
