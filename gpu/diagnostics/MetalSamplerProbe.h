/* SPDX-License-Identifier: BSD-2-Clause
 * Native AIR literal samplers on original target/source/vertex pages.
 */
#pragma once
#include <CommonCrypto/CommonDigest.h>
static uint64_t sampler_probe_hash(const unsigned char *bytes,unsigned count)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)h=(h^bytes[i])*UINT64_C(1099511628211);return h;}
static void sampler_probe_dump(MetalABIEmit emit,const unsigned char *bytes,unsigned phase,unsigned buffer)
{
    for(unsigned offset=0;offset<16384;){
        unsigned same=1;while(offset+same<16384 && bytes[offset+same]==bytes[offset])++same;
        unsigned width=0,total=0;if(same>=8){width=1;total=same;}
        else if(offset+16<=16384){unsigned groups=1;while(offset+(groups+1)*4<=16384 && !memcmp(bytes+offset,bytes+offset+groups*4,4))++groups;
            if(groups>=4){width=4;total=groups*4;}}
        if(width){char pattern[9];for(unsigned i=0;i<width;++i)snprintf(pattern+i*2,3,"%02x",bytes[offset+i]);
            emit("SWEET AIR SAMPLER run phase=%u buffer=%u offset=%u bytes=%u pattern=%s",phase,buffer,offset,total,pattern);offset+=total;
        }else{unsigned n=16384-offset;if(n>64)n=64;char hex[129];for(unsigned i=0;i<n;++i)snprintf(hex+i*2,3,"%02x",bytes[offset+i]);
            emit("SWEET AIR SAMPLER data phase=%u buffer=%u offset=%u bytes=%u hex=%s",phase,buffer,offset,n,hex);offset+=n;}
    }
}
static bool metal_sampler_probe(MetalABIEmit emit)
{
    emit("SWEET AIR SAMPLER begin phases=3 original_word=807bff0000080a49 literal_states=1 explicit_LOD=1 source_slot=5 external_samplers=0 original_pages=1");
    unsigned char digest[32];CC_SHA256(sweet_air_sampler_fixture,(CC_LONG)sizeof(sweet_air_sampler_fixture),digest);char hex[65]={0};
    for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);emit("SWEET AIR SAMPLER input library_sha256=%s",hex);
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);
    dispatch_data_t data=dispatch_data_create(sweet_air_sampler_fixture,sizeof(sweet_air_sampler_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=device?[device newLibraryWithData:data error:&error]:nil;dispatch_release(data);
    id<MTLFunction> vertex=[library newFunctionWithName:@"sampler_position"],functions[3]={nil,nil,nil};id<MTLRenderPipelineState> pipelines[3]={nil,nil,nil};
    NSArray *names=@[@"sampler_original",@"sampler_pair",@"sampler_lod"];bool valid=device && library && vertex;
    for(unsigned i=0;i<3 && valid;++i){functions[i]=[library newFunctionWithName:names[i]];
        MTLRenderPipelineDescriptor *d=[MTLRenderPipelineDescriptor new];d.vertexFunction=vertex;d.fragmentFunction=functions[i];d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        pipelines[i]=[device newRenderPipelineStateWithDescriptor:d error:&error];[d release];valid=functions[i] && pipelines[i];}
    id<MTLBuffer> output=[device newBufferWithLength:16384 options:0],sample=[device newBufferWithLength:16384 options:0],positions=[device newBufferWithLength:16384 options:0];
    MTLTextureDescriptor *td=[MTLTextureDescriptor new];td.textureType=MTLTextureType2D;td.width=33;td.height=19;td.pixelFormat=MTLPixelFormatBGRA8Unorm;
    td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageRenderTarget;
    id<MTLTexture> target=[output newTextureWithDescriptor:td offset:64 bytesPerRow:192];
    td.width=2;td.height=2;td.mipmapLevelCount=2;td.pixelFormat=MTLPixelFormatRGBA8Unorm;td.usage=MTLTextureUsageShaderRead;
    id<MTLTexture> image=[sample newTextureWithDescriptor:td offset:128 bytesPerRow:64];[td release];
    id<MTLCommandQueue> queue=[device newCommandQueueWithMaxCommandBufferCount:8];
    unsigned char *raw[3]={output.contents,sample.contents,positions.contents};valid=valid && target && image && queue && raw[0] && raw[1] && raw[2];
    const float triangle[12]={-1,1,.5f,1,3,1,.5f,1,-1,-3,.5f,1};
    unsigned char wantedSample[16384],wantedVertices[16384];memset(wantedSample,0xb6,sizeof(wantedSample));memset(wantedVertices,0xd4,sizeof(wantedVertices));
    const unsigned char colours[16]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255},blue[4]={0,0,255,255};
    memcpy(wantedSample+128,colours,8);memcpy(wantedSample+192,colours+8,8);memcpy(wantedSample+256,blue,4);memcpy(wantedVertices+64,triangle,sizeof(triangle));
    if(valid){memset(raw[0],0xa5,16384);memcpy(raw[1],wantedSample,16384);memcpy(raw[2],wantedVertices,16384);}
    if(!valid && error)emit("SWEET AIR SAMPLER failure phase=create reason=%s",error.localizedDescription.UTF8String);
    uint32_t previous=0;unsigned completed=0;
    for(unsigned phase=0;phase<3 && valid;++phase){
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=commands && encoder;
        if(valid){[encoder setRenderPipelineState:pipelines[phase]];[encoder setVertexBuffer:positions offset:64 atIndex:0];[encoder setFragmentTexture:image atIndex:5];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
        uint32_t sequence=0,operations=0;valid=valid && SweetMTLCommandResult(commands,&sequence,&operations) && sequence>previous && operations==1 &&
            commands.status==MTLCommandBufferStatusCompleted && !commands.error && output.contents==raw[0] && sample.contents==raw[1] && positions.contents==raw[2];
        const unsigned char expected[3][4]={{0,0,255,255},{0,255,255,255},{255,0,0,255}};unsigned mismatch=0;
        if(valid)for(unsigned i=0;i<16384;++i){unsigned char value=0xa5;
            if(i>=64 && i<3712){unsigned column=(i-64)%192;if(column<132)value=expected[phase][column%4];}
            mismatch+=raw[0][i]!=value;mismatch+=raw[1][i]!=wantedSample[i];mismatch+=raw[2][i]!=wantedVertices[i];}
        emit("SWEET AIR SAMPLER phase=%u sequence=%u operations=%u status=%lu error=%u checked_bytes=49152 pixels=627 mismatches=%u original_pages=%u external_samplers=0 valid=%u",
            phase,sequence,operations,(unsigned long)commands.status,commands.error!=nil,mismatch,output.contents==raw[0] && sample.contents==raw[1] && positions.contents==raw[2],valid && !mismatch);
        if(valid)for(unsigned b=0;b<3;++b){emit("SWEET AIR SAMPLER buffer phase=%u index=%u raw_hash=%016llx original_pages=1",phase,b,(unsigned long long)sampler_probe_hash(raw[b],16384));sampler_probe_dump(emit,raw[b],phase,b);}
        valid=valid && !mismatch;if(valid){previous=sequence;++completed;}
        if(!valid && commands.error)emit("SWEET AIR SAMPLER failure phase=%u reason=%s",phase,commands.error.localizedDescription.UTF8String);[commands release];
    }
    for(unsigned i=0;i<3;++i){[pipelines[i] release];[functions[i] release];}[vertex release];[library release];[queue release];
    [target release];[image release];[output release];[sample release];[positions release];NSUInteger final=device.currentAllocatedSize;valid=valid && completed==3 && !final;
    emit("SWEET AIR SAMPLER END result=%u checked_bytes=147456 phases=3 final_allocated=%lu GPU_only=1",valid?0:1,(unsigned long)final);[device release];return valid;
}
