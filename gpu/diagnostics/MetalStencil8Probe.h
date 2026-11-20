/* Original Stencil8 page plus optional Depth32: compares, operations, masks. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
#include "AttachmentPageSnapshot.h"
enum { STENCIL8_PHASES=99, STENCIL8_COLOR_BYTES=1280, STENCIL8_DEPTH_BYTES=1280, STENCIL8_BYTES=768 };
static unsigned stencil8_operation(unsigned op,unsigned original,unsigned reference)
{
    switch(op){case 0:return original;case 1:return 0;case 2:return reference;case 3:return original<255?original+1:255;
        case 4:return original?original-1:0;case 5:return original^255;case 6:return (original+1)&255;default:return (original-1)&255;}
}
static bool stencil8_compare(unsigned compare,unsigned reference,unsigned original)
{
    switch(compare){case 0:return false;case 1:return reference<original;case 2:return reference==original;case 3:return reference<=original;
        case 4:return reference>original;case 5:return reference!=original;case 6:return reference>=original;default:return compare==7;}
}
static void stencil8_expected(unsigned phase,unsigned char color[1280],unsigned char depth[1280],unsigned char stencil[768])
{
    static const unsigned initial[]={0,1,127,255},masks[]={0,255,15,255,255,240,255,255};
    memset(color,0xa5,1280);memset(depth,0xb6,1280);memset(stencil,0xc7,768);
    unsigned original=3,next=3;bool accepted=false;
    if(phase<48){unsigned index=phase%24;accepted=stencil8_compare(index/3,2+index%3,3);}
    else if(phase<96){unsigned index=(phase-48)%24,op=index%8,branch=index/8,back=(phase-48)/24;
        original=initial[(op+back)%4];unsigned mask=masks[op];next=(stencil8_operation(op,original,42)&mask)|(original&(~mask&255));accepted=branch==2;}
    else {original=phase==96?127:171;next=17;accepted=true;}
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){bool box=x>=3 && x<14 && y>=2 && y<7;unsigned at=64+y*128+x*4;
        const unsigned char yes[]={191,128,64,255},no[]={255,0,255,255};memcpy(color+at,accepted && box?yes:no,4);
        float z=phase==98?0.875f:0.5f;memcpy(depth+at,&z,4);
        stencil[64+y*64+x]=phase>=97?171:box?next:original;}
}
static uint64_t stencil8_hash(const unsigned char *bytes,unsigned count)
{uint64_t value=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)value=(value^bytes[i])*UINT64_C(1099511628211);return value;}
static bool metal_stencil8_probe(MetalABIEmit emit)
{
    emit("SWEET STENCIL8 BEGIN phases=99 width=17 height=9 color_bytes=1280 depth_bytes=1280 stencil_bytes=768 original_pages=1");
    unsigned char digest[32];CC_SHA256(sweet_air_render_fixture,(CC_LONG)sizeof(sweet_air_render_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET STENCIL8 INPUT fixture_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=device?[device newLibraryWithData:data error:&error]:nil;dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;
    pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;pd.stencilAttachmentPixelFormat=MTLPixelFormatStencil8;
    pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> combined=vertex && fragment?[device newRenderPipelineStateWithDescriptor:pd error:&error]:nil;
    pd.depthAttachmentPixelFormat=MTLPixelFormatInvalid;
    id<MTLRenderPipelineState> stencilOnly=vertex && fragment?[device newRenderPipelineStateWithDescriptor:pd error:&error]:nil;[pd release];
    id<MTLBuffer> cb=[device newBufferWithLength:1280 options:0],db=[device newBufferWithLength:1280 options:0],sb=[device newBufferWithLength:768 options:0];
    MTLTextureDescriptor *td=[MTLTextureDescriptor new];td.width=17;td.height=9;td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageRenderTarget;
    td.pixelFormat=MTLPixelFormatBGRA8Unorm;id<MTLTexture> color=[cb newTextureWithDescriptor:td offset:64 bytesPerRow:128];
    td.pixelFormat=MTLPixelFormatDepth32Float;id<MTLTexture> depth=[db newTextureWithDescriptor:td offset:64 bytesPerRow:128];
    td.pixelFormat=MTLPixelFormatStencil8;id<MTLTexture> stencil=[sb newTextureWithDescriptor:td offset:64 bytesPerRow:64];[td release];
    const float initial[12]={-1,1,0.25f,1,3,1,0.25f,1,-1,-3,0.25f,1};
    id<MTLBuffer> positions=[device newBufferWithBytes:initial length:sizeof(initial) options:0];id<MTLCommandQueue> queue=[device newCommandQueue];
    unsigned char *cp=(unsigned char *)cb.contents,*dp=(unsigned char *)db.contents,*sp=(unsigned char *)sb.contents;
    bool valid=device && combined && stencilOnly && positions && queue && color && depth && stencil && cp && dp && sp;uint32_t previous=0;unsigned completed=0;
    if(valid){memset(cp,0xa5,1280);memset(dp,0xb6,1280);memset(sp,0xc7,768);}else if(error)emit("SWEET STENCIL8 FAILURE create=1 reason=%s",error.localizedDescription.UTF8String);
    for(unsigned phase=0;phase<99 && valid;++phase){@autoreleasepool{
        static const unsigned originals[]={0,1,127,255},masks[]={0,255,15,255,255,240,255,255};
        unsigned index=phase<48?phase%24:(phase-48)%24,back=phase<48?phase/24:(phase-48)/24;
        unsigned op=index%8,branch=index/8,reference=phase<48?2+index%3+(back?16:0):phase<96?42:17;
        unsigned original=phase<48?3:phase<96?originals[(op+back)%4]:phase==96?127:171;
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=color;
        pass.colorAttachments[0].loadAction=phase<97?MTLLoadActionClear:MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,1,1);
        pass.depthAttachment.texture=phase==96?nil:depth;pass.depthAttachment.loadAction=phase<96 || phase==98?MTLLoadActionClear:MTLLoadActionLoad;
        pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=phase==98?0.875:0.5;
        pass.stencilAttachment.texture=stencil;pass.stencilAttachment.loadAction=phase==98?MTLLoadActionLoad:MTLLoadActionClear;
        pass.stencilAttachment.storeAction=MTLStoreActionStore;pass.stencilAttachment.clearStencil=original;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=commands && encoder;
        if(valid && phase<97){
            MTLDepthStencilDescriptor *sd=[MTLDepthStencilDescriptor new];sd.depthCompareFunction=phase>=48 && phase<96 && branch==1?MTLCompareFunctionNever:MTLCompareFunctionAlways;
            sd.depthWriteEnabled=NO;MTLStencilDescriptor *selected=back && phase<96?sd.backFaceStencil:sd.frontFaceStencil;
            selected.stencilCompareFunction=phase<48?(MTLCompareFunction)(index/3):phase<96 && branch==0?MTLCompareFunctionNever:MTLCompareFunctionAlways;
            selected.stencilFailureOperation=phase>=48 && phase<96 && branch==0?(MTLStencilOperation)op:MTLStencilOperationKeep;
            selected.depthFailureOperation=phase>=48 && phase<96 && branch==1?(MTLStencilOperation)op:MTLStencilOperationKeep;
            selected.depthStencilPassOperation=phase==96?MTLStencilOperationReplace:phase>=48 && branch==2?(MTLStencilOperation)op:MTLStencilOperationKeep;
            selected.readMask=phase>=24 && phase<48?15:255;selected.writeMask=phase<48?0:phase<96?masks[op]:255;
            id<MTLDepthStencilState> state=[device newDepthStencilStateWithDescriptor:sd];[sd release];valid=state!=nil;
            if(valid){[encoder setRenderPipelineState:phase==96?stencilOnly:combined];[encoder setDepthStencilState:state];
                [encoder setStencilFrontReferenceValue:reference backReferenceValue:reference];[encoder setVertexBuffer:positions offset:0 atIndex:0];
                [encoder setFrontFacingWinding:back && phase<96?MTLWindingCounterClockwise:MTLWindingClockwise];
                [encoder setScissorRect:(MTLScissorRect){3,2,11,5}];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}[state release];
        }
        if(valid){[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && commands.status==MTLCommandBufferStatusCompleted && !commands.error &&
            SweetMTLCommandResult(commands,&sequence,&operations) && operations==1 && sequence>previous && cb.contents==cp && db.contents==dp && sb.contents==sp;
        unsigned mismatches=0,read_mismatches=0;unsigned char ec[1280],ed[1280],es[768],rd[900],rs[300];stencil8_expected(phase,ec,ed,es);memset(rd,0xe7,900);memset(rs,0xd8,300);
        if(valid){for(unsigned i=0;i<1280;++i)mismatches+=(cp[i]!=ec[i])+(dp[i]!=ed[i]);for(unsigned i=0;i<768;++i)mismatches+=sp[i]!=es[i];
            [depth getBytes:rd+8 bytesPerRow:96 fromRegion:MTLRegionMake2D(0,0,17,9) mipmapLevel:0];
            [stencil getBytes:rs+8 bytesPerRow:32 fromRegion:MTLRegionMake2D(0,0,17,9) mipmapLevel:0];
            for(unsigned i=0;i<900;++i){unsigned char wanted=0xe7;if(i>=8){unsigned y=(i-8)/96,x=(i-8)%96;if(y<9 && x<68)wanted=ed[64+y*128+x];}read_mismatches+=rd[i]!=wanted;}
            for(unsigned i=0;i<300;++i){unsigned char wanted=0xd8;if(i>=8){unsigned y=(i-8)/32,x=(i-8)%32;if(y<9 && x<17)wanted=es[64+y*64+x];}read_mismatches+=rs[i]!=wanted;}}
        uint64_t ch=valid?stencil8_hash(cp,1280):0,dh=valid?stencil8_hash(dp,1280):0,sh=valid?stencil8_hash(sp,768):0;
        if(valid && (mismatches || read_mismatches)){
            attachment_page_snapshot(emit,"stencil8",phase,0,cp,1280,cb.gpuAddress,sequence);
            attachment_page_snapshot(emit,"stencil8",phase,8,dp,1280,db.gpuAddress,sequence);
            attachment_page_snapshot(emit,"stencil8",phase,9,sp,768,sb.gpuAddress,sequence);
        }
        valid=valid && !mismatches && !read_mismatches;
        emit("SWEET STENCIL8 PHASE index=%u valid=%u sequence=%u operations=%u mismatches=%u read_mismatches=%u",phase,valid,sequence,operations,mismatches,read_mismatches);
        emit("SWEET STENCIL8 HASH index=%u color=%016llx depth=%016llx stencil=%016llx original_pages=%u",phase,(unsigned long long)ch,(unsigned long long)dh,(unsigned long long)sh,cb.contents==cp && db.contents==dp && sb.contents==sp);
        if(!valid && commands.error)emit("SWEET STENCIL8 FAILURE phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);
        if(valid){++completed;previous=sequence;}[commands release];
    }}
    [queue release];[positions release];[color release];[depth release];[stencil release];[cb release];[db release];[sb release];[combined release];[stencilOnly release];[vertex release];[fragment release];[library release];
    NSUInteger final=device.currentAllocatedSize;valid=valid && completed==99 && !final;
    emit("SWEET STENCIL8 END result=%u phases=%u checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=%lu GPU_only=1",valid?0:1,completed,(unsigned long)final);
    [device release];return valid;
}
