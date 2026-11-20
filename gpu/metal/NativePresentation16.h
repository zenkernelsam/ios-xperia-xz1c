// SPDX-License-Identifier: BSD-2-Clause
// Selectors verified on the user's iOS16.7.16/20H392 Metal/QuartzCore objects.
// This declares projections only, without replacing any system implementation.
#pragma once
#import <Metal/Metal.h>
#include <objc/runtime.h>
#include <string.h>

@interface MTLTextureDescriptor (SweetNativePresentation16)
- (BOOL)isDrawable;
- (BOOL)framebufferOnly;
- (NSUInteger)rotation;
- (NSUInteger)protectionOptions;
@end
@protocol SweetNativeDrawable16 <MTLDrawable>
@property (readonly) id<MTLTexture> texture;
- (void)presentWithOptions:(NSDictionary *)options;
@end
@protocol SweetNativeCommandPresent16 <MTLCommandBuffer>
- (void)presentDrawable:(id<MTLDrawable>)drawable options:(NSDictionary *)options;
@end
#if defined(__cplusplus)
struct SweetNativeTextureFlags {bool drawable=false,framebufferOnly=false;};
static bool sweet_native_method(id object,SEL selector,const char *result,const char *argument=nullptr)
{
    Method method=object?class_getInstanceMethod(object_getClass(object),selector):nullptr;
    if(!method || method_getNumberOfArguments(method)!=(argument?3u:2u))return false;
    char type[64]={0};method_getReturnType(method,type,sizeof(type));if(strcmp(type,result))return false;
    if(argument){method_getArgumentType(method,2,type,sizeof(type));if(strcmp(type,argument))return false;}return true;
}
static bool sweet_native_texture_flags(MTLTextureDescriptor *descriptor,bool surface, SweetNativeTextureFlags &flags)
{
    flags={};if(!descriptor)return false;
    const SEL keys[]={@selector(isDrawable),@selector(framebufferOnly),@selector(rotation),@selector(protectionOptions)};
    for(unsigned i=0;i<4;++i){Method method=class_getInstanceMethod(object_getClass(descriptor),keys[i]);if(!method)continue;
        if(!sweet_native_method(descriptor,keys[i],i<2?@encode(BOOL):@encode(NSUInteger)))return false;
        if(i==0)flags.drawable=[descriptor isDrawable];else if(i==1)flags.framebufferOnly=[descriptor framebufferOnly];
        else if(i==2 && [descriptor rotation])return false;else if(i==3 && [descriptor protectionOptions])return false;}
    if(flags.drawable && !surface)return false;
    if(surface){auto channels=descriptor.swizzle;
        if(channels.red!=MTLTextureSwizzleRed || channels.green!=MTLTextureSwizzleGreen || channels.blue!=MTLTextureSwizzleBlue || channels.alpha!=MTLTextureSwizzleAlpha)return false;}
    return true;
}
#endif
