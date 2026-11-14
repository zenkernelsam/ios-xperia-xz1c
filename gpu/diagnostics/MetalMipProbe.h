/* Six real original-page levels, GPU regeneration, nearest/trilinear sampling. */
#pragma once
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../sweetgl/MetalShaderAPI.h"
#include "../sweetgl/XRColourTables.h"
@protocol SweetMipBackingObserve
- (SweetShaderTextureBinding)bindingForIndex:(unsigned)index stage:(unsigned)stage;
@end
static unsigned mip_probe_code(float value)
{unsigned fixed=(unsigned)nearbyintf(value*8192),n=0;while(n+1<640 && sweet_xr_srgb_encode_first[n+1]<=fixed)++n;return 384+n;}
static void mip_probe_rgba(unsigned level,unsigned x,unsigned y,unsigned phase,unsigned char rgba[4])
{unsigned scale=1u<<level,hscale=level>4?16:scale;rgba[0]=(unsigned char)(16+4*phase+4*x*scale+2*(scale-1));
    rgba[1]=(unsigned char)(32+4*phase+8*y*hscale+4*(hscale-1));rgba[2]=64+4*phase;rgba[3]=255;}
static void mip_probe_stored(unsigned format,unsigned level,unsigned x,unsigned y,unsigned phase,unsigned srgb,unsigned char stored[8])
{
    unsigned char rgba[4];mip_probe_rgba(level,x,y,phase,rgba);
    if(format==81 || format==553){unsigned white=(x+y)&1;rgba[0]=rgba[2]=level?srgb:(white?255:0);rgba[1]=phase?0:rgba[0];rgba[3]=255;}
    if(format==553){unsigned value=level?mip_probe_code(.5f):((x+y)&1?894:384);uint16_t words[4]={(uint16_t)(value<<6),(uint16_t)((phase?384:value)<<6),(uint16_t)(value<<6),894<<6};memcpy(stored,words,8);}
    else if(format==70)memcpy(stored,rgba,4);else{stored[0]=rgba[2];stored[1]=rgba[1];stored[2]=rgba[0];stored[3]=rgba[3];}
}
static uint64_t mip_probe_hash(const unsigned char *bytes,unsigned count)
{uint64_t h=UINT64_C(14695981039346656037);for(unsigned i=0;i<count;++i)h=(h^bytes[i])*UINT64_C(1099511628211);return h;}
static unsigned mip_probe_raw(const unsigned char *bytes,unsigned format,unsigned phase,const SweetShaderTextureBinding *binding,unsigned codes[6])
{
    unsigned mismatch=0;for(unsigned level=1;level<6;++level){unsigned offset=(unsigned)binding->mips[level-1].offset;codes[level]=format==81?bytes[offset+2]:0;
        if(format==81 && (codes[level]<187 || codes[level]>189))++mismatch;}
    unsigned pixel=format==553?8:4;
    for(unsigned i=0;i<16384;++i){unsigned char wanted=0xa5;
        for(unsigned level=0;level<6;++level){unsigned offset=level?(unsigned)binding->mips[level-1].offset:(unsigned)binding->offset;
            unsigned pitch=level?binding->mips[level-1].pitch:binding->pitch,width=level?binding->mips[level-1].width:binding->width,height=level?binding->mips[level-1].height:binding->height;
            if(i>=offset && i<offset+pitch*height){unsigned row=(i-offset)/pitch,column=(i-offset)%pitch;
                if(column<width*pixel){unsigned char stored[8];mip_probe_stored(format,level,column/pixel,row,phase,codes[level],stored);wanted=stored[column%pixel];}break;}}
        mismatch+=bytes[i]!=wanted;
    }return mismatch;
}
static unsigned mip_probe_srgb_byte(unsigned code)
{double encoded=code/255.0,linear=encoded<=.04045?encoded/12.92:pow((encoded+.055)/1.055,2.4);return (unsigned)floor(linear*255+.5);}
static void mip_probe_sample(unsigned format,unsigned x,unsigned y,unsigned phase,unsigned sample,const unsigned codes[6],unsigned char bgra[4])
{
    if(format==81 || format==553){unsigned r;
        if(format==553)r=(unsigned)floor(sweet_xr_srgb_decode_steps[mip_probe_code(.5f)-384]/8192.0*255+.5);
        else if(sample==2){double a=codes[1]/255.0,b=codes[2]/255.0;a=pow((a+.055)/1.055,2.4);b=pow((b+.055)/1.055,2.4);r=(unsigned)floor((a+b)*.5*255+.5);}
        else r=mip_probe_srgb_byte(codes[sample==0?1:4]);bgra[0]=bgra[2]=r;bgra[1]=phase?0:r;bgra[3]=255;return;}
    unsigned char a[4],b[4];unsigned level=sample==1?4:1;mip_probe_rgba(level,x/(1u<<level),y/(1u<<level),phase,a);
    if(sample==2){mip_probe_rgba(2,x/4,y/4,phase,b);for(unsigned c=0;c<3;++c)a[c]=(a[c]+b[c])/2;}
    bgra[0]=a[2];bgra[1]=a[1];bgra[2]=a[0];bgra[3]=255;
}
static bool metal_mip_probe(MetalABIEmit emit)
{
    struct timespec started={0},finished={0};bool timing=clock_gettime(CLOCK_MONOTONIC,&started)==0;
    emit("SWEET METAL MIPS begin formats=70,80,81,553 levels=6 generations=2 samples=3 original_pages=1");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);dispatch_data_t data=dispatch_data_create(sweet_air_render_fixture,sizeof(sweet_air_render_fixture),NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);id<MTLFunction> vertex=[library newFunctionWithName:@"texture_position"],fragment=[library newFunctionWithName:@"textured"];
    MTLRenderPipelineDescriptor *p=[MTLRenderPipelineDescriptor new];p.vertexFunction=vertex;p.fragmentFunction=fragment;p.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:p error:&error];[p release];
    const float coordinates[24]={-1,1,.5f,1,0,0,0,0,3,1,.5f,1,64,0,0,0,-1,-3,.5f,1,0,32,0,0};
    id<MTLBuffer> vertices=[device newBufferWithBytes:coordinates length:sizeof(coordinates) options:0];id<MTLCommandQueue> queue=[device newCommandQueue];
    bool valid=device && library && vertex && fragment && pipeline && vertices && queue;unsigned formats[]={70,80,81,553},passed=0;uint32_t previous=0;
    for(unsigned f=0;f<4 && valid;++f){@autoreleasepool{unsigned format=formats[f],pixel=format==553?8:4;
        id<MTLBuffer> raw=[device newBufferWithLength:16384 options:0],output=[device newBufferWithLength:16384 options:0];
        MTLTextureDescriptor *d=[MTLTextureDescriptor new];d.textureType=MTLTextureType2D;d.width=32;d.height=16;d.mipmapLevelCount=6;d.storageMode=MTLStorageModeShared;
        d.pixelFormat=(MTLPixelFormat)format;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
        id<MTLTexture> source=[raw newTextureWithDescriptor:d offset:64 bytesPerRow:32*pixel];d.mipmapLevelCount=1;d.pixelFormat=MTLPixelFormatBGRA8Unorm;
        id<MTLTexture> target=[output newTextureWithDescriptor:d offset:128 bytesPerRow:128];[d release];unsigned char *original=raw.contents,*out=output.contents;
        valid=valid && raw && output && source && target && original && out;SweetShaderTextureBinding binding={0};if(source)binding=[(id<SweetMipBackingObserve>)source bindingForIndex:13 stage:2];
        if(valid)memset(original,0xa5,16384);unsigned codes[6]={0};
        for(unsigned phase=0;phase<2 && valid;++phase){unsigned char upload[4096];
            for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x)mip_probe_stored(format,0,x,y,phase,0,upload+(y*32+x)*pixel);
            [source replaceRegion:MTLRegionMake2D(0,0,32,16) mipmapLevel:0 withBytes:upload bytesPerRow:32*pixel];
            id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLBlitCommandEncoder> blit=[commands blitCommandEncoder];valid=valid && commands && blit;
            if(valid){[blit generateMipmapsForTexture:source];[blit endEncoding];[commands commit];[commands waitUntilCompleted];}
            uint32_t fence=0,ops=0;valid=valid && SweetMTLCommandResult(commands,&fence,&ops) && ops==5 && fence>previous && !commands.error && raw.contents==original;previous=fence;
            unsigned mismatch=valid?mip_probe_raw(original,format,phase,&binding,codes):0;valid=valid && !mismatch;uint64_t hash=valid?mip_probe_hash(original,16384):0;
            emit("SWEET METAL MIPS generation format=%u phase=%u sequence=%u operations=%u mismatches=%u raw_hash=%016llx valid=%u",format,phase,fence,ops,mismatch,(unsigned long long)hash,valid);
            emit("SWEET METAL MIPS codes format=%u phase=%u level1=%u level2=%u level3=%u level4=%u level5=%u tolerance=%u",format,phase,codes[1],codes[2],codes[3],codes[4],codes[5],format==81?1:0);
            [commands release];
            for(unsigned sample=0;sample<3 && valid;++sample){memset(out,0xe7,16384);MTLSamplerDescriptor *filter=[MTLSamplerDescriptor new];
                filter.minFilter=MTLSamplerMinMagFilterNearest;filter.magFilter=MTLSamplerMinMagFilterNearest;filter.mipFilter=sample==2?MTLSamplerMipFilterLinear:MTLSamplerMipFilterNearest;
                filter.lodMinClamp=filter.lodMaxClamp=sample==0?1:sample==1?4:1.5f;id<MTLSamplerState> sampler=[device newSamplerStateWithDescriptor:filter];[filter release];
                MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
                commands=[[queue commandBuffer] retain];id<MTLRenderCommandEncoder> encoder=[commands renderCommandEncoderWithDescriptor:pass];[pass release];valid=valid && sampler && encoder && commands;
                if(valid){[encoder setRenderPipelineState:pipeline];[encoder setVertexBuffer:vertices offset:0 atIndex:0];[encoder setFragmentTexture:source atIndex:13];[encoder setFragmentSamplerState:sampler atIndex:3];
                    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[commands commit];[commands waitUntilCompleted];}
                fence=ops=0;valid=valid && SweetMTLCommandResult(commands,&fence,&ops) && ops==1 && fence>previous && !commands.error && output.contents==out && raw.contents==original;previous=fence;
                unsigned char actualColour[4]={0};if(valid)memcpy(actualColour,out+128,4);unsigned maximumError=0;mismatch=valid?mip_probe_raw(original,format,phase,&binding,codes):0;
                if(valid)for(unsigned i=0;i<16384;++i){unsigned char wanted=0xe7;if(i>=128 && i<2176){unsigned y=(i-128)/128,x=(i-128)%128;unsigned char bgra[4];mip_probe_sample(format,x/4,y,phase,sample,codes,bgra);
                        if(format==81){unsigned error=(unsigned)abs((int)actualColour[x%4]-(int)bgra[x%4]);if(error>maximumError)maximumError=error;wanted=actualColour[x%4];}else wanted=bgra[x%4];}
                    mismatch+=out[i]!=wanted;}
                valid=valid && !mismatch && maximumError<=(format==81?1:0);
                emit("SWEET METAL MIPS sample format=%u phase=%u sample=%u sequence=%u operations=%u mismatches=%u max_error=%u output_hash=%016llx valid=%u",format,phase,sample,fence,ops,mismatch,maximumError,(unsigned long long)(valid?mip_probe_hash(out,16384):0),valid);
                emit("SWEET METAL MIPS colour format=%u phase=%u sample=%u b=%u g=%u r=%u a=%u",format,phase,sample,actualColour[0],actualColour[1],actualColour[2],actualColour[3]);
                [sampler release];[commands release];
            }
        }
        if(valid)++passed;[source release];[target release];[raw release];[output release];
    }}
    [pipeline release];[vertices release];[queue release];[vertex release];[fragment release];[library release];NSUInteger final=device.currentAllocatedSize;valid=valid && passed==4 && !final;
    emit("SWEET METAL MIPS END result=%u formats=%u levels=6 generations=8 samples=24 checked_bytes=917504 final_allocated=%lu GPU_only=1",valid?0:1,passed,(unsigned long)final);
    if(timing && !clock_gettime(CLOCK_MONOTONIC,&finished))emit("SWEET METAL MIPS timing duration_ms=%llu result=%u",(unsigned long long)((finished.tv_sec-started.tv_sec)*1000+(finished.tv_nsec-started.tv_nsec)/1000000),valid?0:1);
    [device release];return valid;
}
