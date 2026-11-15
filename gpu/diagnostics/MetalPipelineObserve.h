/* Original native factory metadata; every return/error pointer is forwarded. */
#pragma once
#import <Metal/Metal.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <string.h>
typedef void (*SweetPipelineEmit)(const char *,...);
typedef id (*SweetNativeFactory)(id,SEL,id,NSError **);
static SweetPipelineEmit pipeline_emit;
static SweetNativeFactory pipeline_original,library_url_original,library_bundle_original;
static atomic_uint pipeline_calls,library_calls;
static const char *pipeline_text(NSString *text)
{return [text isKindOfClass:[NSString class]]?text.UTF8String:"nil";}
static id observe_pipeline_factory(id object,SEL selector,MTLRenderPipelineDescriptor *d,NSError **error)
{
    unsigned call=atomic_fetch_add_explicit(&pipeline_calls,1,memory_order_relaxed)+1;
    if(call<=32){
        pipeline_emit("SWEET METAL PIPELINE begin call=%u format=%lu samples=%lu depth=%lu stencil=%lu raster=%u original_forwarded=1",call,
            (unsigned long)d.colorAttachments[0].pixelFormat,(unsigned long)d.rasterSampleCount,(unsigned long)d.depthAttachmentPixelFormat,
            (unsigned long)d.stencilAttachmentPixelFormat,d.rasterizationEnabled);
        pipeline_emit("SWEET METAL PIPELINE function call=%u stage=vertex name=%.144s",call,pipeline_text(d.vertexFunction.name));
        pipeline_emit("SWEET METAL PIPELINE function call=%u stage=fragment name=%.144s",call,pipeline_text(d.fragmentFunction.name));
        pipeline_emit("SWEET METAL PIPELINE states call=%u vertex_class=%s fragment_class=%s blend=%u mask=%lu alphaCoverage=%u alphaOne=%u",call,
            d.vertexFunction?object_getClassName(d.vertexFunction):"nil",d.fragmentFunction?object_getClassName(d.fragmentFunction):"nil",
            d.colorAttachments[0].blendingEnabled,(unsigned long)d.colorAttachments[0].writeMask,d.alphaToCoverageEnabled,d.alphaToOneEnabled);
        for(unsigned i=0;i<8;++i){MTLRenderPipelineColorAttachmentDescriptor *color=d.colorAttachments[i];if(!color.pixelFormat)continue;
            pipeline_emit("SWEET METAL PIPELINE color call=%u index=%u format=%lu blend=%u mask=%lu srcRGB=%lu dstRGB=%lu opRGB=%lu srcAlpha=%lu dstAlpha=%lu opAlpha=%lu",call,i,
                (unsigned long)color.pixelFormat,color.blendingEnabled,(unsigned long)color.writeMask,(unsigned long)color.sourceRGBBlendFactor,(unsigned long)color.destinationRGBBlendFactor,
                (unsigned long)color.rgbBlendOperation,(unsigned long)color.sourceAlphaBlendFactor,(unsigned long)color.destinationAlphaBlendFactor,(unsigned long)color.alphaBlendOperation);}
        for(unsigned i=0;i<31;++i){MTLVertexBufferLayoutDescriptor *layout=d.vertexDescriptor.layouts[i];
            if(layout.stride)pipeline_emit("SWEET METAL PIPELINE layout call=%u index=%u stride=%lu step=%lu rate=%lu",call,i,(unsigned long)layout.stride,(unsigned long)layout.stepFunction,(unsigned long)layout.stepRate);
            MTLVertexAttributeDescriptor *attribute=d.vertexDescriptor.attributes[i];
            if(attribute.format)pipeline_emit("SWEET METAL PIPELINE attribute call=%u index=%u format=%lu offset=%lu buffer=%lu",call,i,(unsigned long)attribute.format,(unsigned long)attribute.offset,(unsigned long)attribute.bufferIndex);}
    }
    id result=pipeline_original(object,selector,d,error);
    if(call<=32){pipeline_emit("SWEET METAL PIPELINE END call=%u returned_nil=%u error_pointer=%u error_code=%lld original_return=1",call,result==nil,error!=NULL,(long long)(error && *error?(*error).code:0));
        if(error && *error)pipeline_emit("SWEET METAL PIPELINE error call=%u reason=\"%.168s\"",call,pipeline_text((*error).localizedDescription));}
    return result;
}
static id observe_library_url(id object,SEL selector,NSURL *url,NSError **error)
{
    unsigned call=atomic_fetch_add_explicit(&library_calls,1,memory_order_relaxed)+1;
    if(call<=32)pipeline_emit("SWEET METAL LIBRARY begin call=%u kind=url path=%.160s original_forwarded=1",call,pipeline_text(url.path));
    id result=library_url_original(object,selector,url,error);
    if(call<=32){pipeline_emit("SWEET METAL LIBRARY END call=%u kind=url returned_nil=%u error_pointer=%u error_code=%lld original_return=1",call,result==nil,error!=NULL,(long long)(error && *error?(*error).code:0));
        if(error && *error)pipeline_emit("SWEET METAL LIBRARY error call=%u reason=\"%.168s\"",call,pipeline_text((*error).localizedDescription));}
    return result;
}
static id observe_library_bundle(id object,SEL selector,NSBundle *bundle,NSError **error)
{
    unsigned call=atomic_fetch_add_explicit(&library_calls,1,memory_order_relaxed)+1;
    if(call<=32)pipeline_emit("SWEET METAL LIBRARY begin call=%u kind=bundle path=%.160s original_forwarded=1",call,pipeline_text(bundle.bundlePath));
    id result=library_bundle_original(object,selector,bundle,error);
    if(call<=32){pipeline_emit("SWEET METAL LIBRARY END call=%u kind=bundle returned_nil=%u error_pointer=%u error_code=%lld original_return=1",call,result==nil,error!=NULL,(long long)(error && *error?(*error).code:0));
        if(error && *error)pipeline_emit("SWEET METAL LIBRARY error call=%u reason=\"%.168s\"",call,pipeline_text((*error).localizedDescription));}
    return result;
}
static bool pipeline_factory_hook(Class type,const char *name,IMP observer,SweetNativeFactory *original)
{
    Method method=class_getInstanceMethod(type,sel_registerName(name));char output[16]={0},input[16]={0},pointer[16]={0};
    if(method){method_getReturnType(method,output,sizeof(output));method_getArgumentType(method,2,input,sizeof(input));method_getArgumentType(method,3,pointer,sizeof(pointer));}
    if(!method || method_getNumberOfArguments(method)!=4 || output[0]!='@' || input[0]!='@' || strcmp(pointer,"^@"))return false;
    *original=(SweetNativeFactory)method_getImplementation(method);if(!*original)return false;method_setImplementation(method,observer);return true;
}
static bool metal_pipeline_observe(Class type,SweetPipelineEmit emit)
{
    pipeline_emit=emit;bool valid=type && pipeline_factory_hook(type,"newRenderPipelineStateWithDescriptor:error:",(IMP)observe_pipeline_factory,&pipeline_original) &&
        pipeline_factory_hook(type,"newLibraryWithURL:error:",(IMP)observe_library_url,&library_url_original) &&
        pipeline_factory_hook(type,"newDefaultLibraryWithBundle:error:",(IMP)observe_library_bundle,&library_bundle_original);
    emit("SWEET METAL PIPELINE observer installed=%u methods=3 metadata_only=1 original_returns_errors=1",valid);return valid;
}
