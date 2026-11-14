/* Original Metal XR bytes rendered/decoded on FD540, all strides and guards. */
#pragma once
#include <stdatomic.h>
#include "../sweetgl/XRColourTables.h"
static unsigned xr_probe_code(float value,bool alpha,bool srgb)
{
    double magnitude=fabs((double)value);unsigned amount=0;
    if(srgb && !alpha){unsigned index=(unsigned)nearbyint(magnitude*8192.0);while(amount+1<640 && sweet_xr_srgb_encode_first[amount+1]<=index)++amount;}
    else {double exact=magnitude*510.0;unsigned whole=(unsigned)floor(exact);double part=exact-whole;amount=whole+(part>0.5 || (part==0.5 && (whole&1)));}
    return value<0?384-(amount<384?amount:384):(384+amount<1023?384+amount:1023);
}
static void xr_probe_colour(unsigned phase,unsigned x,unsigned y,unsigned format,unsigned char bytes[8])
{
    bool solid=phase && x>=3 && x<14 && y>=2 && y<7;float colour[4]={1,0,0,1};if(solid){colour[0]=0.25f;colour[1]=0.5f;colour[2]=0.75f;}
    unsigned r=xr_probe_code(colour[0],false,format&1),g=xr_probe_code(colour[1],false,format&1),b=xr_probe_code(colour[2],false,format&1),a=894;
    if(format<554){uint16_t words[4]={(uint16_t)(b<<6),(uint16_t)(g<<6),(uint16_t)(r<<6),(uint16_t)(a<<6)};memcpy(bytes,words,8);}
    else {uint32_t packed=0xc0000000u|(r<<20)|(g<<10)|b;memcpy(bytes,&packed,4);}
}
static unsigned xr_probe_channel(unsigned code,bool srgb)
{
    float value=srgb?(float)sweet_xr_srgb_decode_steps[code>=384?code-384:384-code]/8192.0f:(float)((int)code-384)/510.0f;
    if(srgb && code<384)value=-value;value=fminf(fmaxf(value,0),1);return (unsigned)floorf(value*255.0f+0.5f);
}
static void xr_probe_sample_colour(unsigned x,unsigned y,unsigned format,unsigned char bgra[4])
{
    unsigned char raw[8]={0};xr_probe_colour(1,x,y,format,raw);unsigned r,g,b;
    if(format<554){uint16_t codes[4];memcpy(codes,raw,8);b=codes[0]>>6;g=codes[1]>>6;r=codes[2]>>6;}
    else {uint32_t word;memcpy(&word,raw,4);b=word&1023;g=(word>>10)&1023;r=(word>>20)&1023;}
    bgra[0]=(unsigned char)xr_probe_channel(b,format&1);bgra[1]=(unsigned char)xr_probe_channel(g,format&1);bgra[2]=(unsigned char)xr_probe_channel(r,format&1);bgra[3]=255;
}
static bool metal_xr_probe(MetalABIEmit emit)
{
    emit("SWEET AIR XR begin formats=552,553,554,555 standard_texture_render=1 raw_code_storage=1 GPU_sample_decode=1");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);bool valid=device && !error;
    dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"position"],fragment=[library newFunctionWithName:@"color"];
    id<MTLFunction> sampleVertex=[library newFunctionWithName:@"texture_position"],sampleFragment=[library newFunctionWithName:@"textured"];
    const float positions[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};
    const float coordinates[24]={-1,1,0.5f,1,0,0,0,0,3,1,0.5f,1,34,0,0,0,-1,-3,0.5f,1,0,18,0,0};
    id<MTLBuffer> vertices=[device newBufferWithBytes:positions length:sizeof(positions) options:0],coordinatesBuffer=[device newBufferWithBytes:coordinates length:sizeof(coordinates) options:0];
    id<MTLCommandQueue> queue=[device newCommandQueueWithMaxCommandBufferCount:8];MTLSamplerDescriptor *filter=[MTLSamplerDescriptor new];
    id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:filter];[filter release];
    MTLRenderPipelineDescriptor *pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=sampleVertex;pipeline.fragmentFunction=sampleFragment;pipeline.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> sampled=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
    valid=valid && library && vertex && fragment && sampleVertex && sampleFragment && vertices && coordinatesBuffer && queue && sampler && sampled;
    unsigned formatsPassed=0;
    for(unsigned format=552;format<=555 && valid;++format){@autoreleasepool{
        pipeline.vertexFunction=vertex;pipeline.fragmentFunction=fragment;pipeline.colorAttachments[0].pixelFormat=(MTLPixelFormat)format;
        error=nil;id<MTLRenderPipelineState> solid=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
        unsigned pixel=format<554?8:4,pitch=format<554?192:128,targetBytes=128+pitch*9;
        id<MTLBuffer> raw=[device newBufferWithLength:targetBytes options:0],output=[device newBufferWithLength:1344 options:0];
        MTLTextureDescriptor *descriptor=[MTLTextureDescriptor new];descriptor.textureType=MTLTextureType2D;descriptor.width=17;descriptor.height=9;
        descriptor.storageMode=MTLStorageModeShared;descriptor.pixelFormat=(MTLPixelFormat)format;descriptor.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
        id<MTLTexture> target=[raw newTextureWithDescriptor:descriptor offset:64 bytesPerRow:pitch];descriptor.pixelFormat=MTLPixelFormatBGRA8Unorm;descriptor.usage=MTLTextureUsageRenderTarget;
        id<MTLTexture> destination=[output newTextureWithDescriptor:descriptor offset:128 bytesPerRow:128];[descriptor release];
        unsigned char *original=(unsigned char *)raw.contents,*outputOriginal=(unsigned char *)output.contents;
        valid=valid && solid && raw && output && target && destination && original && outputOriginal;
        if(valid){memset(original,0xa5,targetBytes);memset(outputOriginal,0xe7,1344);}
        uint32_t previousSequence=0;
        for(unsigned phase=0;phase<3 && valid;++phase){
            MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=phase==2?destination:target;
            pass.colorAttachments[0].loadAction=phase==0?MTLLoadActionClear:MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(1,0,0,1);
            id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=valid && encoder && commands;
            if(valid && phase){[encoder setRenderPipelineState:phase==2?sampled:solid];[encoder setVertexBuffer:phase==2?coordinatesBuffer:vertices offset:0 atIndex:0];
                if(phase==1)[encoder setScissorRect:(MTLScissorRect){3,2,11,5}];
                else {[encoder setFragmentTexture:target atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];}
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];}
            if(valid){[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
            uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && operations==1 && sequence>previousSequence &&
                !commands.error && commands.status==MTLCommandBufferStatusCompleted && raw.contents==original && output.contents==outputOriginal;previousSequence=sequence;
            unsigned mismatches=0;uint64_t rawHash=UINT64_C(14695981039346656037),outputHash=rawHash;
            if(valid){for(unsigned i=0;i<targetBytes;++i){unsigned char expected=0xa5;if(i>=64 && i<64+pitch*9){unsigned y=(i-64)/pitch,x=(i-64)%pitch;
                        if(x<17*pixel){unsigned char bytes[8];xr_probe_colour(phase,x/pixel,y,format,bytes);expected=bytes[x%pixel];}}
                    if(original[i]!=expected){if(mismatches<4)emit("SWEET AIR XR mismatch format=%u phase=%u offset=%u actual=%02x expected=%02x",format,phase,i,original[i],expected);++mismatches;}rawHash=(rawHash^original[i])*UINT64_C(1099511628211);}
                for(unsigned i=0;i<1344;++i){unsigned char expected=0xe7;if(phase==2 && i>=128 && i<128+128*9){unsigned y=(i-128)/128,x=(i-128)%128;
                        if(x<17*4){unsigned char bytes[4];xr_probe_sample_colour(x/4,y,format,bytes);expected=bytes[x%4];}}
                    mismatches+=outputOriginal[i]!=expected;outputHash=(outputHash^outputOriginal[i])*UINT64_C(1099511628211);}}
            valid=valid && !mismatches;
            emit("SWEET AIR XR format=%u phase=%u status=%lu error=%u sequence=%u operations=%u raw_bytes=%u output_bytes=1344 pixels=153 mismatches=%u raw_hash=%016llx output_hash=%016llx original_pages=1 valid=%u",
                format,phase,(unsigned long)commands.status,commands.error!=nil,sequence,operations,targetBytes,mismatches,(unsigned long long)rawHash,(unsigned long long)outputHash,valid);
            if(!valid)emit("SWEET AIR XR failure format=%u phase=%u reason=%s",format,phase,(error?:commands.error).localizedDescription.UTF8String?:"pixel or backing mismatch");[commands release];
        }
        [solid release];[target release];[destination release];[raw release];[output release];if(valid)++formatsPassed;
    }}
    [sampled release];[pipeline release];[sampler release];[queue release];[vertices release];[coordinatesBuffer release];[vertex release];[fragment release];[sampleVertex release];[sampleFragment release];[library release];
    NSUInteger final=device?device.currentAllocatedSize:0;valid=valid && formatsPassed==4 && final==0;
    emit("SWEET AIR XR END result=%u formats=%u phases=12 checked_raw_output_bytes=34944 final_allocated=%lu GPU_only=1",valid?0:1,formatsPassed,(unsigned long)final);[device release];return valid;
}
