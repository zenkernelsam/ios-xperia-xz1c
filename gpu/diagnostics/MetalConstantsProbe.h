/* Original MTLFunctionConstantValues -> frozen AIR specialization -> FD540. */
#pragma once
#include <stdatomic.h>
#include <CommonCrypto/CommonDigest.h>
static bool metal_constants_probe(MetalABIEmit emit)
{
    emit("SWEET AIR CONSTANTS begin library_bytes=%lu standard_MTLB=1 native_constant_values=1 original_buffer=1",(unsigned long)sizeof(sweet_air_constants_fixture));
    unsigned char digest[32];CC_SHA256(sweet_air_constants_fixture,(CC_LONG)sizeof(sweet_air_constants_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET AIR CONSTANTS input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_constants_fixture,sizeof(sweet_air_constants_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    MTLFunctionConstantValues *values=[MTLFunctionConstantValues new];bool valid=device && library && values && !error;
    id<MTLFunction> absent=valid?[library newFunctionWithName:@"add_constant"]:nil;bool required=!absent;[absent release];
    NSError *emptyError=nil;absent=valid?[library newFunctionWithName:@"add_constant" constantValues:values error:&emptyError]:nil;
    bool missing=!absent && emptyError!=nil;[absent release];float wrong=13.0f;[values setConstantValue:&wrong type:MTLDataTypeFloat atIndex:7];
    NSError *wrongError=nil;absent=valid?[library newFunctionWithName:@"add_constant" constantValues:values error:&wrongError]:nil;
    bool typeCheck=!absent && wrongError!=nil;[absent release];[values reset];
    valid=valid && required && missing && typeCheck;
    emit("SWEET AIR CONSTANTS rejection required=%u missing_error=%u wrong_type=%u native_values_class=%s valid=%u",required,missing,typeCheck,object_getClassName(values),valid);
    id<MTLBuffer> buffer=[device newBufferWithLength:320 options:0];uint32_t *original=(uint32_t *)buffer.contents;
    id<MTLCommandQueue> queue=[device newCommandQueueWithMaxCommandBufferCount:8];valid=valid && buffer && original && queue;
    if(valid)for(unsigned i=0;i<80;++i)original[i]=(i*0x9e3779b1u)^0x434f4e53u;
    dispatch_semaphore_t callbacks=dispatch_semaphore_create(0);_Atomic unsigned completed=0;_Atomic unsigned *callbackCount=&completed;
    const uint32_t steps[3]={13,29,37};uint32_t total=0,sequences[3]={0};
    for(unsigned phase=0;phase<3 && valid;++phase){
        [values reset];uint32_t step=steps[phase],array[3]={step,5,7};
        if(phase==0)[values setConstantValue:&step type:MTLDataTypeUInt withName:@"step"];
        else if(phase==1)[values setConstantValue:&step type:MTLDataTypeUInt atIndex:7];
        else [values setConstantValues:array type:MTLDataTypeUInt withRange:NSMakeRange(7,3)];
        error=nil;id<MTLFunction> function=[library newFunctionWithName:@"add_constant" constantValues:values error:&error];
        MTLFunctionConstant *reflection=function.functionConstantsDictionary[@"step"];
        valid=function && reflection && reflection.index==7 && reflection.type==MTLDataTypeUInt && reflection.required;
        id<MTLComputePipelineState> pipeline=valid?[device newComputePipelineStateWithFunction:function error:&error]:nil;
        [values reset];step=99;[values setConstantValue:&step type:MTLDataTypeUInt atIndex:7];
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];valid=valid && pipeline && commands && encoder;
        if(valid){[encoder setComputePipelineState:pipeline];[encoder setBuffer:buffer offset:0 atIndex:0];
            [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];
            [commands addCompletedHandler:^(id<MTLCommandBuffer> c){(void)c;atomic_fetch_add_explicit(callbackCount,1,memory_order_relaxed);dispatch_semaphore_signal(callbacks);}];
            [commands commit];[commands waitUntilCompleted];valid=!dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC));}
        uint32_t operations=0;valid=valid && SweetMTLCommandResult(commands,&sequences[phase],&operations) && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && buffer.contents==original;
        unsigned mismatches=0;uint64_t hash=UINT64_C(14695981039346656037);total+=steps[phase];
        if(valid){for(unsigned i=0;i<80;++i){uint32_t expected=(i*0x9e3779b1u)^0x434f4e53u;if(i<64)expected+=total;mismatches+=original[i]!=expected;}
            const unsigned char *bytes=(const unsigned char *)original;for(unsigned i=0;i<320;++i)hash=(hash^bytes[i])*UINT64_C(1099511628211);}
        valid=valid && !mismatches && (phase==0 || sequences[phase]>sequences[phase-1]);
        emit("SWEET AIR CONSTANTS phase=%u step=%u total=%u status=%lu error=%u sequence=%u operations=%u callback_count=%u checked_words=80 guard_words=16 mismatches=%u hash=%016llx frozen_after_reset=1 valid=%u",
            phase,steps[phase],total,(unsigned long)commands.status,commands.error!=nil,sequences[phase],operations,
            atomic_load_explicit(callbackCount,memory_order_relaxed),mismatches,(unsigned long long)hash,valid);
        if(!valid)emit("SWEET AIR CONSTANTS failure phase=%u reason=%s",phase,(error?:commands.error).localizedDescription.UTF8String?:"pixel/fence/lifetime mismatch");
        [commands release];[pipeline release];[function release];
    }
    [values release];[library release];[buffer release];[queue release];NSUInteger final=device?device.currentAllocatedSize:0;
    valid=valid && final==0 && atomic_load_explicit(callbackCount,memory_order_relaxed)==3;
    emit("SWEET AIR CONSTANTS END result=%u phases=3 invocations=192 checked_words=240 guard_checks=48 final_allocated=%lu standard_Metal=1",valid?0:1,(unsigned long)final);
    [device release];dispatch_release(callbacks);return valid;
}
