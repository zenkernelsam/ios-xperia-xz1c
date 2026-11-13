/* Metadata only: no Metal instance, allocation, query or command is invoked. */
#pragma once
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

typedef void (*MetalABIEmit)(const char *,...);
static const char *metal_protocol_types(Protocol *protocol,SEL selector,unsigned depth)
{
    if(!protocol || depth>8)return NULL;
    for(unsigned required=0;required<2;++required) {
        struct objc_method_description description=protocol_getMethodDescription(protocol,selector,required,YES);
        if(description.name && description.types)return description.types;
    }
    unsigned count=0;Protocol *__unsafe_unretained *parents=protocol_copyProtocolList(protocol,&count);
    const char *types=NULL;
    if(count<=16)for(unsigned i=0;i<count && !types;++i)types=metal_protocol_types(parents[i],selector,depth+1);
    free(parents);return types;
}
static void metal_abi_method(MetalABIEmit emit,unsigned index,const char *owner,const char *selector_name,const char *types)
{
    char hex[257]={0};size_t bytes=types?strlen(types):0;
    if(bytes>128){emit("SWEET METAL ABI method index=%u owner=%s selector=%s present=1 types_bytes=%lu oversized=1 metadata_only=1",
        index,owner,selector_name,(unsigned long)bytes);return;}
    const char digits[]="0123456789abcdef";
    for(size_t i=0;i<bytes;++i){unsigned byte=(unsigned char)types[i];hex[2*i]=digits[byte>>4];hex[2*i+1]=digits[byte&15];}
    emit("SWEET METAL ABI method index=%u owner=%s selector=%s present=%u types_hex=%s metadata_only=1",
        index,owner,selector_name,types!=NULL,types?hex:"none");
}
static void metal_runtime_observe(Class base,Protocol *device,Protocol *buffer,Protocol *queue,Protocol *commands,MetalABIEmit emit)
{
    Ivar limits=base?class_getInstanceVariable(base,"_limits"):NULL;
    NSUInteger size=0,alignment=0;
    if(limits)NSGetSizeAndAlignment(ivar_getTypeEncoding(limits),&size,&alignment);
    bool bounded=limits && ivar_getOffset(limits)>=0 && size<=class_getInstanceSize(base) &&
        (size_t)ivar_getOffset(limits)<=class_getInstanceSize(base)-size;
    emit("SWEET METAL ABI begin class=%s present=%u limits_bytes=%lu limits_alignment=%lu limits_bounded=%u device_protocol=%u buffer_protocol=%u queue_protocol=%u command_protocol=%u metadata_only=1",
        base?class_getName(base):"nil",base!=Nil,(unsigned long)size,(unsigned long)alignment,bounded,device!=NULL,buffer!=NULL,queue!=NULL,commands!=NULL);
    const char *class_selectors[]={"init","initWithAcceleratorPort:","initLimits","initFeatureQueries","featureProfile"};
    unsigned index=0;
    for(unsigned i=0;i<5;++i) {
        Method method=base?class_getInstanceMethod(base,sel_registerName(class_selectors[i])):NULL;
        metal_abi_method(emit,index++,"device_base",class_selectors[i],method?method_getTypeEncoding(method):NULL);
    }
    struct {const char *owner,*selector;Protocol *protocol;} methods[]={
        {"device","newBufferWithLength:options:",device},
        {"device","newBufferWithBytes:length:options:",device},
        {"device","newCommandQueue",device},
        {"device","newLibraryWithData:error:",device},
        {"device","newTextureWithDescriptor:",device},
        {"device","newRenderPipelineStateWithDescriptor:error:",device},
        {"device","newComputePipelineStateWithFunction:error:",device},
        {"buffer","contents",buffer},{"buffer","length",buffer},{"buffer","gpuAddress",buffer},
        {"queue","commandBuffer",queue},{"queue","commandBufferWithUnretainedReferences",queue},
        {"commands","commit",commands},{"commands","waitUntilCompleted",commands},
        {"commands","addCompletedHandler:",commands},{"commands","status",commands},
        {"commands","error",commands}};
    for(unsigned i=0;i<sizeof(methods)/sizeof(methods[0]);++i)
        metal_abi_method(emit,index++,methods[i].owner,methods[i].selector,
            metal_protocol_types(methods[i].protocol,sel_registerName(methods[i].selector),0));
    emit("SWEET METAL ABI end methods=%u metadata_only=1 instances_created=0 gpu_operations=0",index);
}
