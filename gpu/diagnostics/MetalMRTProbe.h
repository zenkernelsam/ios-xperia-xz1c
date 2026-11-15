/* Simultaneous raw targets, independent blends/masks and original XR bit preservation. */
#pragma once
#include "MRTProbeValues.h"
#include <CommonCrypto/CommonDigest.h>
static MTLTextureDescriptor *mrt_texture(unsigned format)
{MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=17;d.height=9;d.pixelFormat=(MTLPixelFormat)format;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;return d;}
static bool metal_mrt_probe(MetalABIEmit emit)
{
    emit("SWEET METAL MRT begin XR_formats=552,553,554,555 dual_targets=2 phases=4 eight_targets=8 original_pages=1");
    unsigned char digest[32];CC_SHA256(sweet_air_mrt_fixture,(CC_LONG)sizeof(sweet_air_mrt_fixture),digest);char hex[65]={0};for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);
    emit("SWEET METAL MRT input library_sha256=%s",hex);NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_mrt_fixture,sizeof(sweet_air_mrt_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"mrt_position"],color=[library newFunctionWithName:@"mrt_colour"],changed=[library newFunctionWithName:@"mrt_changed"],sample=[library newFunctionWithName:@"mrt_sample"],eight=[library newFunctionWithName:@"mrt_eight"];
    id<MTLFunction> orderVertex=[library newFunctionWithName:@"mrt_order_position"],orderColor=[library newFunctionWithName:@"mrt_order_colour"];
    const float coords[12]={-1,1,.5f,1,3,1,.5f,1,-1,-3,.5f,1};id<MTLBuffer> vertices=[device newBufferWithBytes:coords length:sizeof(coords) options:0];id<MTLCommandQueue> queue=[device newCommandQueue];
    MTLSamplerDescriptor *filter=[MTLSamplerDescriptor new];id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:filter];[filter release];
    bool valid=device && library && vertex && color && changed && sample && eight && orderVertex && orderColor && vertices && queue && sampler;unsigned passed=0,ordered=0;uint32_t previous=0;
    for(unsigned format=552;format<=555 && valid;++format){@autoreleasepool{
        MTLRenderPipelineDescriptor *p=[MTLRenderPipelineDescriptor new];p.vertexFunction=vertex;p.fragmentFunction=color;
        p.colorAttachments[0].pixelFormat=(MTLPixelFormat)format;p.colorAttachments[1].pixelFormat=MTLPixelFormatBGRA8Unorm;
        p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorSourceAlpha;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
        p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;
        p.colorAttachments[1].writeMask=MTLColorWriteMaskGreen;
        id<MTLRenderPipelineState> alpha=[device newRenderPipelineStateWithDescriptor:p error:&error];p.fragmentFunction=changed;
        p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorSourceAlpha;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOne;p.colorAttachments[0].writeMask=MTLColorWriteMaskRed|MTLColorWriteMaskBlue;
        p.colorAttachments[1].blendingEnabled=YES;p.colorAttachments[1].sourceRGBBlendFactor=MTLBlendFactorOne;p.colorAttachments[1].destinationRGBBlendFactor=MTLBlendFactorZero;p.colorAttachments[1].writeMask=MTLColorWriteMaskGreen|MTLColorWriteMaskBlue;
        id<MTLRenderPipelineState> masked=[device newRenderPipelineStateWithDescriptor:p error:&error];
        p.fragmentFunction=sample;p.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;p.colorAttachments[1].pixelFormat=MTLPixelFormatRGBA8Unorm;
        p.colorAttachments[0].blendingEnabled=p.colorAttachments[1].blendingEnabled=NO;p.colorAttachments[0].writeMask=p.colorAttachments[1].writeMask=MTLColorWriteMaskAll;
        id<MTLRenderPipelineState> sampled=[device newRenderPipelineStateWithDescriptor:p error:&error];[p release];
        id<MTLBuffer> raw[4]={[device newBufferWithLength:1920 options:0],[device newBufferWithLength:1344 options:0],[device newBufferWithLength:1344 options:0],[device newBufferWithLength:1344 options:0]};
        MTLTextureDescriptor *d=mrt_texture(format);id<MTLTexture> t[4]={[raw[0] newTextureWithDescriptor:d offset:64 bytesPerRow:format<554?192:128],nil,nil,nil};
        d.pixelFormat=MTLPixelFormatBGRA8Unorm;t[1]=[raw[1] newTextureWithDescriptor:d offset:128 bytesPerRow:128];t[2]=[raw[2] newTextureWithDescriptor:d offset:128 bytesPerRow:128];
        d.pixelFormat=MTLPixelFormatRGBA8Unorm;t[3]=[raw[3] newTextureWithDescriptor:d offset:128 bytesPerRow:128];[d release];
        unsigned char *original[4]={raw[0].contents,raw[1].contents,raw[2].contents,raw[3].contents};valid=valid && alpha && masked && sampled;
        for(unsigned i=0;i<4 && valid;++i)valid=raw[i] && t[i] && original[i] && raw[i].allocatedSize==16384;
        if(!valid && error)emit("SWEET METAL MRT failure format=%u phase=create reason=%s",format,error.localizedDescription.UTF8String);
        if(valid){memset(original[0],0xa5,16384);for(unsigned i=1;i<4;++i)memset(original[i],0xb6,16384);}
        for(unsigned phase=0;phase<4 && valid;++phase){if(phase==1)mrt_seed_old_codes(original[0],format);
            MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=phase==3?t[2]:t[0];pass.colorAttachments[1].texture=phase==3?t[3]:t[1];
            for(unsigned i=0;i<2;++i){pass.colorAttachments[i].storeAction=MTLStoreActionStore;pass.colorAttachments[i].loadAction=phase?MTLLoadActionLoad:MTLLoadActionClear;}
            pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,1,1);pass.colorAttachments[1].clearColor=MTLClearColorMake(1,0,0,1);
            id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=valid && commands && encoder;
            if(valid && phase){[encoder setRenderPipelineState:phase==1?alpha:phase==2?masked:sampled];[encoder setVertexBuffer:vertices offset:0 atIndex:0];
                if(phase<3)[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];else{[encoder setFragmentTexture:t[0] atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];}
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}
            if(valid){[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
            uint32_t sequence=0,ops=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&ops) && sequence>previous && ops==1 && commands.status==MTLCommandBufferStatusCompleted && !commands.error;previous=sequence;
            for(unsigned i=0;i<4 && valid;++i)valid=raw[i].contents==original[i];unsigned mismatch=0;
            if(valid){mismatch=mrt_check(original[0],format,phase==3?2:phase,0,false)+mrt_check(original[1],format,phase==3?2:phase,1,false);
                if(phase==3)mismatch+=mrt_check(original[2],format,phase,0,true)+mrt_check(original[3],format,phase,1,true);
                else for(unsigned i=0;i<16384;++i)mismatch+=(original[2][i]!=0xb6)+(original[3][i]!=0xb6);}
            valid=valid && !mismatch;emit("SWEET METAL MRT phase format=%u phase=%u sequence=%u operations=%u status=%lu error=%u checked_bytes=65536 mismatches=%u valid=%u",format,phase,sequence,ops,(unsigned long)commands.status,commands.error!=nil,mismatch,valid);
            for(unsigned i=0;i<4;++i)emit("SWEET METAL MRT buffer format=%u phase=%u index=%u hash=%016llx original_pages=%u",format,phase,i,(unsigned long long)(valid?mrt_hash(original[i],16384):0),raw[i].contents==original[i]);
            if(!valid && commands.error)emit("SWEET METAL MRT failure format=%u phase=%u reason=%s",format,phase,commands.error.localizedDescription.UTF8String);[commands release];
        }
        if(valid)++passed;[alpha release];[masked release];[sampled release];for(unsigned i=0;i<4;++i){[t[i] release];[raw[i] release];}
    }}
    if(valid){@autoreleasepool{
        MTLRenderPipelineDescriptor *p=[MTLRenderPipelineDescriptor new];p.vertexFunction=vertex;p.fragmentFunction=eight;
        id<MTLBuffer> raw[8];id<MTLTexture> targets[8];unsigned char *original[8];
        for(unsigned i=0;i<8;++i){unsigned format=i&1?MTLPixelFormatRGBA8Unorm:MTLPixelFormatBGRA8Unorm;p.colorAttachments[i].pixelFormat=(MTLPixelFormat)format;
            raw[i]=[device newBufferWithLength:1280 options:0];MTLTextureDescriptor *d=mrt_texture(format);targets[i]=[raw[i] newTextureWithDescriptor:d offset:64 bytesPerRow:128];[d release];original[i]=raw[i].contents;valid=valid && targets[i] && original[i];if(original[i])memset(original[i],0xa0+i,16384);}
        id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:p error:&error];[p release];valid=valid && pipeline;
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];for(unsigned i=0;i<8;++i){pass.colorAttachments[i].texture=targets[i];pass.colorAttachments[i].loadAction=MTLLoadActionLoad;pass.colorAttachments[i].storeAction=MTLStoreActionStore;}
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=valid && encoder && commands;
        if(valid){[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,ops=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&ops) && sequence>previous && ops==1 && !commands.error;previous=sequence;unsigned mismatch=0;
        for(unsigned i=0;i<8 && valid;++i){valid=raw[i].contents==original[i];mismatch+=mrt_eight_check(original[i],i);}valid=valid && !mismatch;
        emit("SWEET METAL MRT eight sequence=%u operations=%u checked_bytes=131072 mismatches=%u valid=%u",sequence,ops,mismatch,valid);
        for(unsigned i=0;i<8;++i)emit("SWEET METAL MRT eight_buffer index=%u hash=%016llx original_pages=%u",i,(unsigned long long)(valid?mrt_hash(original[i],16384):0),raw[i].contents==original[i]);
        [commands release];[pipeline release];for(unsigned i=0;i<8;++i){[targets[i] release];[raw[i] release];}
    }}
    float orderCoords[36];for(unsigned i=0;i<3;++i)memcpy(orderCoords+i*12,coords,sizeof(coords));
    id<MTLBuffer> orderVertices=valid?[device newBufferWithBytes:orderCoords length:sizeof(orderCoords) options:0]:nil;valid=valid && orderVertices;
    for(unsigned format=552;format<=555 && valid;++format){@autoreleasepool{
        MTLRenderPipelineDescriptor *p=[MTLRenderPipelineDescriptor new];p.vertexFunction=orderVertex;p.fragmentFunction=orderColor;p.colorAttachments[0].pixelFormat=(MTLPixelFormat)format;
        p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorSourceAlpha;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
        p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;p.colorAttachments[0].writeMask=MTLColorWriteMaskRed|MTLColorWriteMaskGreen|MTLColorWriteMaskBlue;
        id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:p error:&error];[p release];
        id<MTLBuffer> raw=[device newBufferWithLength:1920 options:0];MTLTextureDescriptor *d=mrt_texture(format);id<MTLTexture> target=[raw newTextureWithDescriptor:d offset:64 bytesPerRow:format<554?192:128];[d release];
        unsigned char *original=raw.contents;valid=valid && pipeline && target && original && raw.allocatedSize==16384;
        if(valid){memset(original,0xa5,16384);mrt_seed_old_codes(original,format);}
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=valid?[commands renderCommandEncoderWithDescriptor:pass]:nil;[pass release];valid=valid && commands && encoder;
        if(valid){[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:orderVertices offset:0 atIndex:0];[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:3 vertexCount:6 instanceCount:2];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,ops=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&ops) && sequence>previous && ops==1 && !commands.error && commands.status==MTLCommandBufferStatusCompleted;previous=sequence;
        unsigned mismatch=valid?mrt_order_check(original,format):0;valid=valid && !mismatch && raw.contents==original && orderVertices.contents && !memcmp(orderVertices.contents,orderCoords,sizeof(orderCoords));
        emit("SWEET METAL MRT order format=%u first=3 count=6 instances=2 primitives=4 sequence=%u operations=%u checked_bytes=16384 mismatches=%u valid=%u hash=%016llx original_pages=%u HDR_negative=1 HDR_above_one=1",
            format,sequence,ops,mismatch,valid,(unsigned long long)(valid?mrt_hash(original,16384):0),raw.contents==original);
        if(!valid && commands.error)emit("SWEET METAL MRT failure format=%u phase=order reason=%s",format,commands.error.localizedDescription.UTF8String);
        if(valid)++ordered;[commands release];[pipeline release];[target release];[raw release];
    }}
    [orderVertices release];[vertices release];[queue release];[sampler release];[vertex release];[color release];[changed release];[sample release];[eight release];[orderVertex release];[orderColor release];[library release];NSUInteger final=device.currentAllocatedSize;valid=valid && passed==4 && ordered==4 && !final;
    emit("SWEET METAL MRT END result=%u XR_formats=%u phases=16 eight_targets=8 ordered_formats=%u checked_bytes=1245184 final_allocated=%lu GPU_only=1",valid?0:1,passed,ordered,(unsigned long)final);[device release];return valid;
}
