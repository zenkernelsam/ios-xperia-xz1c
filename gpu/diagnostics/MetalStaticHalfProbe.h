/* Authored static half/i16 memory aliases through original Metal constants and GPU. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
static bool metal_static_half_probe(MetalABIEmit emit)
{
    const uint16_t patterns[12]={0x0000,0x8000,0x0001,0x03ff,0x0400,0x3c00,0x7bff,0x7c00,0xfc00,0x7e55,0x7d01,0xfe01};
    emit("SWEET AIR STATIC HALF begin cases=12 paths=4 original_constants=1 original_pages=1 GPU_only=1");
    unsigned char digest[32];CC_SHA256(sweet_air_static_half_fixture,(CC_LONG)sizeof(sweet_air_static_half_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET AIR STATIC HALF input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_static_half_fixture,sizeof(sweet_air_static_half_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLCommandQueue> queue=[device newCommandQueueWithMaxCommandBufferCount:8];id<MTLBuffer> buffer=[device newBufferWithLength:16384 options:0];
    unsigned char *original=(unsigned char *)buffer.contents;MTLFunctionConstantValues *values=[MTLFunctionConstantValues new];
    bool valid=device && library && queue && buffer && original && values && !error;uint32_t previous=0;
    for(unsigned n=0;n<12 && valid;++n){
        memset(original,0xa5,16384);uint16_t half=patterns[n],word=patterns[(n+1)%12];[values reset];
        [values setConstantValue:&half type:MTLDataTypeHalf atIndex:1];[values setConstantValue:&word type:MTLDataTypeUShort atIndex:2];
        error=nil;id<MTLFunction> function=[library newFunctionWithName:@"static_half" constantValues:values error:&error];
        id<MTLComputePipelineState> pipeline=function?[device newComputePipelineStateWithFunction:function error:&error]:nil;
        [values reset];uint16_t changed=0;[values setConstantValue:&changed type:MTLDataTypeHalf atIndex:1];[values setConstantValue:&changed type:MTLDataTypeUShort atIndex:2];
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];
        valid=valid && function && pipeline && commands && encoder && !error;
        if(valid){[encoder setComputePipelineState:pipeline];[encoder setBuffer:buffer offset:64 atIndex:0];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(1,1,1)];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && operations==1 && sequence>previous &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && buffer.contents==original;
        unsigned mismatches=0;uint64_t hash=UINT64_C(14695981039346656037);
        if(valid)for(unsigned i=0;i<16384;++i){unsigned char expected=0xa5;
            if(i>=64 && i<80){unsigned at=i-64,shift=(at%4)*8;uint32_t value=at/4<2?half:word;expected=(unsigned char)(value>>shift);}
            mismatches+=original[i]!=expected;hash=(hash^original[i])*UINT64_C(1099511628211);}
        valid=valid && !mismatches;previous=sequence;
        emit("SWEET AIR STATIC HALF case=%u half=%04x word=%04x status=%lu error=%u sequence=%u operations=%u checked_bytes=16384 mismatches=%u hash=%016llx frozen_after_reset=1 valid=%u",
            n,half,word,(unsigned long)commands.status,commands.error!=nil,sequence,operations,mismatches,(unsigned long long)hash,valid);
        if(valid){char bytes[33]={0};for(unsigned i=0;i<16;++i)snprintf(bytes+2*i,3,"%02x",original[64+i]);
            emit("SWEET AIR STATIC HALF run case=%u offset=0 bytes=64 pattern=a5",n);
            emit("SWEET AIR STATIC HALF data case=%u offset=64 bytes=16 hex=%s",n,bytes);
            emit("SWEET AIR STATIC HALF run case=%u offset=80 bytes=16304 pattern=a5",n);}
        if(!valid)emit("SWEET AIR STATIC HALF failure case=%u reason=%s",n,(error?:commands.error).localizedDescription.UTF8String?:"byte/fence/ownership mismatch");
        [commands release];[pipeline release];[function release];
    }
    [values release];[library release];[buffer release];[queue release];NSUInteger final=device?device.currentAllocatedSize:0;valid=valid && final==0;
    emit("SWEET AIR STATIC HALF END result=%u cases=12 paths=4 checked_bytes=196608 final_allocated=%lu original_constants=1 GPU_only=1",valid?0:1,(unsigned long)final);
    [device release];return valid;
}
