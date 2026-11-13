/* Actual AIR render on original Metal texture pages, including every guard. */
#pragma once
#include <stdatomic.h>
#include <CommonCrypto/CommonDigest.h>
static void render_sample_pixel(unsigned x,unsigned y,unsigned phase,unsigned char color[4])
{color[0]=(unsigned char)(x*29+y*7+phase*13);color[1]=(unsigned char)(x*5+y*31+phase*17);color[2]=(unsigned char)(x*19+y*11+phase*23);color[3]=255;}
static void render_target_pixel(unsigned x,unsigned y,unsigned phase,unsigned char bgra[4])
{
    if(phase==0 || (phase==1 && !(x>=5 && x<28 && y>=4 && y<15))){bgra[0]=0;bgra[1]=0;bgra[2]=255;bgra[3]=255;return;}
    if(phase==1){bgra[0]=191;bgra[1]=128;bgra[2]=64;bgra[3]=255;return;}
    unsigned samplePhase=phase==3 && x>=3 && x<26 && y>=4 && y<15;
    unsigned sx=((2*x+1)*7)/66,sy=((2*y+1)*5)/38;unsigned char rgba[4];render_sample_pixel(sx,sy,samplePhase,rgba);
    bgra[0]=rgba[2];bgra[1]=rgba[1];bgra[2]=rgba[0];bgra[3]=rgba[3];
}
static uint64_t render_bytes_hash(const unsigned char *bytes,unsigned count)
{uint64_t value=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)value=(value^bytes[i])*UINT64_C(1099511628211);return value;}
static unsigned render_compare_target(const unsigned char *bytes,unsigned phase)
{
    unsigned mismatches=0;
    for(unsigned i=0;i<3776;++i){unsigned char expected=0xa5;
        if(i>=64 && i<3712){unsigned row=(i-64)/192,column=(i-64)%192;
            if(column<132){unsigned char bgra[4];render_target_pixel(column/4,row,phase,bgra);expected=bgra[column%4];}}
        mismatches+=bytes[i]!=expected;
    }
    return mismatches;
}
static unsigned render_compare_sample(const unsigned char *bytes,unsigned phase)
{
    unsigned mismatches=0;
    for(unsigned i=0;i<512;++i){unsigned char expected=0xb6;
        if(i>=128 && i<448){unsigned row=(i-128)/64,column=(i-128)%64;
            if(column<28){unsigned char rgba[4];render_sample_pixel(column/4,row,phase,rgba);expected=rgba[column%4];}}
        mismatches+=bytes[i]!=expected;
    }
    return mismatches;
}
static void render_upload_sample(id<MTLTexture> texture,unsigned phase)
{
    unsigned char upload[200];memset(upload,0xd4,sizeof(upload));
    for(unsigned y=0;y<5;++y)for(unsigned x=0;x<7;++x)render_sample_pixel(x,y,phase,upload+y*40+x*4);
    [texture replaceRegion:MTLRegionMake2D(0,0,7,5) mipmapLevel:0 withBytes:upload bytesPerRow:40];
}
static bool metal_render_probe(MetalABIEmit emit)
{
    emit("SWEET AIR RENDER begin library_bytes=%lu standard_MTLB=1 standard_render_encoder=1 original_textures=1",(unsigned long)sizeof(sweet_air_render_fixture));
    emit("SWEET AIR RENDER contract target_bytes=3776 sample_bytes=512 target_pixels=627 target_offset=64 target_pitch=192 sample_offset=128 sample_pitch=64");
    unsigned char digest[32];CC_SHA256(sweet_air_render_fixture,(CC_LONG)sizeof(sweet_air_render_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);emit("SWEET AIR RENDER input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=device?[device newLibraryWithData:data error:&error]:nil;dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    id<MTLFunction> textureVertex=[library newFunctionWithName:@"texture_position"],textureFragment=[library newFunctionWithName:@"textured"];
    MTLRenderPipelineDescriptor *descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=vertex;descriptor.fragmentFunction=fragment;
    descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> solid=vertex && fragment?[device newRenderPipelineStateWithDescriptor:descriptor error:&error]:nil;
    descriptor.vertexFunction=textureVertex;descriptor.fragmentFunction=textureFragment;
    id<MTLRenderPipelineState> sampled=textureVertex && textureFragment?[device newRenderPipelineStateWithDescriptor:descriptor error:&error]:nil;[descriptor release];
    MTLTextureDescriptor *textureDescriptor=[MTLTextureDescriptor new];textureDescriptor.textureType=MTLTextureType2D;textureDescriptor.width=33;textureDescriptor.height=19;
    textureDescriptor.storageMode=MTLStorageModeShared;textureDescriptor.pixelFormat=MTLPixelFormatBGRA8Unorm;textureDescriptor.usage=MTLTextureUsageRenderTarget;
    id<MTLBuffer> targetBuffer=[device newBufferWithLength:3776 options:0],sampleBuffer=[device newBufferWithLength:512 options:0];
    id<MTLTexture> target=[targetBuffer newTextureWithDescriptor:textureDescriptor offset:64 bytesPerRow:192];
    textureDescriptor.width=7;textureDescriptor.height=5;textureDescriptor.pixelFormat=MTLPixelFormatRGBA8Unorm;textureDescriptor.usage=MTLTextureUsageShaderRead;
    id<MTLTexture> source=[sampleBuffer newTextureWithDescriptor:textureDescriptor offset:128 bytesPerRow:64];[textureDescriptor release];
    MTLSamplerDescriptor *samplerDescriptor=[MTLSamplerDescriptor new];samplerDescriptor.minFilter=MTLSamplerMinMagFilterNearest;samplerDescriptor.magFilter=MTLSamplerMinMagFilterNearest;
    id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:samplerDescriptor];[samplerDescriptor release];
    const float positions[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};
    const float textured[24]={-1,1,0.5f,1,0,0,0,0,3,1,0.5f,1,14,0,0,0,-1,-3,0.5f,1,0,10,0,0};
    id<MTLBuffer> positionBuffer=[device newBufferWithBytes:positions length:sizeof(positions) options:0];
    id<MTLBuffer> textureBuffer=[device newBufferWithBytes:textured length:sizeof(textured) options:0];
    id<MTLCommandQueue> queue=[device newCommandQueueWithMaxCommandBufferCount:8];
    unsigned char *originalTarget=(unsigned char *)targetBuffer.contents,*originalSample=(unsigned char *)sampleBuffer.contents;
    bool valid=device && library && solid && sampled && queue && target && source && sampler && positionBuffer && textureBuffer && originalTarget && originalSample;
    if(!valid && error)emit("SWEET AIR RENDER failure phase=create reason=%s",error.localizedDescription.UTF8String);
    if(valid){memset(originalTarget,0xa5,3776);memset(originalSample,0xb6,512);render_upload_sample(source,0);}
    dispatch_semaphore_t callbacks=dispatch_semaphore_create(0);_Atomic unsigned completed=0;_Atomic unsigned *callbackCount=&completed;
    uint32_t sequences[4]={0};
    for(unsigned phase=0;phase<4 && valid;++phase){
        if(phase==3)render_upload_sample(source,1);
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.colorAttachments[0].loadAction=phase==0?MTLLoadActionClear:MTLLoadActionLoad;pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,0,1);
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];
        valid=commands && encoder;
        if(valid && phase){[encoder setRenderPipelineState:phase==1?solid:sampled];[encoder setVertexBuffer:phase==1?positionBuffer:textureBuffer offset:0 atIndex:0];
            [encoder setCullMode:MTLCullModeBack];[encoder setFrontFacingWinding:MTLWindingClockwise];
            if(phase==1){[encoder setViewport:(MTLViewport){3,2,27,15,0,1}];[encoder setScissorRect:(MTLScissorRect){5,4,23,11}];}
            if(phase>=2){[encoder setFragmentTexture:source atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];}
            if(phase==3)[encoder setScissorRect:(MTLScissorRect){3,4,23,11}];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}
        if(valid){[encoder endEncoding];[commands addCompletedHandler:^(id<MTLCommandBuffer> c){(void)c;atomic_fetch_add_explicit(callbackCount,1,memory_order_relaxed);dispatch_semaphore_signal(callbacks);}];
            [commands commit];[commands waitUntilCompleted];valid=!dispatch_semaphore_wait(callbacks,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC));}
        uint32_t operations=0;valid=valid && SweetMTLCommandResult(commands,&sequences[phase],&operations) && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && targetBuffer.contents==originalTarget && sampleBuffer.contents==originalSample;
        unsigned mismatches=valid?render_compare_target(originalTarget,phase)+render_compare_sample(originalSample,phase==3):0;
        if(valid && mismatches){unsigned shown=0;for(unsigned i=0;i<3776 && shown<8;++i){unsigned char expected=0xa5;
                if(i>=64 && i<3712){unsigned row=(i-64)/192,column=(i-64)%192;if(column<132){unsigned char bgra[4];render_target_pixel(column/4,row,phase,bgra);expected=bgra[column%4];}}
                if(originalTarget[i]!=expected){emit("SWEET AIR RENDER mismatch phase=%u target_offset=%u actual=%02x expected=%02x",phase,i,originalTarget[i],expected);++shown;}}}
        uint64_t hash=valid?render_bytes_hash(originalTarget,3776):0,sampleHash=valid?render_bytes_hash(originalSample,512):0;
        unsigned char output[240];memset(output,0xe7,sizeof(output));unsigned readMismatches=0;
        if(valid){[source getBytes:output+8 bytesPerRow:44 fromRegion:MTLRegionMake2D(0,0,7,5) mipmapLevel:0];
            for(unsigned i=0;i<sizeof(output);++i){unsigned char expected=0xe7;if(i>=8){unsigned y=(i-8)/44,x=(i-8)%44;
                    if(y<5 && x<28){unsigned char rgba[4];render_sample_pixel(x/4,y,phase==3,rgba);expected=rgba[x%4];}}
                readMismatches+=output[i]!=expected;}}
        valid=valid && !mismatches && !readMismatches && (phase==0 || sequences[phase]>sequences[phase-1]);
        emit("SWEET AIR RENDER phase=%u status=%lu error=%u sequence=%u operations=%u callback_count=%u mismatches=%u read_mismatches=%u target_hash=%016llx sample_hash=%016llx original_pages=%u valid=%u",
            phase,(unsigned long)commands.status,commands.error!=nil,sequences[phase],operations,atomic_load_explicit(callbackCount,memory_order_relaxed),mismatches,readMismatches,
            (unsigned long long)hash,(unsigned long long)sampleHash,targetBuffer.contents==originalTarget && sampleBuffer.contents==originalSample,valid);
        if(!valid && commands.error)emit("SWEET AIR RENDER failure phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);
        [commands release];
    }
    [queue release];[solid release];[sampled release];[vertex release];[fragment release];[textureVertex release];[textureFragment release];[library release];
    [target release];[source release];[sampler release];[targetBuffer release];[sampleBuffer release];[positionBuffer release];[textureBuffer release];
    NSUInteger final=device?device.currentAllocatedSize:0;valid=valid && final==0 && atomic_load_explicit(callbackCount,memory_order_relaxed)==4;
    emit("SWEET AIR RENDER END result=%u phases=4 draw_ops=3 clear_ops=1 checked_target_bytes=15104 checked_sample_bytes=2048 guard_bytes=6560 final_allocated=%lu standard_Metal=1",valid?0:1,(unsigned long)final);
    [device release];dispatch_release(callbacks);return valid;
}
