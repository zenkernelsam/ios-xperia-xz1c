// SPDX-License-Identifier: BSD-2-Clause
// Transient attachment semantics follow ios_sagit/QcomMTL (898998cf).
// Public textures own metadata; each encoder owns independent private pages.
#include <atomic>
static std::atomic<uint64_t> memoryless_pass_ids{1};
static MTLRenderPassAttachmentDescriptor *memoryless_attachment(MTLRenderPassDescriptor *pass,unsigned slot)
{return slot<8?pass.colorAttachments[slot]:slot==8?pass.depthAttachment:pass.stencilAttachment;}
static MTLRenderPassDescriptor *memoryless_pass_descriptor(SweetMTLCommandBuffer *commands,MTLRenderPassDescriptor *input,NSArray **bindings)
{
    @autoreleasepool{
    if(!commands || !input || !bindings)return nil;*bindings=nil;
    NSMutableArray *originals=[NSMutableArray new];bool valid=true;
    for(unsigned slot=0;slot<10 && valid;++slot){auto attachment=memoryless_attachment(input,slot);auto texture=(SweetMTLTexture *)attachment.texture;
        if(![texture isKindOfClass:[SweetMTLTexture class]] || !texture->_memorylessOwner)continue;
        bool resolve=texture.sampleCount>1 && slot<9 && attachment.storeAction==MTLStoreActionMultisampleResolve;
        valid=texture.device==commands.device && attachment.loadAction!=MTLLoadActionLoad && attachment.loadAction<=MTLLoadActionClear &&
            (attachment.storeAction==MTLStoreActionDontCare || resolve) && ((attachment.resolveTexture!=nil)==resolve) &&
            !attachment.level && !attachment.slice && !attachment.depthPlane;
        for(id prior in originals)if(prior==texture)valid=false;
        [originals addObject:texture];
    }
    if(!valid){[originals release];return nil;}[originals release];
    auto pass=[input copy];NSMutableArray *entries=[NSMutableArray new];uint64_t identity=0;
    for(unsigned slot=0;slot<10 && valid;++slot){auto attachment=memoryless_attachment(pass,slot);auto texture=(SweetMTLTexture *)attachment.texture;
        if(![texture isKindOfClass:[SweetMTLTexture class]] || !texture->_memorylessOwner)continue;
        auto backing=[texture newMemorylessPassBackingForDevice:commands->_queue->_device];valid=backing!=nil;
        if(!valid)break;if(!identity)identity=memoryless_pass_ids.fetch_add(1,std::memory_order_relaxed);
        if(!identity){[backing release];valid=false;break;}
        [entries addObject:@{@"slot":@(slot),@"public":texture,@"private":backing,@"pass":@(identity)}];
        attachment.texture=backing;[backing release];
    }
    if(valid)*bindings=[entries copy];[entries release];
    if(!valid){[pass release];return nil;}return pass;}
}
static bool memoryless_operation_valid(NSDictionary *operation,id<MTLDevice> device)
{
    NSArray *entries=operation[@"memoryless"];if(![entries isKindOfClass:[NSArray class]] || entries.count>10)return false;
    NSArray *targets=operation[@"targets"];NSData *colorData=operation[@"pass"],*depthData=operation[@"depthPass"],*stencilData=operation[@"stencilPass"];
    if(![targets isKindOfClass:[NSArray class]] || targets.count!=8 || colorData.length!=sizeof(SweetRenderPass) ||
        depthData.length!=sizeof(SweetRenderDepthPass) || stencilData.length!=sizeof(SweetRenderStencilPass))return false;
    SweetRenderPass color={};SweetRenderDepthPass depth={};SweetRenderStencilPass stencil={};
    memcpy(&color,colorData.bytes,sizeof(color));memcpy(&depth,depthData.bytes,sizeof(depth));memcpy(&stencil,stencilData.bytes,sizeof(stencil));
    uint32_t seen=0;uint64_t identity=0;
    for(NSDictionary *entry in entries){unsigned slot=[entry[@"slot"] unsignedIntValue];
        auto original=(SweetMTLTexture *)entry[@"public"],backing=(SweetMTLTexture *)entry[@"private"];
        uint64_t current=[entry[@"pass"] unsignedLongLongValue];
        if(slot>9 || (seen&(1u<<slot)) || !current || (identity && current!=identity) ||
            ![original isKindOfClass:[SweetMTLTexture class]] || ![backing isKindOfClass:[SweetMTLTexture class]] ||
            !original->_memorylessOwner || backing->_memorylessOwner || original.device!=device || backing.device!=device ||
            backing.storageMode!=MTLStorageModePrivate || original.pixelFormat!=backing.pixelFormat ||
            original.width!=backing.width || original.height!=backing.height || original.sampleCount!=backing.sampleCount || !backing->_buffer || backing->_layout.offset!=64)return false;
        id actual=slot<8?targets[slot]:operation[slot==8?@"depthTarget":@"stencilTarget"];
        unsigned store=slot<8?sweet_render_attachment(&color,slot).store_action:slot==8?depth.store_action:stencil.store_action;
        if(store!=MTLStoreActionDontCare)return false;
        if(actual!=backing)return false;seen|=1u<<slot;identity=current;
    }return true;
}
