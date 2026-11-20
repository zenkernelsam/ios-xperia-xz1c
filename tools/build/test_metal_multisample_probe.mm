/* Execute the real author probe and native objects with an explicit host model.
 * The model is not GLES/FD540 hardware evidence; the shader test is separate. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <cassert>
#include <map>
#include <vector>
#include <cstdarg>
#include <algorithm>
#include <mutex>
static std::recursive_mutex hostPageGate;
static std::map<uint32_t,SweetMetalBacking> pages;
static uint32_t nextHandle=1,fence=0;
static unsigned programs=0,retired=0;
struct SweetMetalProgram {uint32_t samples;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_depth32_supported(void){return 1;}
extern "C" int sweetgl_metal_stencil8_supported(void){return 1;}
extern "C" int sweetgl_metal_multisample_supported(uint32_t format,uint32_t samples)
{return (samples==2 || samples==4) && (format==80 || format==252 || format==253);}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *out)
{std::lock_guard<std::recursive_mutex> lock(hostPageGate);uint64_t span=sweet_metal_backing_span(bytes);void *data=nullptr;if(!span || posix_memalign(&data,16384,span))return -ENOMEM;memset(data,0,span);
    *out={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*16384,data,nextHandle++,0};pages[out->handle]=*out;return 0;}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){std::lock_guard<std::recursive_mutex> lock(hostPageGate);assert(pages.count(handle));free(pages[handle].contents);pages.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){std::lock_guard<std::recursive_mutex> lock(hostPageGate);return pages.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *,uint32_t,uint32_t,char *,size_t){return nullptr;}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *,const SweetShaderBufferBinding *,uint32_t,const uint32_t[3],const uint32_t[3],const uint32_t[3],uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_program_cache_state(const SweetMetalProgram *){return 0;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *,const char *,const SweetRenderPipelineConfig *,char *,size_t){return nullptr;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create_multisample(const char *v,const char *f,const SweetRenderPipelineConfig *config,uint32_t samples,char *,size_t)
{assert(v && f && config && config->vertex_buffers==1 && config->fragment_buffers==1 && config->fragment_writes==0);++programs;return new SweetMetalProgram{samples};}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *p){if(p){delete p;++retired;}}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_draw_depth(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_depth(const SweetRenderPass *,const SweetRenderDepthPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_draw_stencil(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_render_clear_stencil(const SweetRenderPass *,const SweetRenderDepthPass *,const SweetRenderStencilPass *,uint32_t *){return -ENOTSUP;}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -ENOTSUP;}
static unsigned char *pixel(SweetShaderTextureBinding b,unsigned samples,unsigned x,unsigned y,unsigned sample)
{auto &p=pages.at(b.handle);unsigned bytes=b.format==253?1:4;uint64_t at=b.offset+y*b.pitch+(x*samples+sample)*bytes;assert(at+bytes<=p.logical_bytes);return (unsigned char *)p.contents+at;}
static void put_color(unsigned char *dst,const double *rgba){for(unsigned i=0;i<4;++i)dst[i]=(unsigned char)llround(rgba[i==0?2:i==2?0:i]*255.0);}
static void clear_pages(const SweetRenderPass *c,const SweetRenderDepthPass *z,const SweetRenderStencilPass *s,unsigned samples)
{for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x)for(unsigned sample=0;sample<samples;++sample){
    if(c->load_action==2)put_color(pixel(c->target,samples,x,y,sample),c->clear);
    if(z->load_action==2){float v=z->clear_depth;memcpy(pixel(z->target,samples,x,y,sample),&v,4);}
    if(s->load_action==2)*pixel(s->target,samples,x,y,sample)=(unsigned char)s->clear_stencil;}}
static void imports(const SweetRenderPass *c,const SweetRenderDepthPass *z,const SweetRenderStencilPass *s,unsigned samples)
{printf("SWEET MSAA import handle=%u width=17 height=9 pitch=%u offset=64 format=80 samples=%u GPU=0x%llx result=0 original_pages=1\n",c->target.handle,c->target.pitch,samples,(unsigned long long)pages.at(c->target.handle).gpu_address);
    printf("SWEET MSAA pair depth=%u stencil=%u samples=%u result=0 separate_z32s8=1 original_pages=1\n",z->target.handle,s->target.handle,samples);
    printf("SWEET MSAA stencil_import handle=%u width=17 height=9 pitch=%u offset=64 samples=%u GPU=0x%llx result=0 separate_z32s8=1 original_pages=1\n",s->target.handle,s->target.pitch,samples,(unsigned long long)pages.at(s->target.handle).gpu_address);}
static void retire_pages(const SweetRenderPass *c,const SweetRenderDepthPass *z,const SweetRenderStencilPass *s,uint32_t sequence)
{printf("SWEET METAL MRT retired index=0 handle=%u format=80 result=0 sequence=%u original_pages=1\n",c->target.handle,sequence);
    printf("SWEET METAL DEPTH retired handle=%u result=0 sequence=%u original_pages=1\n",z->target.handle,sequence);
    printf("SWEET METAL STENCIL retired handle=%u result=0 sequence=%u original_pages=1\n",s->target.handle,sequence);}
extern "C" int sweetgl_metal_render_draw_multisample(SweetMetalProgram *program,const SweetRenderPass *c,const SweetRenderDepthPass *z,const SweetRenderStencilPass *s,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *,uint32_t textures,const SweetShaderSamplerBinding *,uint32_t samplers,uint32_t samples,uint32_t mask,uint32_t *sequence)
{
    std::lock_guard<std::recursive_mutex> lock(hostPageGate);
    assert(program && program->samples==samples && count==2 && !textures && !samplers && z && s);imports(c,z,s,samples);clear_pages(c,z,s,samples);
    const float *vertices=nullptr,*rgba=nullptr;for(unsigned i=0;i<count;++i){assert(pages.count(buffers[i].handle));auto p=(const float *)((unsigned char *)pages.at(buffers[i].handle).contents+buffers[i].offset);if(buffers[i].index==0)vertices=p;else if(buffers[i].index==8)rgba=p;}
    assert(vertices && rgba && s->state.front.compare==MTLCompareFunctionAlways && s->state.front.pass==MTLStencilOperationReplace);
    double values[4];for(unsigned i=0;i<4;++i)values[i]=rgba[i];float depth=vertices[2];
    for(unsigned y=draw->scissor_y;y<draw->scissor_y+draw->scissor_height;++y)for(unsigned x=draw->scissor_x;x<draw->scissor_x+draw->scissor_width;++x)
        for(unsigned sample=0;sample<samples;++sample)if(mask&(1u<<sample)){put_color(pixel(c->target,samples,x,y,sample),values);memcpy(pixel(z->target,samples,x,y,sample),&depth,4);*pixel(s->target,samples,x,y,sample)=(unsigned char)s->state.front_reference;}
    *sequence=++fence;retire_pages(c,z,s,*sequence);printf("SWEET MSAA draw samples=%u mask=%u color=%u depth=%u stencil=%u result=0 sequence=%u original_pages=1\n",samples,mask,c->target.handle,z->target.handle,s->target.handle,*sequence);return 0;
}
extern "C" int sweetgl_metal_render_clear_multisample(const SweetRenderPass *c,const SweetRenderDepthPass *z,const SweetRenderStencilPass *s,uint32_t samples,uint32_t *sequence)
{std::lock_guard<std::recursive_mutex> lock(hostPageGate);imports(c,z,s,samples);clear_pages(c,z,s,samples);*sequence=++fence;retire_pages(c,z,s,*sequence);printf("SWEET MSAA clear samples=%u color=%u depth=%u stencil=%u result=0 sequence=%u original_pages=1\n",samples,c->target.handle,z->target.handle,s->target.handle,*sequence);return 0;}
extern "C" int sweetgl_metal_multisample_resolve(const SweetMultisampleResolve *request,uint32_t *sequence)
{
    std::lock_guard<std::recursive_mutex> lock(hostPageGate);
    auto src=request->source,dst=request->target;unsigned samples=request->samples;
    printf("SWEET MSAA import handle=%u width=17 height=9 pitch=%u offset=64 format=%u samples=%u GPU=0x%llx result=0 original_pages=1\n",src.handle,src.pitch,src.format,samples,(unsigned long long)pages.at(src.handle).gpu_address);
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){if(!request->aspect){unsigned total[4]={0};for(unsigned sample=0;sample<samples;++sample){auto value=pixel(src,samples,x,y,sample);for(unsigned i=0;i<4;++i)total[i]+=value[i];}
            auto value=pixel(dst,1,x,y,0);for(unsigned i=0;i<4;++i)value[i]=total[i]/samples;}
        else{float result;memcpy(&result,pixel(src,samples,x,y,0),4);for(unsigned sample=1;sample<samples && request->filter;++sample){float v;memcpy(&v,pixel(src,samples,x,y,sample),4);result=request->filter==1?std::min(result,v):std::max(result,v);}memcpy(pixel(dst,1,x,y,0),&result,4);}}
    *sequence=++fence;printf("SWEET MSAA resolve src=%u dst=%u samples=%u aspect=%u filter=%u result=0 sequence=%u original_pages=1 GPU_only=1\n",src.handle,dst.handle,samples,request->aspect,request->filter,*sequence);return 0;
}
typedef void (*MetalABIEmit)(const char *,...);
#include "AIRMultisampleFixture.h"
#include "../../gpu/diagnostics/MetalMultisampleProbe.h"
static void emit_probe(const char *format,...){char row[512];va_list args;va_start(args,format);int n=vsnprintf(row,sizeof(row),format,args);va_end(args);assert(n>0 && n<(int)sizeof(row));puts(row);}
int main(void){@autoreleasepool{
    bool valid=metal_multisample_probe(emit_probe);assert(valid && pages.empty() && programs==retired);
    puts("Actual author MSAA probe host model PASS: native objects, all 18 phases, caller input retention, sample-mask snapshots, source/resolve guards and final retirement; not FD540 evidence");
}return 0;}
