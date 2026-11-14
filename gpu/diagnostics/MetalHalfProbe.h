/* GPU binary16 arithmetic and vertex/fragment results on original backing pages. */
#pragma once
#include "HalfProbeValues.h"
#include <CommonCrypto/CommonDigest.h>
static bool metal_half_probe(MetalABIEmit emit)
{
    emit("SWEET AIR HALF begin phases=2 invocations=64 raw16_storage=1 result_rounding=1 graphics=1 original_buffers=1");
    unsigned char digest[32];CC_SHA256(sweet_air_half_fixture,(CC_LONG)sizeof(sweet_air_half_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);emit("SWEET AIR HALF input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);dispatch_data_t data=dispatch_data_create(sweet_air_half_fixture,sizeof(sweet_air_half_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);id<MTLFunction> function=[library newFunctionWithName:@"half_arithmetic"];
    id<MTLComputePipelineState> compute=[device newComputePipelineStateWithFunction:function error:&error];id<MTLCommandQueue> queue=[device newCommandQueue];
    id<MTLBuffer> buffers[3]={[device newBufferWithLength:1664 options:0],[device newBufferWithLength:640 options:0],[device newBufferWithLength:384 options:0]};
    unsigned char *original[3]={buffers[0].contents,buffers[1].contents,buffers[2].contents};bool valid=device && library && function && compute && queue && original[0] && original[1] && original[2];uint32_t previous=0;
    for(unsigned b=0;b<3 && valid;++b)valid=buffers[b].allocatedSize==16384;
    for(unsigned phase=0;phase<2 && valid;++phase){memset(original[0],0xa5,16384);memset(original[1],0xb6,16384);memset(original[2],0xc7,16384);half_probe_sources(original[1]+64,original[2]+64,phase);
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];valid=valid && commands && encoder;
        if(valid){[encoder setComputePipelineState:compute];for(unsigned b=0;b<3;++b)[encoder setBuffer:buffers[b] offset:64 atIndex:b];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 && !commands.error && commands.status==MTLCommandBufferStatusCompleted;previous=sequence;
        for(unsigned b=0;b<3 && valid;++b)valid=buffers[b].contents==original[b];unsigned mismatch=valid?half_probe_check(original[0],original[1],original[2],phase):0;valid=valid && !mismatch;
        emit("SWEET AIR HALF phase=%u sequence=%u operations=%u status=%lu error=%u checked_bytes=49152 mismatches=%u valid=%u",phase,sequence,operations,(unsigned long)commands.status,commands.error!=nil,mismatch,valid);
        unsigned nans=0;uint64_t normalized=valid?half_probe_normalized_hash(original[0],&nans):0;
        emit("SWEET AIR HALF normalized phase=%u output_hash=%016llx nan_words=%u finite_exact=1 NaN_class=1",phase,(unsigned long long)normalized,nans);
        if(valid && nans)for(unsigned word=0;word<768;++word){uint16_t actual;memcpy(&actual,original[0]+64+word*2,2);
            if((actual&31744)==31744 && (actual&1023))emit("SWEET AIR HALF NaN phase=%u word=%u bits=%04x",phase,word,actual);}
        for(unsigned b=0;b<3;++b)emit("SWEET AIR HALF buffer phase=%u index=%u raw_hash=%016llx original_pages=%u",phase,b,(unsigned long long)(valid?half_probe_hash(original[b],16384):0),buffers[b].contents==original[b]);
        if(!valid && commands.error)emit("SWEET AIR HALF failure phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);[commands release];
    }
    id<MTLFunction> vertex=[library newFunctionWithName:@"half_position"],fragment=[library newFunctionWithName:@"half_colour"];
    MTLRenderPipelineDescriptor *p=[MTLRenderPipelineDescriptor new];p.vertexFunction=vertex;p.fragmentFunction=fragment;p.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> graphics=[device newRenderPipelineStateWithDescriptor:p error:&error];[p release];
    id<MTLBuffer> positions=[device newBufferWithLength:152 options:0],output=[device newBufferWithLength:1344 options:0];unsigned char *rawPositions=positions.contents,*rawOutput=output.contents;
    MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=17;d.height=9;d.pixelFormat=MTLPixelFormatBGRA8Unorm;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;
    id<MTLTexture> target=[output newTextureWithDescriptor:d offset:128 bytesPerRow:128];[d release];valid=valid && graphics && target && rawPositions && rawOutput;
    const uint16_t coords[12]={0xbc00,0x3c00,0x3800,0x3c00,0x4200,0x3c00,0x3800,0x3c00,0xbc00,0xc200,0x3800,0x3c00};
    if(valid){memset(rawPositions,0xd4,16384);memcpy(rawPositions+64,coords,24);memset(rawOutput,0xe7,16384);
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=commands && encoder;
        if(valid){[encoder setRenderPipelineState:graphics];[encoder setVertexBuffer:positions offset:64 atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 && !commands.error && output.contents==rawOutput && positions.contents==rawPositions;
        unsigned mismatch=0;if(valid)for(unsigned i=0;i<16384;++i){unsigned char wanted=0xe7;
            if(i>=128 && i<128+1152){unsigned column=(i-128)%128;if(column<68){const unsigned char color[4]={191,128,64,255};wanted=color[column%4];}}
            mismatch+=rawOutput[i]!=wanted;unsigned char unchanged=i>=64 && i<88?((const unsigned char *)coords)[i-64]:0xd4;mismatch+=rawPositions[i]!=unchanged;}
        valid=valid && !mismatch;emit("SWEET AIR HALF graphics sequence=%u operations=%u status=%lu error=%u checked_bytes=32768 pixels=153 mismatches=%u valid=%u",sequence,operations,(unsigned long)commands.status,commands.error!=nil,mismatch,valid);
        emit("SWEET AIR HALF graphics_hash target=%016llx vertices=%016llx original_pages=1",(unsigned long long)(valid?half_probe_hash(rawOutput,16384):0),(unsigned long long)(valid?half_probe_hash(rawPositions,16384):0));
        if(!valid && commands.error)emit("SWEET AIR HALF graphics_failure reason=%s",commands.error.localizedDescription.UTF8String);[commands release];}
    [compute release];[function release];[graphics release];[vertex release];[fragment release];[target release];[positions release];[output release];[queue release];[library release];
    for(unsigned b=0;b<3;++b)[buffers[b] release];NSUInteger final=device.currentAllocatedSize;valid=valid && !final;
    emit("SWEET AIR HALF END result=%u checked_bytes=131072 phases=2 graphics=1 final_allocated=%lu GPU_only=1",valid?0:1,(unsigned long)final);[device release];return valid;
}
