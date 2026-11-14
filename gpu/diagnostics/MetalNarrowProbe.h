/* Original-page compute results for packed dynamic scalar/vector conversions. */
#pragma once
#include "NarrowProbeValues.h"
static bool metal_narrow_probe(MetalABIEmit emit)
{
    emit("SWEET AIR NARROW begin bits=8,16 signs=signed,unsigned lanes=1,2 phases=2 invocations=64 original_buffers=3");
    unsigned char digest[32];CC_SHA256(sweet_air_narrow_fixture,(CC_LONG)sizeof(sweet_air_narrow_fixture),digest);char hex[65]={0};for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);
    emit("SWEET AIR NARROW input library_sha256=%s",hex);NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_narrow_fixture,sizeof(sweet_air_narrow_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);id<MTLFunction> function=[library newFunctionWithName:@"narrow_convert"];
    id<MTLComputePipelineState> pipeline=[device newComputePipelineStateWithFunction:function error:&error];id<MTLCommandQueue> queue=[device newCommandQueue];
    id<MTLBuffer> buffers[3]={[device newBufferWithLength:3200 options:0],[device newBufferWithLength:384 options:0],[device newBufferWithLength:256 options:0]};unsigned char *original[3]={buffers[0].contents,buffers[1].contents,buffers[2].contents};
    bool valid=device && library && function && pipeline && queue && original[0] && original[1] && original[2];uint32_t previous=0;
    for(unsigned b=0;b<3 && valid;++b)valid=buffers[b].allocatedSize==16384;
    for(unsigned phase=0;phase<2 && valid;++phase){memset(original[0],0xa5,16384);memset(original[1],0xb6,16384);memset(original[2],0xc7,16384);narrow_source(original[1]+64,original[2]+64,phase);
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];valid=valid && commands && encoder;
        if(valid){[encoder setComputePipelineState:pipeline];for(unsigned b=0;b<3;++b)[encoder setBuffer:buffers[b] offset:64 atIndex:b];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 && !commands.error && commands.status==MTLCommandBufferStatusCompleted;previous=sequence;
        for(unsigned b=0;b<3 && valid;++b)valid=buffers[b].contents==original[b];unsigned mismatch=valid?narrow_check(original[0],original[1],original[2],phase):0;valid=valid && !mismatch;
        emit("SWEET AIR NARROW phase=%u sequence=%u operations=%u status=%lu error=%u checked_bytes=49152 mismatches=%u valid=%u",phase,sequence,operations,(unsigned long)commands.status,commands.error!=nil,mismatch,valid);
        for(unsigned b=0;b<3;++b)emit("SWEET AIR NARROW buffer phase=%u index=%u bytes=16384 hash=%016llx original_pages=%u",phase,b,(unsigned long long)(valid?narrow_hash(original[b],16384):0),buffers[b].contents==original[b]);
        if(!valid && commands.error)emit("SWEET AIR NARROW failure reason=%s",commands.error.localizedDescription.UTF8String);[commands release];
    }
    [pipeline release];[function release];[library release];[queue release];for(unsigned b=0;b<3;++b)[buffers[b] release];NSUInteger final=device.currentAllocatedSize;valid=valid && !final;
    emit("SWEET AIR NARROW END result=%u phases=2 checked_bytes=98304 final_allocated=%lu GPU_only=1",valid?0:1,(unsigned long)final);[device release];return valid;
}
