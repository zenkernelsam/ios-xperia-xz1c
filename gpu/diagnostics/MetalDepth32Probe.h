/* Original Depth32Float pages: all compares, writes, Load/Clear, guard bytes. */
#pragma once
#include <CommonCrypto/CommonDigest.h>
#include "AttachmentPageSnapshot.h"
enum { DEPTH32_PHASES=29, DEPTH32_BYTES=1280, DEPTH32_READ_BYTES=900 };
static bool depth32_compare(unsigned compare,float incoming,float old)
{
    switch(compare){case 0:return false;case 1:return incoming<old;case 2:return incoming==old;case 3:return incoming<=old;
    case 4:return incoming>old;case 5:return incoming!=old;case 6:return incoming>=old;default:return compare==7;}
}
static void depth32_expected(unsigned phase,unsigned char color[DEPTH32_BYTES],unsigned char depth[DEPTH32_BYTES])
{
    memset(color,0xa5,DEPTH32_BYTES);memset(depth,0xb6,DEPTH32_BYTES);
    bool passed=phase<24?depth32_compare(phase/3,(phase%3+1)*0.25f,0.5f):phase==24 || phase==26;
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){bool box=x>=3 && x<14 && y>=2 && y<7;unsigned at=64+y*128+x*4;
        const unsigned char yes[]={191,128,64,255},no[]={255,0,255,255};memcpy(color+at,box && passed?yes:no,4);
        float value=phase<24?0.5f:phase==28?0.875f:box?0.25f:0.75f;memcpy(depth+at,&value,4);}
}
static uint64_t depth32_hash(const unsigned char *bytes,unsigned count)
{uint64_t result=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)result=(result^bytes[i])*UINT64_C(1099511628211);return result;}
static bool metal_depth32_probe(MetalABIEmit emit)
{
    emit("SWEET DEPTH32 BEGIN phases=29 width=17 height=9 bytes=1280 offset=64 pitch=128 original_pages=1");
    unsigned char digest[32];CC_SHA256(sweet_air_render_fixture,(CC_LONG)sizeof(sweet_air_render_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET DEPTH32 INPUT fixture_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=device?[device newLibraryWithData:data error:&error]:nil;dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    MTLRenderPipelineDescriptor *pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=vertex;pd.fragmentFunction=fragment;
    pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> pipeline=vertex && fragment?[device newRenderPipelineStateWithDescriptor:pd error:&error]:nil;[pd release];
    id<MTLBuffer> cb=[device newBufferWithLength:1280 options:0],db=[device newBufferWithLength:1280 options:0];
    MTLTextureDescriptor *td=[MTLTextureDescriptor new];td.width=17;td.height=9;td.storageMode=MTLStorageModeShared;
    td.pixelFormat=MTLPixelFormatBGRA8Unorm;td.usage=MTLTextureUsageRenderTarget;
    id<MTLTexture> color=[cb newTextureWithDescriptor:td offset:64 bytesPerRow:128];td.pixelFormat=MTLPixelFormatDepth32Float;
    id<MTLTexture> depth=[db newTextureWithDescriptor:td offset:64 bytesPerRow:128];[td release];
    const float initial[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};
    id<MTLBuffer> positions=[device newBufferWithBytes:initial length:sizeof(initial) options:0];id<MTLCommandQueue> queue=[device newCommandQueue];
    unsigned char *cp=(unsigned char *)cb.contents,*dp=(unsigned char *)db.contents;
    bool valid=device && pipeline && color && depth && positions && queue && cp && dp;
    if(valid){memset(cp,0xa5,1280);memset(dp,0xb6,1280);}uint32_t previous=0;unsigned completed=0;
    if(!valid && error)emit("SWEET DEPTH32 FAILURE create=1 reason=%s",error.localizedDescription.UTF8String);
    for(unsigned phase=0;phase<DEPTH32_PHASES && valid;++phase){@autoreleasepool{
        float z=phase<24?(phase%3+1)*0.25f:phase==24?0.25f:phase==25?0.5f:phase==26?0.875f:0.125f;
        float *bytes=(float *)positions.contents;valid=bytes!=NULL;if(!valid)break;
        for(unsigned i=0;i<3;++i)bytes[i*4+2]=z;
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=color;
        pass.colorAttachments[0].loadAction=phase==28?MTLLoadActionLoad:MTLLoadActionClear;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,1,1);
        pass.depthAttachment.texture=depth;pass.depthAttachment.storeAction=MTLStoreActionStore;
        pass.depthAttachment.loadAction=phase<25 || phase==28?MTLLoadActionClear:MTLLoadActionLoad;
        pass.depthAttachment.clearDepth=phase<24?0.5:phase==24?0.75:0.875;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];
        valid=commands && encoder;
        if(valid && phase!=28){
            MTLDepthStencilDescriptor *sd=[MTLDepthStencilDescriptor new];sd.depthCompareFunction=(MTLCompareFunction)(phase<24?phase/3:phase==24 || phase==25?1:phase==27?0:7);
            sd.depthWriteEnabled=phase==24 || phase==27;id<MTLDepthStencilState> state=[device newDepthStencilStateWithDescriptor:sd];[sd release];valid=state!=nil;
            if(valid){[encoder setRenderPipelineState:pipeline];[encoder setDepthStencilState:phase==26?nil:state];
                [encoder setVertexBuffer:positions offset:0 atIndex:0];[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}[state release];
        }
        if(valid){[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && commands.status==MTLCommandBufferStatusCompleted && !commands.error &&
            SweetMTLCommandResult(commands,&sequence,&operations) && operations==1 && sequence>previous && cb.contents==cp && db.contents==dp;
        unsigned mismatches=0,read_mismatches=0;unsigned char expectedColor[1280],expectedDepth[1280],output[900];
        depth32_expected(phase,expectedColor,expectedDepth);memset(output,0xe7,sizeof(output));
        if(valid){for(unsigned i=0;i<1280;++i)mismatches+=(cp[i]!=expectedColor[i])+(dp[i]!=expectedDepth[i]);
            [depth getBytes:output+8 bytesPerRow:96 fromRegion:MTLRegionMake2D(0,0,17,9) mipmapLevel:0];
            for(unsigned i=0;i<900;++i){unsigned char wanted=0xe7;
                if(i>=8){unsigned y=(i-8)/96,x=(i-8)%96;if(y<9 && x<68)wanted=expectedDepth[64+y*128+x];}read_mismatches+=output[i]!=wanted;}}
        uint64_t colorHash=valid?depth32_hash(cp,1280):0,depthHash=valid?depth32_hash(dp,1280):0;
        if(valid && (mismatches || read_mismatches)){
            attachment_page_snapshot(emit,"depth32",phase,0,cp,1280,cb.gpuAddress,sequence);
            attachment_page_snapshot(emit,"depth32",phase,8,dp,1280,db.gpuAddress,sequence);
        }
        valid=valid && !mismatches && !read_mismatches;
        emit("SWEET DEPTH32 PHASE index=%u valid=%u sequence=%u operations=%u mismatches=%u read_mismatches=%u",phase,valid,sequence,operations,mismatches,read_mismatches);
        emit("SWEET DEPTH32 HASH index=%u color=%016llx depth=%016llx original_pages=%u",phase,(unsigned long long)colorHash,(unsigned long long)depthHash,cb.contents==cp && db.contents==dp);
        if(!valid && commands.error)emit("SWEET DEPTH32 FAILURE phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);
        if(valid){++completed;previous=sequence;}[commands release];
    }}
    [queue release];[positions release];[color release];[depth release];[cb release];[db release];[pipeline release];[vertex release];[fragment release];[library release];
    NSUInteger final=device.currentAllocatedSize;valid=valid && completed==29 && !final;
    emit("SWEET DEPTH32 END result=%u phases=%u checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=%lu GPU_only=1",valid?0:1,completed,(unsigned long)final);
    [device release];return valid;
}
