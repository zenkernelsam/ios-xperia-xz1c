// SPDX-License-Identifier: BSD-2-Clause
// Native resolve operations retain the original sample source and target.
static_assert(MTLMultisampleDepthResolveFilterSample0==0 && MTLMultisampleDepthResolveFilterMin==1 &&
    MTLMultisampleDepthResolveFilterMax==2,"Native depth-resolve filter ABI differs from GLES transport");
static bool multisample_textures_overlap(SweetMTLTexture *left,SweetMTLTexture *right)
{
    if(!left || !right)return false;
    if(![left isKindOfClass:[SweetMTLTexture class]] || ![right isKindOfClass:[SweetMTLTexture class]])return true;
    if(left==right || (left->_surface && left->_surface==right->_surface))return true;
    auto a=[left sweetBacking],b=[right sweetBacking];
    if(!a.handle || !b.handle || a.handle!=b.handle)return false;
    // Valid immutable layouts have already bounded offset+bytes to their owner.
    return left->_layout.offset<right->_layout.offset+right->_layout.bytes &&
        right->_layout.offset<left->_layout.offset+left->_layout.bytes;
}
static bool multisample_pass_resolves(SweetMTLCommandBuffer *commands,MTLRenderPassDescriptor *pass,NSArray **operations)
{
    @autoreleasepool{
    if(!commands || !pass || !operations)return false;*operations=nil;NSMutableArray *resolves=[NSMutableArray new];bool valid=true;
    for(unsigned slot=0;slot<10 && valid;++slot){auto attachment=memoryless_attachment(pass,slot);auto source=(SweetMTLTexture *)attachment.texture;
        if(!source){if(attachment.resolveTexture)valid=false;continue;}
        if(![source isKindOfClass:[SweetMTLTexture class]]){valid=false;break;}
        bool resolve=attachment.storeAction==MTLStoreActionMultisampleResolve || attachment.storeAction==MTLStoreActionStoreAndMultisampleResolve;
        if(!resolve){if(attachment.resolveTexture)valid=false;continue;}
        auto target=(SweetMTLTexture *)attachment.resolveTexture;
        valid=slot<9 && source.device==commands.device && source.sampleCount>1 &&
            [target isKindOfClass:[SweetMTLTexture class]] && target.device==commands.device && target.sampleCount==1 &&
            target.storageMode!=MTLStorageModeMemoryless && source.pixelFormat==target.pixelFormat &&
            source.textureType==MTLTextureType2DMultisample && target.textureType==MTLTextureType2D &&
            source.mipmapLevelCount==1 && target.mipmapLevelCount==1 &&
            source.width==target.width && source.height==target.height && !attachment.resolveLevel && !attachment.resolveSlice && !attachment.resolveDepthPlane &&
            !attachment.level && !attachment.slice && !attachment.depthPlane && (!target.usage || (target.usage&MTLTextureUsageRenderTarget));
        if(!valid)break;
        // A resolve target must not overwrite any live color/depth/stencil
        // source or another resolve destination in this same render pass.
        for(unsigned other=0;other<10 && valid;++other)
            if(multisample_textures_overlap(target,(SweetMTLTexture *)memoryless_attachment(pass,other).texture))valid=false;
        for(NSDictionary *prior in resolves)if(multisample_textures_overlap(target,prior[@"targetTexture"]))valid=false;
        if(!valid)break;
        unsigned filter=0;
        if(slot==8){auto depth=pass.depthAttachment;valid=source.pixelFormat==MTLPixelFormatDepth32Float &&
            (depth.depthResolveFilter==MTLMultisampleDepthResolveFilterSample0 || depth.depthResolveFilter==MTLMultisampleDepthResolveFilterMin || depth.depthResolveFilter==MTLMultisampleDepthResolveFilterMax);
            filter=(unsigned)depth.depthResolveFilter;}
        if(!valid)break;
        auto src=[source bindingForIndex:0 stage:0],dst=[target bindingForIndex:0 stage:0];
        if(!src.handle || !dst.handle){valid=false;break;}
        SweetMultisampleResolve request={src,dst,(uint32_t)source.sampleCount,slot==8?1u:0u,filter,0};
        [resolves addObject:@{@"multisampleResolve":@YES,@"sourceTexture":source,@"targetTexture":target,
            @"resolveRequest":[NSData dataWithBytes:&request length:sizeof(request)]}];
        bool preserve=attachment.storeAction==MTLStoreActionStoreAndMultisampleResolve;
        attachment.resolveTexture=nil;attachment.storeAction=preserve?MTLStoreActionStore:MTLStoreActionDontCare;
    }
    if(valid)*operations=[resolves copy];[resolves release];return valid;}
}
static NSError *execute_multisample_resolve(NSDictionary *operation,id<MTLDevice> device,uint32_t *sequence)
{
    auto source=(SweetMTLTexture *)operation[@"sourceTexture"],target=(SweetMTLTexture *)operation[@"targetTexture"];
    NSData *data=operation[@"resolveRequest"];SweetMultisampleResolve request={};
    if(![source isKindOfClass:[SweetMTLTexture class]] || ![target isKindOfClass:[SweetMTLTexture class]] ||
        source.device!=device || target.device!=device || source.sampleCount<2 || target.sampleCount!=1 ||
        source->_memorylessOwner || target->_memorylessOwner || ![data isKindOfClass:[NSData class]] || data.length!=sizeof(request))return metal_queue_error(@"Resolve retained source/target owner or ABI differs");
    memcpy(&request,data.bytes,sizeof(request));auto src=[source bindingForIndex:0 stage:0],dst=[target bindingForIndex:0 stage:0];
    if(memcmp(&src,&request.source,sizeof(src)) || memcmp(&dst,&request.target,sizeof(dst)) || request.samples!=source.sampleCount ||
        request.reserved || request.aspect>1 || (request.aspect==1)!=(source.pixelFormat==MTLPixelFormatDepth32Float) ||
        request.filter>2 || (!request.aspect && request.filter) || multisample_textures_overlap(source,target))
        return metal_queue_error(@"Resolve sample source or target snapshot differs from original page");
    int result=sweetgl_metal_multisample_resolve(&request,sequence);
    return result?metal_queue_error([NSString stringWithFormat:@"GPU multisample resolve failed result=%d",result]):nil;
}
