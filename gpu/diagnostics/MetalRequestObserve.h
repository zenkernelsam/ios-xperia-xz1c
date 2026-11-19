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
typedef id (*SweetRequestInit)(id,SEL,CGRect,id,NSUInteger,NSInteger);
typedef id (*SweetRequestInitAsync)(id,SEL,CGRect,id,NSUInteger,NSInteger,id);
typedef void (*SweetRequestVoid0)(id,SEL);
typedef BOOL (*SweetRequestBool1)(id,SEL,id);
typedef BOOL (*SweetRequestBool2)(id,SEL,BOOL,id);
typedef CGSize (*SweetRequestSize0)(id,SEL);
static SweetRequestEmit request_emit;
static IMP request_function_original,request_constants_original,request_names_original;
static IMP request_drawable_original,request_device_original,request_size_original;
static IMP request_init_original,request_async_original,request_layout_original,request_display_original;
static IMP request_prewarm_original,request_getsize_original,request_updatesize_original;
static atomic_uint request_calls;
enum {SWEET_REQUEST_LIMIT=64};

static unsigned request_begin(const char *api,id receiver,id argument)
{
    unsigned call=atomic_fetch_add_explicit(&request_calls,1,memory_order_relaxed)+1;
    if(call<=SWEET_REQUEST_LIMIT){
        request_emit("SWEET METAL REQUEST BEGIN pid=%d call=%u api=%s original_forwarded=1",getpid(),call,api);
        request_emit("SWEET METAL REQUEST OBJECT pid=%d call=%u receiver=%.32s argument_nil=%u argument_class=%.32s",
            getpid(),call,receiver?object_getClassName(receiver):"nil",argument==nil,argument?object_getClassName(argument):"nil");}
    return call;
}
static void request_name(unsigned call,id name)
{
    if(call>SWEET_REQUEST_LIMIT)return;
    const char *value=[name isKindOfClass:[NSString class]]?[(NSString *)name UTF8String]:NULL;
    size_t bytes=value?strnlen(value,97):0;size_t count=bytes>96?96:bytes;
    request_emit("SWEET METAL REQUEST NAME pid=%d call=%u bytes=%lu truncated=%u original_name_argument=1",getpid(),call,(unsigned long)count,bytes>96);
    for(size_t at=0;at<count;at+=24){char hex[49]={0};size_t size=count-at<24?count-at:24;
        for(size_t i=0;i<size;++i)snprintf(hex+i*2,3,"%02x",(unsigned char)value[at+i]);
        request_emit("SWEET METAL REQUEST NAME_DATA pid=%d call=%u offset=%lu bytes=%lu hex=%s",getpid(),call,(unsigned long)at,(unsigned long)size,hex);}
}
static void request_end(unsigned call,const char *api,id result,bool returned,NSError **error)
{
    if(call<=SWEET_REQUEST_LIMIT){
        request_emit("SWEET METAL REQUEST RETURN pid=%d call=%u returned_nil=%u return_class=%.32s",getpid(),call,result==nil,result?object_getClassName(result):"nil");
        request_emit("SWEET METAL REQUEST ERROR pid=%d call=%u pointer=%u code=%lld",getpid(),call,error!=NULL,(long long)(returned && error && *error?(*error).code:0));
        request_emit("SWEET METAL REQUEST END pid=%d call=%u api=%s outcome=%s original_behavior=1",getpid(),call,api,returned?"return":"throw");}
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
static void request_init_input(unsigned call,CGRect rectangle,NSUInteger options,NSInteger colorSpace,id async)
{
    if(call>SWEET_REQUEST_LIMIT)return;
    request_emit("SWEET METAL REQUEST INIT pid=%d call=%u options=%llu color_space=%lld async_nil=%u",getpid(),call,(unsigned long long)options,(long long)colorSpace,async==nil);
    request_emit("SWEET METAL REQUEST ORIGIN pid=%d call=%u x=%.17g y=%.17g",getpid(),call,rectangle.origin.x,rectangle.origin.y);
    request_emit("SWEET METAL REQUEST SIZE pid=%d call=%u width=%.17g height=%.17g original_arguments=1",getpid(),call,rectangle.size.width,rectangle.size.height);
}
static id request_init(id receiver,SEL selector,CGRect rectangle,id identifier,NSUInteger options,NSInteger colorSpace)
{
    int incoming=errno;unsigned call=request_begin("quad_init",receiver,identifier);request_init_input(call,rectangle,options,colorSpace,nil);id result=nil;bool returned=false;
    @try{errno=incoming;result=((SweetRequestInit)request_init_original)(receiver,selector,rectangle,identifier,options,colorSpace);returned=true;return result;}
    @finally{int outgoing=errno;request_end(call,"quad_init",result,returned,NULL);errno=outgoing;}
}
static id request_async(id receiver,SEL selector,CGRect rectangle,id identifier,NSUInteger options,NSInteger colorSpace,id async)
{
    int incoming=errno;unsigned call=request_begin("quad_init_async",receiver,identifier);request_init_input(call,rectangle,options,colorSpace,async);id result=nil;bool returned=false;
    @try{errno=incoming;result=((SweetRequestInitAsync)request_async_original)(receiver,selector,rectangle,identifier,options,colorSpace,async);returned=true;return result;}
    @finally{int outgoing=errno;request_end(call,"quad_init_async",result,returned,NULL);errno=outgoing;}
}
static void request_void0(id receiver,SEL selector,const char *api,SweetRequestVoid0 original)
{
    int incoming=errno;unsigned call=request_begin(api,receiver,nil);bool returned=false;
    @try{errno=incoming;original(receiver,selector);returned=true;}
    @finally{int outgoing=errno;request_end(call,api,nil,returned,NULL);errno=outgoing;}
}
static void request_layout(id receiver,SEL selector)
{request_void0(receiver,selector,"quad_layout",(SweetRequestVoid0)request_layout_original);}
static void request_updatesize(id receiver,SEL selector)
{request_void0(receiver,selector,"quad_update_size",(SweetRequestVoid0)request_updatesize_original);}
static BOOL request_display(id receiver,SEL selector,BOOL flag,id completion)
{
    int incoming=errno;unsigned call=request_begin("quad_display",receiver,completion);BOOL answer=NO;bool returned=false;
    if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST FLAG pid=%d call=%u value=%u",getpid(),call,flag!=NO);
    @try{errno=incoming;answer=((SweetRequestBool2)request_display_original)(receiver,selector,flag,completion);returned=true;return answer;}
    @finally{int outgoing=errno;if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST BOOL pid=%d call=%u value=%u",getpid(),call,answer!=NO);request_end(call,"quad_display",nil,returned,NULL);errno=outgoing;}
}
static BOOL request_prewarm(id receiver,SEL selector,id completion)
{
    int incoming=errno;unsigned call=request_begin("quad_prewarm",receiver,completion);BOOL answer=NO;bool returned=false;
    @try{errno=incoming;answer=((SweetRequestBool1)request_prewarm_original)(receiver,selector,completion);returned=true;return answer;}
    @finally{int outgoing=errno;if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST BOOL pid=%d call=%u value=%u",getpid(),call,answer!=NO);request_end(call,"quad_prewarm",nil,returned,NULL);errno=outgoing;}
}
static CGSize request_getsize(id receiver,SEL selector)
{
    int incoming=errno;unsigned call=request_begin("quad_drawable_size",receiver,nil);CGSize answer={0};bool returned=false;
    @try{errno=incoming;answer=((SweetRequestSize0)request_getsize_original)(receiver,selector);returned=true;return answer;}
    @finally{int outgoing=errno;if(call<=SWEET_REQUEST_LIMIT)request_emit("SWEET METAL REQUEST SIZE pid=%d call=%u width=%.17g height=%.17g original_arguments=1",getpid(),call,answer.width,answer.height);request_end(call,"quad_drawable_size",nil,returned,NULL);errno=outgoing;}
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
        const char *types=method?method_getTypeEncoding(method):NULL;size_t bytes=types?strlen(types):0,names=strlen(selectors[i]);
        request_emit("SWEET METAL REQUEST ABI pid=%d owner=%s index=%u present=%u name_bytes=%lu type_bytes=%lu truncated=%u",getpid(),owner,i,method!=NULL,(unsigned long)names,(unsigned long)(bytes>144?144:bytes),bytes>144);
        const char *parts[2]={selectors[i],types};size_t lengths[2]={names,bytes>144?144:bytes};
        for(unsigned part=0;part<2;++part)for(size_t at=0;at<lengths[part];at+=24){size_t size=lengths[part]-at<24?lengths[part]-at:24;char hex[49]={0};
            for(size_t b=0;b<size;++b)snprintf(hex+b*2,3,"%02x",(unsigned char)parts[part][at+b]);
            request_emit("SWEET METAL REQUEST ABI_DATA pid=%d owner=%s index=%u part=%s offset=%lu bytes=%lu hex=%s",getpid(),owner,i,part?"types":"name",(unsigned long)at,(unsigned long)size,hex);}
        request_emit("SWEET METAL REQUEST ABI_END pid=%d owner=%s index=%u",getpid(),owner,i);
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
    const char *init_args[]={@encode(CGRect),@encode(id),@encode(NSUInteger),@encode(NSInteger)};
    const char *async_args[]={@encode(CGRect),@encode(id),@encode(NSUInteger),@encode(NSInteger),@encode(id)};
    const char *display_args[]={@encode(BOOL),"@?"};const char *block[]={"@?"};
    installed+=request_install(quad,"initWithFrame:identifier:options:colorSpace:","quad_init",(IMP)request_init,&request_init_original,@encode(id),init_args,4);
    installed+=request_install(quad,"initWithFrame:identifier:options:colorSpace:asyncRenderQueue:","quad_init_async",(IMP)request_async,&request_async_original,@encode(id),async_args,5);
    installed+=request_install(quad,"layoutSubviews","quad_layout",(IMP)request_layout,&request_layout_original,@encode(void),NULL,0);
    installed+=request_install(quad,"_displayAndCheckForDrawable:withCompletion:","quad_display",(IMP)request_display,&request_display_original,@encode(BOOL),display_args,2);
    installed+=request_install(quad,"prewarmWithCompletion:","quad_prewarm",(IMP)request_prewarm,&request_prewarm_original,@encode(BOOL),block,1);
    installed+=request_install(quad,"drawableSize","quad_drawable_size",(IMP)request_getsize,&request_getsize_original,@encode(CGSize),NULL,0);
    installed+=request_install(quad,"_updateDrawableSizeIfNecessary","quad_update_size",(IMP)request_updatesize,&request_updatesize_original,@encode(void),NULL,0);
    const char *quad_methods[]={"initWithFrame:identifier:options:colorSpace:","initWithFrame:identifier:options:colorSpace:asyncRenderQueue:",
        "_displayAndCheckForDrawable:withCompletion:","prewarmWithCompletion:","snapshotTextureInRect:scale:time:","_msaaCount"};
    const char *renderer_methods[]={"initWithPixelFormat:textureSuffix:rendererStyle:","renderOffscreenWithScene:viewport:commandBuffer:","renderWithScene:viewport:commandBuffer:passDescriptor:","setRendererOptions:"};
    request_inventory(quad,"quad",quad_methods,sizeof(quad_methods)/sizeof(*quad_methods));
    request_inventory(renderer,"renderer",renderer_methods,sizeof(renderer_methods)/sizeof(*renderer_methods));
    request_emit("SWEET METAL REQUEST CONFIG pid=%d installed=%u expected=13 limit=64 revision=2 original_behavior=1 no_extra_GPU_work=1",getpid(),installed);
    errno=incoming;return installed;
}
