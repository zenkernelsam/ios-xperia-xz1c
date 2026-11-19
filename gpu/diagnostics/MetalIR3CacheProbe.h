/* Actual fresh compute pipelines must load the exact compiled hardware variant. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
static bool metal_ir3_cache_probe(MetalABIEmit emit)
{
    emit("SWEET IR3 CACHE PROBE begin phases=3 steps=13,29,13 original_pages=1 fresh_pipelines=1");
    unsigned char digest[32];CC_SHA256(sweet_air_constants_fixture,(CC_LONG)sizeof(sweet_air_constants_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);emit("SWEET IR3 CACHE PROBE input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);id<MTLCommandQueue> queue=[device newCommandQueue];
    dispatch_data_t data=dispatch_data_create(sweet_air_constants_fixture,sizeof(sweet_air_constants_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLBuffer> output=[device newBufferWithLength:320 options:0];uint32_t *original=(uint32_t *)output.contents;
    MTLFunctionConstantValues *values=[MTLFunctionConstantValues new];bool valid=device && queue && library && output && original && values;
    const uint32_t steps[3]={13,29,13};uint32_t previous=0,total=0;unsigned passed=0;
    for(unsigned phase=0;phase<3 && valid;++phase){@autoreleasepool{
        uint64_t before[6],after[6];sweetgl_ir3_cache_stats(before);[values reset];uint32_t step=steps[phase];[values setConstantValue:&step type:MTLDataTypeUInt atIndex:7];
        id<MTLFunction> function=[library newFunctionWithName:@"add_constant" constantValues:values error:&error];
        id<MTLComputePipelineState> pipeline=function?[device newComputePipelineStateWithFunction:function error:&error]:nil;
        if(!phase)for(unsigned i=0;i<80;++i)original[i]=(i*0x9e3779b1u)^0x434f4e53u;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];
        valid=function && pipeline && commands && encoder;if(valid){[encoder setComputePipelineState:pipeline];[encoder setBuffer:output offset:0 atIndex:0];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && output.contents==original;previous=sequence;total+=step;
        unsigned mismatches=0;uint64_t hash=UINT64_C(14695981039346656037);
        if(valid){for(unsigned i=0;i<80;++i){uint32_t wanted=(i*0x9e3779b1u)^0x434f4e53u;if(i<64)wanted+=total;mismatches+=original[i]!=wanted;}
            const unsigned char *bytes=(const unsigned char *)original;for(unsigned i=0;i<320;++i)hash=(hash^bytes[i])*UINT64_C(1099511628211);}
        sweetgl_ir3_cache_stats(after);valid=valid && !mismatches && after[3]==before[3] && (phase!=2 || after[1]>before[1]);
        emit("SWEET IR3 CACHE PROBE phase=%u step=%u total=%u sequence=%u operations=%u checked_bytes=320 mismatches=%u hash=%016llx valid=%u",
            phase,step,total,sequence,operations,mismatches,(unsigned long long)hash,valid);
        emit("SWEET IR3 CACHE PROBE stats phase=%u hits=%llu stores=%llu rejected=%llu bytes_read=%llu exact_variant=1",
            phase,(unsigned long long)(after[1]-before[1]),(unsigned long long)(after[2]-before[2]),(unsigned long long)(after[3]-before[3]),(unsigned long long)(after[4]-before[4]));
        if(valid)++passed;[commands release];[pipeline release];[function release];
    }}
    [values release];[output release];[library release];[queue release];NSUInteger final=device.currentAllocatedSize;valid=valid && passed==3 && !final;
    emit("SWEET IR3 CACHE PROBE END result=%u phases=%u checked_bytes=960 final_allocated=%lu cold_and_restore=1 original_pages=1",valid?0:1,passed,(unsigned long)final);
    [device release];return valid;
}
