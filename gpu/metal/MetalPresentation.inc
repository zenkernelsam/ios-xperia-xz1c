// SPDX-License-Identifier: BSD-2-Clause
// Hand the real QuartzCore drawable back only after owned GPU writes retire.
static NSError *publish_metal_surface(SweetMTLTexture *texture,uint32_t sequence)
{
    if(!texture->_surface)return nil;
    int sync=sweetgl_metal_buffer_synchronize([texture sweetBacking].handle);if(sync)return metal_queue_error(@"Original GPU IOSurface page synchronization failed");
    if(IOSurfaceLock(texture->_surface,0,nullptr)!=KERN_SUCCESS)return metal_queue_error(@"Original GPU IOSurface publication lock failed");
    if(IOSurfaceUnlock(texture->_surface,0,nullptr)!=KERN_SUCCESS)return metal_queue_error(@"Original GPU IOSurface publication unlock failed");
    if(texture->_drawable)fprintf(stderr,"SWEET DRAWABLE PUBLISH surface=%u handle=%u sequence=%u result=0 original_surface=1\n",
        IOSurfaceGetID(texture->_surface),[texture sweetBacking].handle,sequence);
    return nil;
}
static NSError *publish_metal_render_surfaces(NSDictionary *operation,id<MTLDevice> device,uint32_t sequence)
{
    NSArray *textures=operation[@"multisampleResolve"]?@[operation[@"targetTexture"]]:operation[@"targets"];
    for(id object in textures){if(object==[NSNull null])continue;auto texture=(SweetMTLTexture *)object;
        if(![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device)return metal_queue_error(@"GPU surface publication owner changed");
        NSError *failure=publish_metal_surface(texture,sequence);if(failure)return failure;}
    return nil;
}
static NSDictionary *new_metal_presentation(id drawable,id<MTLDevice> device,NSUInteger mode,double time,NSDictionary *options,NSError **error)
{
    if(error)*error=nil;
    bool valid=drawable && [drawable conformsToProtocol:@protocol(MTLDrawable)] && mode<=3 && std::isfinite(time) && time>=0 &&
        sweet_native_method(drawable,@selector(texture),@encode(id));
    SEL method=mode==0?@selector(present):mode==1?@selector(presentAtTime:):mode==2?@selector(presentAfterMinimumDuration:):@selector(presentWithOptions:);
    valid=valid && sweet_native_method(drawable,method,@encode(void),mode==0?nullptr:mode==3?@encode(id):@encode(double));
    if(mode==3)valid=valid && (!options || [options isKindOfClass:[NSDictionary class]]);
    auto texture=valid?(SweetMTLTexture *)[(id<SweetNativeDrawable16>)drawable texture]:nil;
    valid=valid && [texture isKindOfClass:[SweetMTLTexture class]] && texture.device==device && texture->_drawable && texture->_surface &&
        texture.sampleCount==1 && texture.mipmapLevelCount==1 && (!texture.usage || (texture.usage&MTLTextureUsageRenderTarget));
    if(!valid){if(error)*error=metal_queue_error(@"Original drawable, texture owner, format or presentation ABI is unsupported");return nil;}
    SweetShaderTextureBinding binding=[texture bindingForIndex:0 stage:0];
    if(!binding.handle){if(error)*error=metal_queue_error(@"Native drawable has no owned original IOSurface page");return nil;}
    NSDictionary *copied=mode==3 && options?[options copy]:nil;
    auto result=[[NSDictionary alloc] initWithObjectsAndKeys:drawable,@"drawable",texture,@"texture",@(mode),@"mode",@(time),@"time",
        copied?(id)copied:[NSNull null],@"options",[NSData dataWithBytes:&binding length:sizeof(binding)],@"binding",nil];
    [copied release];return result;
}
static NSError *execute_metal_presentation(NSDictionary *record,id<MTLDevice> device,uint32_t completedSequence)
{
    @try {
        id drawable=record[@"drawable"];auto texture=(SweetMTLTexture *)record[@"texture"];NSData *snapshot=record[@"binding"];
        unsigned mode=[record[@"mode"] unsignedIntValue];double time=[record[@"time"] doubleValue];
        if(![texture isKindOfClass:[SweetMTLTexture class]] || texture.device!=device || !texture->_drawable || !texture->_surface ||
            ![snapshot isKindOfClass:[NSData class]] || snapshot.length!=sizeof(SweetShaderTextureBinding) || mode>3 || !std::isfinite(time) || time<0 ||
            !sweet_native_method(drawable,@selector(texture),@encode(id)) || [(id<SweetNativeDrawable16>)drawable texture]!=texture)
            return metal_queue_error(@"Retained drawable or IOSurface texture snapshot changed before presentation");
        auto actual=[texture bindingForIndex:0 stage:0];if(memcmp(&actual,snapshot.bytes,sizeof(actual)))return metal_queue_error(@"Presentation no longer refers to its original resource page");
        int sync=sweetgl_metal_buffer_synchronize(actual.handle);if(sync)return metal_queue_error(@"Original drawable GPU page synchronization failed");
        // Render/resolve already published these original pages before the
        // queue reached Scheduled. Do not invent a display-completion event.
        SEL method=mode==0?@selector(present):mode==1?@selector(presentAtTime:):mode==2?@selector(presentAfterMinimumDuration:):@selector(presentWithOptions:);
        if(!sweet_native_method(drawable,method,@encode(void),mode==0?nullptr:mode==3?@encode(id):@encode(double)))return metal_queue_error(@"Original presentation method ABI changed");
        uint64_t bits=0;memcpy(&bits,&time,sizeof(bits));
        fprintf(stderr,"SWEET PRESENT BEGIN mode=%u drawable=%016llx surface=%u handle=%u sequence=%u time_bits=%016llx original_object=1\n",
            mode,(unsigned long long)(uintptr_t)drawable,IOSurfaceGetID(texture->_surface),actual.handle,completedSequence,(unsigned long long)bits);
        if(mode==0)[drawable present];else if(mode==1)[drawable presentAtTime:time];else if(mode==2)[drawable presentAfterMinimumDuration:time];
        else [(id<SweetNativeDrawable16>)drawable presentWithOptions:record[@"options"]==[NSNull null]?nil:record[@"options"]];
        fprintf(stderr,"SWEET PRESENT END mode=%u surface=%u sequence=%u result=0 original_present=1 display_completion_unverified=1\n",mode,IOSurfaceGetID(texture->_surface),completedSequence);
        return nil;
    } @catch(NSException *exception) {
        return metal_queue_error(exception.reason?:@"Original drawable presentation raised an exception");
    }
}
