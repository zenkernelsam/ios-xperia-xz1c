/* SPDX-License-Identifier: BSD-2-Clause
 * Observe original library/layer calls. Preserve arguments, returns and errors. */
#pragma once
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*SweetRequestEmit)(const char *,...);
typedef id (*SweetRequestId0)(id,SEL);
typedef id (*SweetRequestId1)(id,SEL,id);
typedef id (*SweetRequestId3)(id,SEL,id,id,NSError **);
typedef void (*SweetRequestVoid1)(id,SEL,id);
typedef void (*SweetRequestSize1)(id,SEL,CGSize);
static SweetRequestEmit request_emit;
static IMP request_function_original,request_constants_original,request_names_original;
static IMP request_drawable_original,request_device_original,request_size_original;
static atomic_uint request_calls;
enum {SWEET_REQUEST_LIMIT=64};

static unsigned request_begin(const char *api,id receiver,id argument)
{
    unsigned call=atomic_fetch_add_explicit(&request_calls,1,memory_order_relaxed)+1;
    if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST BEGIN pid=%d call=%u api=%s receiver=%s argument_nil=%u argument_class=%s original_forwarded=1",
        getpid(),call,api,receiver?object_getClassName(receiver):"nil",argument==nil,argument?object_getClassName(argument):"nil");
    return call;
}
static void request_name(unsigned call,id name)
{
    if(call>SWEET_REQUEST_LIMIT)return;
    const char *value=[name isKindOfClass:[NSString class]]?[(NSString *)name UTF8String]:NULL;
    size_t bytes=value?strnlen(value,97):0;char hex[193]={0};size_t count=bytes>96?96:bytes;
    for(size_t i=0;i<count;++i)snprintf(hex+i*2,3,"%02x",(unsigned char)value[i]);
    request_emit("SWEET METAL REQUEST NAME pid=%d call=%u bytes=%lu hex=%s truncated=%u original_name_argument=1",
        getpid(),call,(unsigned long)count,count?hex:"none",bytes>96);
}
static void request_end(unsigned call,const char *api,id result,bool returned,NSError **error)
{
    if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST END pid=%d call=%u api=%s outcome=%s returned_nil=%u return_class=%s error_pointer=%u error_code=%lld original_behavior=1",
        getpid(),call,api,returned?"return":"throw",result==nil,result?object_getClassName(result):"nil",error!=NULL,
        (long long)(returned && error && *error?(*error).code:0));
}
static id request_id0(id receiver,SEL selector,const char *api,SweetRequestId0 original)
{
    int incoming=errno;unsigned call=request_begin(api,receiver,nil);id result=nil;bool returned=false;
    @try {errno=incoming;result=original(receiver,selector);returned=true;return result;}
    @finally {int outgoing=errno;request_end(call,api,result,returned,NULL);errno=outgoing;}
}
static id request_names(id receiver,SEL selector)
{return request_id0(receiver,selector,"function_names",(SweetRequestId0)request_names_original);}
static id request_drawable(id receiver,SEL selector)
{return request_id0(receiver,selector,"next_drawable",(SweetRequestId0)request_drawable_original);}
static id request_function(id receiver,SEL selector,id name)
{
    int incoming=errno;unsigned call=request_begin("function",receiver,name);request_name(call,name);id result=nil;bool returned=false;
    @try {errno=incoming;result=((SweetRequestId1)request_function_original)(receiver,selector,name);returned=true;return result;}
    @finally {int outgoing=errno;request_end(call,"function",result,returned,NULL);errno=outgoing;}
}
static id request_constants(id receiver,SEL selector,id name,id values,NSError **error)
{
    int incoming=errno;unsigned call=request_begin("function_constants",receiver,name);request_name(call,name);id result=nil;bool returned=false;
    @try {errno=incoming;result=((SweetRequestId3)request_constants_original)(receiver,selector,name,values,error);returned=true;return result;}
    @finally {int outgoing=errno;request_end(call,"function_constants",result,returned,error);errno=outgoing;}
}
static void request_device(id receiver,SEL selector,id device)
{
    int incoming=errno;unsigned call=request_begin("set_device",receiver,device);bool returned=false;
    @try {errno=incoming;((SweetRequestVoid1)request_device_original)(receiver,selector,device);returned=true;}
    @finally {int outgoing=errno;request_end(call,"set_device",nil,returned,NULL);errno=outgoing;}
}
static void request_size(id receiver,SEL selector,CGSize size)
{
    int incoming=errno;unsigned call=request_begin("set_drawable_size",receiver,nil);bool returned=false;
    if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST SIZE pid=%d call=%u width=%.17g height=%.17g original_arguments=1",getpid(),call,size.width,size.height);
    @try {errno=incoming;((SweetRequestSize1)request_size_original)(receiver,selector,size);returned=true;}
    @finally {int outgoing=errno;request_end(call,"set_drawable_size",nil,returned,NULL);errno=outgoing;}
}
static bool request_install(Class type,const char *name,const char *api,IMP replacement,IMP *original,
    const char *result,const char *const *arguments,unsigned count)
{
    int incoming=errno;SEL selector=sel_registerName(name);Method method=type?class_getInstanceMethod(type,selector):NULL;
    bool owned=false;unsigned total=0;Method *methods=type?class_copyMethodList(type,&total):NULL;
    for(unsigned i=0;i<total;++i)if(method_getName(methods[i])==selector)owned=true;free(methods);
    char actual[192]={0};if(method)method_getReturnType(method,actual,sizeof(actual));
    bool valid=method && owned && method_getNumberOfArguments(method)==count+2 && !strcmp(actual,result);
    for(unsigned i=0;valid && i<count;++i){method_getArgumentType(method,i+2,actual,sizeof(actual));valid=!strcmp(actual,arguments[i]);}
    IMP prior=valid?method_getImplementation(method):NULL;valid=valid && prior && prior!=replacement;
    if(valid){*original=prior;method_setImplementation(method,replacement);}
    request_emit("SWEET METAL REQUEST METHOD pid=%d api=%s present=%u owned=%u installed=%u arguments=%u exact_ABI=1",
        getpid(),api,method!=NULL,owned,valid,method?method_getNumberOfArguments(method):0);errno=incoming;return valid;
}
static void request_inventory(Class type,const char *owner,const char *const *selectors,unsigned count)
{
    for(unsigned i=0;i<count;++i){Method method=type?class_getInstanceMethod(type,sel_registerName(selectors[i])):NULL;
        const char *types=method?method_getTypeEncoding(method):NULL;size_t bytes=types?strlen(types):0;char hex[289]={0};
        if(bytes>144)bytes=144;for(size_t b=0;b<bytes;++b)snprintf(hex+b*2,3,"%02x",(unsigned char)types[b]);
        request_emit("SWEET METAL REQUEST ABI pid=%d owner=%s selector=%s present=%u types_hex=%s truncated=%u metadata_only=1",
            getpid(),owner,selectors[i],method!=NULL,types?hex:"none",types && strlen(types)>144);
    }
}
static unsigned metal_request_observe(Class library,Class layer,Class quad,Class renderer,SweetRequestEmit emit)
{
    int incoming=errno;request_emit=emit;const char *object[]={@encode(id)};
    const char *constants[]={@encode(id),@encode(id),@encode(NSError **)};const char *size[]={@encode(CGSize)};
    unsigned installed=0;
    installed+=request_install(library,"newFunctionWithName:","function",(IMP)request_function,&request_function_original,@encode(id),object,1);
    installed+=request_install(library,"newFunctionWithName:constantValues:error:","function_constants",(IMP)request_constants,&request_constants_original,@encode(id),constants,3);
    installed+=request_install(library,"functionNames","function_names",(IMP)request_names,&request_names_original,@encode(id),NULL,0);
    installed+=request_install(layer,"nextDrawable","next_drawable",(IMP)request_drawable,&request_drawable_original,@encode(id),NULL,0);
    installed+=request_install(layer,"setDevice:","set_device",(IMP)request_device,&request_device_original,@encode(void),object,1);
    installed+=request_install(layer,"setDrawableSize:","set_drawable_size",(IMP)request_size,&request_size_original,@encode(void),size,1);
    const char *quad_methods[]={"initWithFrame:identifier:options:colorSpace:","initWithFrame:identifier:options:colorSpace:asyncRenderQueue:",
        "_displayAndCheckForDrawable:withCompletion:","prewarmWithCompletion:","snapshotTextureInRect:scale:time:","_msaaCount"};
    const char *renderer_methods[]={"initWithPixelFormat:textureSuffix:rendererStyle:","renderOffscreenWithScene:viewport:commandBuffer:","renderWithScene:viewport:commandBuffer:passDescriptor:","setRendererOptions:"};
    request_inventory(quad,"quad",quad_methods,sizeof(quad_methods)/sizeof(*quad_methods));
    request_inventory(renderer,"renderer",renderer_methods,sizeof(renderer_methods)/sizeof(*renderer_methods));
    request_emit("SWEET METAL REQUEST CONFIG pid=%d installed=%u expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1",getpid(),installed);
    errno=incoming;return installed;
}
