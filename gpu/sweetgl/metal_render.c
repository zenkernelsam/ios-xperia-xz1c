/* AIR vertex/fragment programs render on original linear Metal backing pages. */
#include "MetalShaderInternals.h"
#include "MetalDepthAttachment.h"
#include "MetalStencilAttachment.h"
#include "MetalMultisampleAPI.h"
#include "MultisampleLayout.h"
#include "MultisampleResolveGLES.h"
#include "DepthStencilGLES.h"
#include "Depth32ClearGLES.h"
#include "Stencil8ClearGLES.h"
#include "StencilResourcePair.h"
#include "MetalBackingRules.h"
#include "XRColourTables.h"
#include "XRColourShader.h"
#include "MipColourShader.h"
#include "MultiTargetShader.h"
#include "XRBlendShader.h"
#include "RenderUniformRules.h"
#include "RenderSamplerRules.h"
#include "sweetgl.h"
#include "sweet_drm.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "frontend/winsys_handle.h"
#include "util/u_inlines.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_manager.h"
#include "state_tracker/st_format.h"
#include "util/format/u_format.h"
#include "main/enable.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "freedreno/freedreno_resource.h"
#include "drm-uapi/drm_fourcc.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
static uint64_t render_clock_ns(clockid_t clock)
{struct timespec value;if(clock_gettime(clock,&value))return 0;return (uint64_t)value.tv_sec*UINT64_C(1000000000)+(uint64_t)value.tv_nsec;}
static void render_compile_time(const char *stage,uint64_t wall,uint64_t cpu,GLint ok)
{
    uint64_t end=render_clock_ns(CLOCK_MONOTONIC),thread=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);
    sweet_drm_log("SWEET AIR RENDER compile_end stage=%s wall_us=%llu cpu_us=%llu cpu_clock=%u status=%d",stage,
        (unsigned long long)(end>=wall?(end-wall)/1000:0),(unsigned long long)(cpu && thread>=cpu?(thread-cpu)/1000:0),cpu && thread,ok);
}
/* Bounded metadata only. Preserve errno and the original GL/API call order. */
struct render_stage_observation { unsigned call; const char *stage; uint64_t wall,cpu; };
static unsigned render_observation_calls;
static unsigned render_observation_call(void)
{
    unsigned previous=__atomic_load_n(&render_observation_calls,__ATOMIC_RELAXED);
    while(previous<32){unsigned next=previous+1;
        if(__atomic_compare_exchange_n(&render_observation_calls,&previous,next,false,__ATOMIC_RELAXED,__ATOMIC_RELAXED))return next;}
    return 0;
}
static struct render_stage_observation render_stage_begin(unsigned call,const char *stage)
{
    int saved=errno;struct render_stage_observation value={call,stage,0,0};
    if(call){value.wall=render_clock_ns(CLOCK_MONOTONIC);value.cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);
        sweet_drm_log("SWEET RENDER STAGE BEGIN call=%u stage=%s at_ns=%llu metadata_only=1",call,stage,(unsigned long long)value.wall);}
    errno=saved;return value;
}
static void render_stage_end(struct render_stage_observation value,int result)
{
    int saved=errno;if(value.call){uint64_t wall=render_clock_ns(CLOCK_MONOTONIC),cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);
        sweet_drm_log("SWEET RENDER STAGE END call=%u stage=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u result=%d original_order=1",value.call,value.stage,
            (unsigned long long)(value.wall && wall>=value.wall?wall-value.wall:0),
            (unsigned long long)(value.cpu && cpu>=value.cpu?cpu-value.cpu:0),value.cpu && cpu>=value.cpu,result);}
    errno=saved;
}
struct render_target_observation {unsigned call,index,format,load;const char *part;uint64_t wall,cpu;};
static struct render_target_observation render_target_begin(unsigned call,unsigned index,const SweetRenderColorAttachment *a,const char *part)
{
    int saved=errno;struct render_target_observation value={call,index,a->target.format,a->load_action,part,0,0};
    if(call){value.wall=render_clock_ns(CLOCK_MONOTONIC);value.cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);
        sweet_drm_log("SWEET RENDER TARGET BEGIN call=%u index=%u format=%u load=%u part=%s at_ns=%llu metadata_only=1",
            call,index,value.format,value.load,part,(unsigned long long)value.wall);}
    errno=saved;return value;
}
static void render_target_end(struct render_target_observation value,int result)
{
    int saved=errno;if(value.call){uint64_t wall=render_clock_ns(CLOCK_MONOTONIC),cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);
        sweet_drm_log("SWEET RENDER TARGET END call=%u index=%u part=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u result=%d original_order=1",
            value.call,value.index,value.part,(unsigned long long)(value.wall && wall>=value.wall?wall-value.wall:0),
            (unsigned long long)(value.cpu && cpu>=value.cpu?cpu-value.cpu:0),value.cpu && cpu>=value.cpu,result);}
    errno=saved;
}
/* These are the same pinned Mesa entry bodies used by EXT_draw_buffers_indexed.
 * The iOS dispatch table does not expose their spellings, so call the linked
 * internal entry bodies on this private Mesa context. */
void GLAPIENTRY _mesa_BlendFuncSeparateiARB(GLuint,GLenum,GLenum,GLenum,GLenum);
void GLAPIENTRY _mesa_BlendEquationSeparateiARB(GLuint,GLenum,GLenum);
void GLAPIENTRY _mesa_ColorMaski(GLuint,GLboolean,GLboolean,GLboolean,GLboolean);
static int render_gl_error(const char *stage,int target)
{
    GLenum first=glGetError();if(first==GL_NO_ERROR)return 0;
    sweet_drm_log("SWEET METAL MRT GL_error stage=%s target=%d error=%#x",stage,target,first);
    for(GLenum next;(next=glGetError())!=GL_NO_ERROR;)sweet_drm_log("SWEET METAL MRT GL_error stage=%s target=%d additional=%#x",stage,target,next);
    return -EIO;
}
static void render_capabilities(struct st_context *context)
{
    struct pipe_screen *screen=context->screen;GLint images=0,units=0;glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS,&images);glGetIntegerv(GL_MAX_IMAGE_UNITS,&units);
    sweet_drm_log("SWEET METAL MRT caps targets=%u fp32_rt=%u fp32_blend=%u fp32_image=%u independent_enable=%u independent_func=%u float_blend=%u fbfetch=%u no_attachment=%u fragment_images=%d image_units=%d",
        context->ctx->Const.MaxDrawBuffers,
        screen->is_format_supported(screen,PIPE_FORMAT_R32G32B32A32_FLOAT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_RENDER_TARGET),
        screen->is_format_supported(screen,PIPE_FORMAT_R32G32B32A32_FLOAT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_RENDER_TARGET|PIPE_BIND_SAMPLER_VIEW|PIPE_BIND_BLENDABLE),
        screen->is_format_supported(screen,PIPE_FORMAT_R32G32B32A32_FLOAT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_SHADER_IMAGE),
        screen->get_param(screen,PIPE_CAP_INDEP_BLEND_ENABLE),screen->get_param(screen,PIPE_CAP_INDEP_BLEND_FUNC),
        context->ctx->Extensions.EXT_float_blend,screen->get_param(screen,PIPE_CAP_FBFETCH),screen->get_param(screen,PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT),images,units);
}
static enum pipe_format texture_format(uint32_t format)
{
    switch(format){case 10:return PIPE_FORMAT_R8_UNORM;case 30:return PIPE_FORMAT_R8G8_UNORM;
    case 70:return PIPE_FORMAT_R8G8B8A8_UNORM;case 71:return PIPE_FORMAT_R8G8B8A8_SRGB;
    case 80:return PIPE_FORMAT_B8G8R8A8_UNORM;case 81:return PIPE_FORMAT_B8G8R8A8_SRGB;
    case 115:return PIPE_FORMAT_R16G16B16A16_FLOAT;case 252:return PIPE_FORMAT_Z32_FLOAT;case 253:return PIPE_FORMAT_S8_UINT;
    case 552:case 553:return PIPE_FORMAT_R16G16B16A16_UINT;case 554:case 555:return PIPE_FORMAT_R32_UINT;default:return PIPE_FORMAT_NONE;}
}
static bool xr_format(uint32_t format){return format>=552 && format<=555;}
static unsigned pixel_bytes(uint32_t format){return format==10 || format==253?1:format==30?2:(format==115 || format==552 || format==553)?8:4;}
static int bind_xr_lut(GLuint program,GLuint *name)
{
    GLint at=glGetUniformLocation(program,"_smtl_xr_lut");if(at<0)return 0;
    GLint oldUnit=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&oldUnit);
    uint16_t data[1280];for(unsigned i=0;i<640;++i){data[i*2]=sweet_xr_srgb_decode_steps[i];data[i*2+1]=sweet_xr_srgb_encode_first[i];}
    glActiveTexture(GL_TEXTURE16);if(!*name){glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RG16UI,640,1,0,GL_RG_INTEGER,GL_UNSIGNED_SHORT,data);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }else glBindTexture(GL_TEXTURE_2D,*name);
    glUniform1i(at,16);glActiveTexture((GLenum)oldUnit);return glGetError()==GL_NO_ERROR?0:-EIO;
}
static bool texture_layout_samples(const SweetShaderTextureBinding *texture,unsigned samples,SweetMetalBacking *backing)
{
    enum pipe_format format=texture_format(texture?texture->format:0);
    if(!texture || format==PIPE_FORMAT_NONE || !texture->width || !texture->height || texture->width>16384 || texture->height>16384 ||
        (texture->offset&63) || (texture->pitch&63) || sweet_drm_metal_buffer_view(texture->handle,backing))return false;
    unsigned bytes=pixel_bytes(texture->format);
    return sweet_multisample_layout(texture->width,texture->height,bytes,samples,texture->pitch,texture->offset,backing->logical_bytes);
}
static bool texture_layout(const SweetShaderTextureBinding *texture,SweetMetalBacking *backing)
{return texture_layout_samples(texture,1,backing);}
static GLenum sample_target(unsigned samples){return samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D;}
#include "DepthTiledPlane.h"
static struct pipe_resource *import_multisample_plane(struct st_context *context,const SweetShaderTextureBinding *texture,unsigned samples,unsigned usage)
{
    SweetMetalBacking backing={0};if(!texture_layout_samples(texture,samples,&backing) || texture->offset>UINT32_MAX || texture->mip_count>1 || texture->reserved)return NULL;
    enum pipe_format format=texture_format(texture->format);
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=format,.width0=texture->width,.height0=texture->height,
        .depth0=1,.array_size=1,.nr_samples=samples,.nr_storage_samples=samples,.bind=usage,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=texture->handle,.stride=texture->pitch,
        .offset=(unsigned)texture->offset,.format=format,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(resource && sweet_drm_metal_buffer_view_keep(texture->handle))pipe_resource_reference(&resource,NULL);
    if(resource){struct fd_resource *actual=fd_resource(resource);
        if(actual->layout.cpp!=pixel_bytes(texture->format)*samples || actual->layout.pitch0!=texture->pitch ||
            actual->layout.slices[0].offset!=texture->offset || resource->nr_samples!=samples || resource->nr_storage_samples!=samples){
            pipe_resource_reference(&resource,NULL);return NULL;}
        // The external import initializes cpp but does not set this fdl field.
        // Bind it to the original resource's verified sample count.
        actual->layout.nr_samples=samples;}
    return resource;
}
static int describe_multisample_image(struct st_context *context,struct pipe_resource *resource,
    unsigned samples,GLenum internal_format,mesa_format storage,GLuint *name)
{
    GLenum target=sample_target(samples);glGenTextures(1,name);glBindTexture(target,*name);
    bool accepted=st_context_teximage(context,target,0,resource->format,resource,false);
    if(accepted){struct gl_texture_object *object=_mesa_get_current_tex_object(context->ctx,target);
        _mesa_lock_texture(context->ctx,object);struct gl_texture_image *image=_mesa_get_tex_image(context->ctx,object,target,0);
        if(!image)accepted=false;
        else{_mesa_init_teximage_fields_ms(context->ctx,image,resource->width0,resource->height0,1,0,internal_format,storage,samples,GL_TRUE);
            _mesa_update_texture_object_swizzle(context->ctx,object);_mesa_dirty_texobj(context->ctx,object);}
        _mesa_unlock_texture(context->ctx,object);}
    return accepted && glGetError()==GL_NO_ERROR?0:-EIO;
}
int sweetgl_metal_multisample_supported(uint32_t format,uint32_t samples)
{
    if((samples!=2 && samples!=4) || !sweetgl_init())return 0;struct pipe_screen *screen=sweetgl_screen();
    enum pipe_format pipe=texture_format(format);if(!screen || pipe==PIPE_FORMAT_NONE || xr_format(format))return 0;
    unsigned usage=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW|(format==252?PIPE_BIND_DEPTH_STENCIL:format==253?0:PIPE_BIND_RENDER_TARGET);
    if(!screen->is_format_supported(screen,pipe,PIPE_TEXTURE_2D,samples,samples,usage))return 0;
    if(format==253)return screen->is_format_supported(screen,PIPE_FORMAT_Z32_FLOAT_S8X24_UINT,PIPE_TEXTURE_2D,samples,samples,PIPE_BIND_DEPTH_STENCIL);
    return 1;
}
static int bind_multisample_texture(struct st_context *context,const SweetShaderTextureBinding *texture,unsigned samples,GLuint *name)
{
    if(!texture || texture->format==253 || !sweetgl_metal_multisample_supported(texture->format,samples))return -ENOTSUP;
    bool depth=texture->format==252;
    struct pipe_resource *resource=import_multisample_plane(context,texture,samples,PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW|(depth?PIPE_BIND_DEPTH_STENCIL:PIPE_BIND_RENDER_TARGET));
    if(!resource)return -ENOMEM;
    mesa_format storage=depth?MESA_FORMAT_Z_FLOAT32:st_pipe_format_to_mesa_format(resource->format);
    int result=describe_multisample_image(context,resource,samples,depth?GL_DEPTH_COMPONENT32F:(util_format_has_alpha(resource->format)?GL_RGBA:GL_RGB),storage,name);
    pipe_resource_reference(&resource,NULL);
    SweetMetalBacking backing={0};(void)texture_layout_samples(texture,samples,&backing);
    sweet_drm_log("SWEET MSAA import handle=%u width=%u height=%u pitch=%u offset=%llu format=%u samples=%u GPU=%#llx result=%d original_pages=1",
        texture->handle,texture->width,texture->height,texture->pitch,(unsigned long long)texture->offset,texture->format,samples,(unsigned long long)backing.gpu_address,result);
    return result;
}
static int bind_multisample_stencil(struct st_context *context,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,unsigned samples,GLuint *name)
{
    if(!stencil || !sweetgl_metal_multisample_supported(253,samples))return -ENOTSUP;
    struct pipe_resource *primary=NULL,*secondary=NULL;
    if(depth)primary=import_multisample_plane(context,&depth->target,samples,PIPE_BIND_SHARED|PIPE_BIND_DEPTH_STENCIL|PIPE_BIND_SAMPLER_VIEW);
    else{struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_Z32_FLOAT,.width0=stencil->target.width,.height0=stencil->target.height,
            .depth0=1,.array_size=1,.nr_samples=samples,.nr_storage_samples=samples,.bind=PIPE_BIND_DEPTH_STENCIL,.usage=PIPE_USAGE_DEFAULT};
        uint64_t modifier=DRM_FORMAT_MOD_LINEAR;primary=context->screen->resource_create_with_modifiers(context->screen,&description,&modifier,1);}
    if(!primary)return -ENOMEM;
    secondary=import_multisample_plane(context,&stencil->target,samples,PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW);
    if(!secondary || !sweet_stencil_pair_take_samples(primary,&secondary,samples)){
        pipe_resource_reference(&secondary,NULL);pipe_resource_reference(&primary,NULL);return -EINVAL;}
    int result=describe_multisample_image(context,primary,samples,GL_DEPTH32F_STENCIL8,MESA_FORMAT_Z32_FLOAT_S8X24_UINT,name);
    sweet_drm_log("SWEET MSAA pair depth=%u stencil=%u samples=%u result=%d separate_z32s8=1 original_pages=1",
        depth?depth->target.handle:0,stencil->target.handle,samples,result);
    pipe_resource_reference(&primary,NULL);SweetMetalBacking backing={0};(void)texture_layout_samples(&stencil->target,samples,&backing);
    sweet_drm_log("SWEET MSAA stencil_import handle=%u width=%u height=%u pitch=%u offset=%llu samples=%u GPU=%#llx result=%d separate_z32s8=1 original_pages=1",
        stencil->target.handle,stencil->target.width,stencil->target.height,stencil->target.pitch,(unsigned long long)stencil->target.offset,
        samples,(unsigned long long)backing.gpu_address,result);
    return result;
}
static int bind_texture(struct st_context *context,const SweetShaderTextureBinding *texture,GLuint *name)
{
    SweetMetalBacking backing={0};if(!texture_layout(texture,&backing))return -EINVAL;
    enum pipe_format format=texture_format(texture->format);
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=format,.width0=texture->width,.height0=texture->height,
        .depth0=1,.array_size=1,.bind=PIPE_BIND_SHARED|PIPE_BIND_RENDER_TARGET|PIPE_BIND_SAMPLER_VIEW,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=texture->handle,.stride=texture->pitch,
        .offset=(unsigned)texture->offset,.format=format,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(!resource)return -ENOMEM;
    if(sweet_drm_metal_buffer_view_keep(texture->handle)){pipe_resource_reference(&resource,NULL);return -EINVAL;}
    glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);
    bool accepted=st_context_teximage(context,GL_TEXTURE_2D,0,format,resource,false);pipe_resource_reference(&resource,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    sweet_drm_log("SWEET AIR RENDER texture handle=%u width=%u height=%u pitch=%u format=%u offset=%llu gpu=%#llx original_pages=1",
        texture->handle,texture->width,texture->height,texture->pitch,texture->format,(unsigned long long)texture->offset,(unsigned long long)backing.gpu_address);
    return accepted && glGetError()==GL_NO_ERROR?0:-EIO;
}
int sweetgl_metal_depth32_supported(void)
{
    if(!sweetgl_init())return 0;struct pipe_screen *screen=sweetgl_screen();
    return screen && screen->is_format_supported(screen,PIPE_FORMAT_Z32_FLOAT,PIPE_TEXTURE_2D,0,0,
        PIPE_BIND_DEPTH_STENCIL|PIPE_BIND_SAMPLER_VIEW|PIPE_BIND_SHARED);
}
int sweetgl_metal_stencil8_supported(void)
{
    if(!sweetgl_metal_depth32_supported())return 0;struct pipe_screen *screen=sweetgl_screen();
    return screen && screen->is_format_supported(screen,PIPE_FORMAT_Z32_FLOAT_S8X24_UINT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_DEPTH_STENCIL) &&
        screen->is_format_supported(screen,PIPE_FORMAT_S8_UINT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW);
}
static int bind_depth32(struct st_context *context,const SweetShaderTextureBinding *texture,GLuint *name)
{
    SweetMetalBacking backing={0};if(!texture || texture->format!=252 || texture->mip_count>1 || !texture_layout(texture,&backing) || !sweetgl_metal_depth32_supported())return -EINVAL;
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_Z32_FLOAT,.width0=texture->width,.height0=texture->height,
        .depth0=1,.array_size=1,.bind=PIPE_BIND_SHARED|PIPE_BIND_DEPTH_STENCIL|PIPE_BIND_SAMPLER_VIEW,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=texture->handle,.stride=texture->pitch,
        .offset=(unsigned)texture->offset,.format=PIPE_FORMAT_Z32_FLOAT,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(!resource)return -ENOMEM;
    if(sweet_drm_metal_buffer_view_keep(texture->handle)){pipe_resource_reference(&resource,NULL);return -EINVAL;}
    glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);
    bool accepted=st_context_teximage(context,GL_TEXTURE_2D,0,PIPE_FORMAT_Z32_FLOAT,resource,false);pipe_resource_reference(&resource,NULL);
    if(accepted){
        // st_context_teximage is a color-oriented external-image helper. Keep
        // its exact imported pipe_resource, but describe the real depth plane.
        struct gl_texture_object *object=_mesa_get_current_tex_object(context->ctx,GL_TEXTURE_2D);
        _mesa_lock_texture(context->ctx,object);struct gl_texture_image *image=_mesa_get_tex_image(context->ctx,object,GL_TEXTURE_2D,0);
        if(!image)accepted=false;
        else{_mesa_init_teximage_fields(context->ctx,image,texture->width,texture->height,1,0,GL_DEPTH_COMPONENT32F,MESA_FORMAT_Z_FLOAT32);
            _mesa_update_texture_object_swizzle(context->ctx,object);_mesa_dirty_texobj(context->ctx,object);}
        _mesa_unlock_texture(context->ctx,object);
    }
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    sweet_drm_log("SWEET METAL DEPTH import handle=%u width=%u height=%u pitch=%u offset=%llu GPU=%#llx original_pages=1",
        texture->handle,texture->width,texture->height,texture->pitch,(unsigned long long)texture->offset,(unsigned long long)backing.gpu_address);
    return accepted && glGetError()==GL_NO_ERROR?0:-EIO;
}
static struct pipe_resource *import_stencil_plane(struct st_context *context,const SweetShaderTextureBinding *texture)
{
    SweetMetalBacking backing={0};if(!texture || texture->format!=253 || texture->mip_count>1 || !texture_layout(texture,&backing))return NULL;
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_S8_UINT,.width0=texture->width,.height0=texture->height,
        .depth0=1,.array_size=1,.bind=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=texture->handle,.stride=texture->pitch,
        .offset=(unsigned)texture->offset,.format=PIPE_FORMAT_S8_UINT,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(resource && sweet_drm_metal_buffer_view_keep(texture->handle))pipe_resource_reference(&resource,NULL);
    return resource;
}
static int bind_stencil8(struct st_context *context,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,GLuint *name)
{
    if(!sweetgl_metal_stencil8_supported())return -ENOTSUP;
    struct pipe_resource *primary=NULL,*secondary=NULL;
    if(depth){SweetMetalBacking backing={0};if(!texture_layout(&depth->target,&backing))return -EINVAL;
        struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_Z32_FLOAT,.width0=depth->target.width,.height0=depth->target.height,
            .depth0=1,.array_size=1,.bind=PIPE_BIND_SHARED|PIPE_BIND_DEPTH_STENCIL|PIPE_BIND_SAMPLER_VIEW,.usage=PIPE_USAGE_DEFAULT};
        struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=depth->target.handle,.stride=depth->target.pitch,.offset=(unsigned)depth->target.offset,
            .format=PIPE_FORMAT_Z32_FLOAT,.modifier=DRM_FORMAT_MOD_LINEAR};
        primary=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
        if(primary && sweet_drm_metal_buffer_view_keep(depth->target.handle))pipe_resource_reference(&primary,NULL);
    }else{
        // FD540 programs one combined Z/S surface. With only stencil requested,
        // a private unused depth plane supplies its layout; depth tests and writes
        // stay disabled and no caller depth bytes are invented or altered.
        struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_Z32_FLOAT,.width0=stencil->target.width,.height0=stencil->target.height,
            .depth0=1,.array_size=1,.bind=PIPE_BIND_DEPTH_STENCIL,.usage=PIPE_USAGE_DEFAULT};
        const uint64_t modifier=DRM_FORMAT_MOD_LINEAR;
        primary=context->screen->resource_create_with_modifiers(context->screen,&description,&modifier,1);
    }
    if(!primary)return -ENOMEM;
    secondary=import_stencil_plane(context,&stencil->target);
    if(!secondary){pipe_resource_reference(&primary,NULL);return -ENOMEM;}
    // This is the pinned Mesa separate-z32s8 contract from u_transfer_helper:
    // keep the original depth internal_format/layout and own the imported S8
    // reference until resource_destroy releases both planes.
    if(!sweet_stencil_pair_take(primary,&secondary)){pipe_resource_reference(&secondary,NULL);pipe_resource_reference(&primary,NULL);return -EINVAL;}
    glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);
    bool accepted=st_context_teximage(context,GL_TEXTURE_2D,0,PIPE_FORMAT_Z32_FLOAT_S8X24_UINT,primary,false);
    pipe_resource_reference(&primary,NULL);
    if(accepted){struct gl_texture_object *object=_mesa_get_current_tex_object(context->ctx,GL_TEXTURE_2D);_mesa_lock_texture(context->ctx,object);
        struct gl_texture_image *image=_mesa_get_tex_image(context->ctx,object,GL_TEXTURE_2D,0);
        if(!image)accepted=false;
        else{_mesa_init_teximage_fields(context->ctx,image,stencil->target.width,stencil->target.height,1,0,GL_DEPTH32F_STENCIL8,MESA_FORMAT_Z32_FLOAT_S8X24_UINT);
            _mesa_update_texture_object_swizzle(context->ctx,object);_mesa_dirty_texobj(context->ctx,object);}
        _mesa_unlock_texture(context->ctx,object);}
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    if(depth){SweetMetalBacking backing={0};(void)texture_layout(&depth->target,&backing);
        sweet_drm_log("SWEET METAL DEPTH import handle=%u width=%u height=%u pitch=%u offset=%llu GPU=%#llx original_pages=1",
            depth->target.handle,depth->target.width,depth->target.height,depth->target.pitch,(unsigned long long)depth->target.offset,(unsigned long long)backing.gpu_address);}
    SweetMetalBacking backing={0};(void)texture_layout(&stencil->target,&backing);
    sweet_drm_log("SWEET METAL STENCIL import handle=%u width=%u height=%u pitch=%u offset=%llu GPU=%#llx original_pages=1 separate_z32s8=1",
        stencil->target.handle,stencil->target.width,stencil->target.height,stencil->target.pitch,(unsigned long long)stencil->target.offset,(unsigned long long)backing.gpu_address);
    return accepted && glGetError()==GL_NO_ERROR?0:-EIO;
}
static int bind_vertex_words(struct st_context *context,const SweetShaderBufferBinding *binding,GLuint program,GLuint *name)
{
    SweetMetalBacking backing={0};
    if(binding->index>=8 || sweet_drm_metal_buffer_view(binding->handle,&backing) || (binding->offset&3) || (binding->bytes&3) ||
        !binding->bytes || binding->offset>backing.logical_bytes || binding->bytes>backing.logical_bytes-binding->offset)return -EINVAL;
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_R32_UINT,.width0=4096,
        .height0=(unsigned)(backing.allocated_bytes/16384),.depth0=1,.array_size=1,.bind=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=binding->handle,.stride=16384,
        .offset=0,.format=PIPE_FORMAT_R32_UINT,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(!resource)return -ENOMEM;
    if(sweet_drm_metal_buffer_view_keep(binding->handle)){pipe_resource_reference(&resource,NULL);return -EINVAL;}
    unsigned unit=8+binding->index;glActiveTexture(GL_TEXTURE0+unit);glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);
    bool accepted=st_context_teximage(context,GL_TEXTURE_2D,0,PIPE_FORMAT_R32_UINT,resource,false);pipe_resource_reference(&resource,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    char uniform[64];snprintf(uniform,sizeof(uniform),"_smtl_vertex_buffer%u",binding->index);
    GLint at=glGetUniformLocation(program,uniform);if(at<0)return -EINVAL;glUniform1i(at,(GLint)unit);
    snprintf(uniform,sizeof(uniform),"_smtl_vertex_buffer_info[%u]",binding->index);at=glGetUniformLocation(program,uniform);
    if(at<0)return -EINVAL;glUniform2ui(at,(GLuint)(binding->offset/4),(GLuint)(binding->bytes/4));
    sweet_drm_log("SWEET AIR RENDER vertex_words index=%u handle=%u gpu=%#llx offset=%llu bytes=%llu logical=%llu integer_texture=1 guarded=1 original_pages=1",
        binding->index,binding->handle,(unsigned long long)backing.gpu_address,(unsigned long long)binding->offset,(unsigned long long)binding->bytes,(unsigned long long)backing.logical_bytes);
    return accepted && glGetError()==GL_NO_ERROR?0:-EIO;
}
static int decode_xr_texture_mode(struct st_context *context,const SweetShaderTextureBinding *binding,GLuint *texture,SweetMetalShaderCache *cache,bool render_target)
{
    // Filtering reads the values represented by XR codes. A GPU-only float
    // image retains that linear colour for the ordinary AIR sampled shader.
    // Original XR bytes remain on the original backing; no CPU conversion.
    static const char *vertex="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
    size_t bytes=strlen(sweet_xr_shader)+512;char *fragment=malloc(bytes);if(!fragment)return -ENOMEM;
    snprintf(fragment,bytes,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform highp usampler2D raw_image;layout(location=0) out vec4 colour;void main(){colour=_smtl_xr_unpack(texelFetch(raw_image,ivec2(gl_FragCoord.xy),0),%uu);}",sweet_xr_shader,binding->format);
    GLint previousFBO=0,previousProgram=0,previousUnit=0,previousTexture=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previousFBO);
    glGetIntegerv(GL_CURRENT_PROGRAM,&previousProgram);glGetIntegerv(GL_ACTIVE_TEXTURE,&previousUnit);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&previousTexture);
    GLuint raw=0,lut=0,fbo=0,vao=0,program=0,shaders[2]={0};int result=bind_texture(context,binding,&raw);GLint ok=0;
    if(result)goto done;
    const GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={vertex,fragment};
    program=cache?cache->decode[binding->format-552]:0;
    if(!program){for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
            glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok){result=-ENOTSUP;goto done;}}
        program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);
        if(!ok){result=-ENOTSUP;goto done;}if(cache)cache->decode[binding->format-552]=program;
        sweet_drm_log("SWEET METAL MIPMAP shader kind=decode format=%u program=%u cache=%u",binding->format,program,cache!=NULL);}
    glUseProgram(program);result=bind_xr_lut(program,cache?&cache->lut:&lut);if(result)goto done;glUniform1i(glGetUniformLocation(program,"raw_image"),0);
    glGenTextures(1,texture);glBindTexture(GL_TEXTURE_2D,*texture);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,binding->width,binding->height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,*texture,0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){result=-ENOTSUP;goto done;}
    glBindTexture(GL_TEXTURE_2D,raw);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_DITHER);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glViewport(0,0,binding->width,binding->height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,binding->width,binding->height);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previousFBO);glUseProgram((GLuint)previousProgram);
    if(fbo)glDeleteFramebuffers(1,&fbo);if(raw)glDeleteTextures(1,&raw);if(lut)glDeleteTextures(1,&lut);
    for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program && (!cache || cache->decode[binding->format-552]!=program))glDeleteProgram(program);free(fragment);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,(GLuint)previousTexture);glActiveTexture((GLenum)previousUnit);
    if(result && *texture){glDeleteTextures(1,texture);*texture=0;}
    if(render_target)sweet_drm_log("SWEET METAL MRT decode format=%u handle=%u width=%u height=%u result=%d GPU_only=1 original_target_pages=1",binding->format,binding->handle,binding->width,binding->height,result);
    else sweet_drm_log("SWEET AIR XR sample_decode format=%u handle=%u width=%u height=%u result=%d GPU_only=1 original_source_pages=1 float_shadow=1",binding->format,binding->handle,binding->width,binding->height,result);
    return result;
}
static int decode_xr_texture(struct st_context *context,const SweetShaderTextureBinding *binding,GLuint *texture,SweetMetalShaderCache *cache)
{return decode_xr_texture_mode(context,binding,texture,cache,false);}
static uint32_t mip_count(const SweetShaderTextureBinding *texture){return texture->mip_count?texture->mip_count:1;}
static SweetShaderTextureBinding mip_level(const SweetShaderTextureBinding *texture,unsigned level)
{
    SweetShaderTextureBinding view=*texture;view.mip_count=1;memset(view.mips,0,sizeof(view.mips));
    if(level){const SweetShaderTextureMip *m=&texture->mips[level-1];view.handle=m->handle;view.width=m->width;view.height=m->height;view.pitch=m->pitch;view.offset=m->offset;}return view;
}
static bool mip_layout_valid(const SweetShaderTextureBinding *texture)
{
    if(!texture || texture->reserved || mip_count(texture)>15)return false;
    SweetMetalBacking backing;SweetShaderTextureBinding prior={0};
    for(unsigned level=0;level<mip_count(texture);++level){SweetShaderTextureBinding current=mip_level(texture,level);
        if(!texture_layout(&current,&backing))return false;
        if(level && ((prior.width==1 && prior.height==1) || current.width!=(prior.width>1?prior.width/2:1) || current.height!=(prior.height>1?prior.height/2:1)))return false;
        for(unsigned earlier=0;earlier<level;++earlier){SweetShaderTextureBinding other=mip_level(texture,earlier);
            if(current.handle==other.handle){uint64_t end=current.offset+(uint64_t)current.pitch*current.height,otherEnd=other.offset+(uint64_t)other.pitch*other.height;
                if(current.offset<otherEnd && other.offset<end)return false;}}
        prior=current;
    }return true;
}
static int mip_copy_draw(GLuint source,unsigned width,unsigned height,uint32_t target_format,SweetMetalShaderCache *cache)
{
    GLuint shaders[2]={0},program=0,vao=0,lut=0;GLint valid=0;int result=-EIO;char *xr=NULL;
    const char *sources[2]={sweet_mip_vertex,sweet_mip_fragment};const GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
    if(xr_format(target_format)){xr=malloc(strlen(sweet_xr_shader)+512);if(!xr)return -ENOMEM;
        sprintf(xr,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform highp sampler2D source_image;uniform vec2 destination_extent;layout(location=0) out highp uvec4 colour;void main(){colour=_smtl_xr_pack(textureLod(source_image,gl_FragCoord.xy/destination_extent,0.0),%uu);}",sweet_xr_shader,target_format);sources[1]=xr;}
    unsigned slot=xr_format(target_format)?target_format-551:0;program=cache?cache->copy[slot]:0;
    if(!program){for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&valid);
            if(!valid){char error[1024];glGetShaderInfoLog(shaders[i],sizeof(error),NULL,error);sweet_drm_log("SWEET METAL MIPMAP compile failed %s",error);goto done;}}
        program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&valid);if(!valid)goto done;
        if(cache)cache->copy[slot]=program;sweet_drm_log("SWEET METAL MIPMAP shader kind=copy format=%u program=%u cache=%u",target_format,program,cache!=NULL);}
    glUseProgram(program);if(bind_xr_lut(program,cache?&cache->lut:&lut))goto done;glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glUniform1i(glGetUniformLocation(program,"source_image"),0);glUniform2f(glGetUniformLocation(program,"destination_extent"),width,height);
    glViewport(0,0,width,height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glUseProgram(0);if(program && (!cache || cache->copy[slot]!=program))glDeleteProgram(program);if(lut)glDeleteTextures(1,&lut);
    for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);free(xr);return result;
}
static int bind_mip_chain(struct st_context *context,const SweetShaderTextureBinding *texture,GLuint *name,SweetMetalShaderCache *cache)
{
    if(!mip_layout_valid(texture))return -EINVAL;
    uint64_t bytes=0;for(unsigned level=0;level<mip_count(texture);++level){SweetShaderTextureBinding view=mip_level(texture,level);bytes+=(uint64_t)view.width*view.height*16;}
    if(bytes>(UINT64_C(256)<<20))return -ENOMEM;
    GLint previousProgram=0,previousFBO=0,previousUnit=0,previousTexture=0;glGetIntegerv(GL_CURRENT_PROGRAM,&previousProgram);glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previousFBO);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&previousUnit);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&previousTexture);
    GLuint fbo=0;int result=-EIO;glGenTextures(1,name);glBindTexture(GL_TEXTURE_2D,*name);glTexStorage2D(GL_TEXTURE_2D,mip_count(texture),GL_RGBA32F,texture->width,texture->height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,mip_count(texture)-1);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    for(unsigned level=0;level<mip_count(texture);++level){SweetShaderTextureBinding view=mip_level(texture,level);GLuint source=0;
        result=xr_format(texture->format)?decode_xr_texture(context,&view,&source,cache):bind_texture(context,&view,&source);
        if(!result){glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,*name,level);
            result=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE?mip_copy_draw(source,view.width,view.height,0,cache):-EIO;}
        if(source)glDeleteTextures(1,&source);if(result)break;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previousFBO);glUseProgram((GLuint)previousProgram);if(fbo)glDeleteFramebuffers(1,&fbo);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,(GLuint)previousTexture);glActiveTexture((GLenum)previousUnit);
    if(result && *name){glDeleteTextures(1,name);*name=0;}
    sweet_drm_log("SWEET METAL MIPMAP sample_chain levels=%u width=%u height=%u shadow_bytes=%llu result=%d GPU_only=1 original_levels=1",mip_count(texture),texture->width,texture->height,(unsigned long long)bytes,result);
    return result;
}
int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *texture,uint32_t *sequence)
{
    if(sequence)*sequence=0;if(!sequence || !mip_layout_valid(texture))return -EINVAL;
    if(mip_count(texture)==1)return 0;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();struct sweetgl_ctx *context=sweetgl_create_metal();int result=-EIO;SweetMetalShaderCache cache={0};
    if(context && sweetgl_make_current(context)){for(unsigned level=1;level<mip_count(texture);++level){GLuint source=0,target=0,fbo=0;
        SweetShaderTextureBinding from=mip_level(texture,level-1),to=mip_level(texture,level);
        result=xr_format(texture->format)?decode_xr_texture(sweetgl_internal_context(context),&from,&source,&cache):bind_texture(sweetgl_internal_context(context),&from,&source);
        if(!result)result=bind_texture(sweetgl_internal_context(context),&to,&target);
        if(!result){glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
            result=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE?mip_copy_draw(source,to.width,to.height,to.format,&cache):-EIO;}
        if(!result)result=sweet_drm_metal_buffer_sequence(to.handle,sequence);
        if(!result)result=sweetgl_metal_buffer_synchronize(to.handle);
        sweet_drm_log("SWEET METAL MIPMAP level=%u src_handle=%u dst_handle=%u width=%u height=%u result=%d sequence=%u original_pages=1",level,from.handle,to.handle,to.width,to.height,result,*sequence);
        if(fbo)glDeleteFramebuffers(1,&fbo);if(source)glDeleteTextures(1,&source);if(target)glDeleteTextures(1,&target);if(result)break;
    }}
    if(context){sweet_metal_shader_cache_dispose(&cache);sweetgl_make_current(NULL);sweetgl_destroy(context);}sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
}
static uint64_t source_hash(const char *source)
{uint64_t hash=UINT64_C(14695981039346656037);for(const unsigned char *p=(const unsigned char *)source;*p;++p)hash=(hash^*p)*UINT64_C(1099511628211);return hash;}
static SweetMetalProgram *render_create_samples(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,unsigned samples,char *error,size_t error_bytes)
{
    if(error && error_bytes)error[0]=0;
    if(!sweet_multisample_count(samples) || !vertex || !fragment || !config || config->vertex_buffers>255 || config->fragment_buffers>255 ||
        (config->fragment_writes&~config->fragment_buffers) || config->reserved || (sweet_render_color_mask(config)&~255u) ||
        !(sweet_render_color_mask(config)&1u) || config->format==252 || config->format==253 || texture_format(config->format)==PIPE_FORMAT_NONE ||
        strnlen(vertex,1<<20)==(1<<20) || strnlen(fragment,1<<20)==(1<<20))return NULL;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();
    SweetMetalProgram *program=calloc(1,sizeof(*program));GLuint shaders[2]={0};GLint ok=0;char *ordered_vertex=NULL,*ordered_fragment=NULL;
    if(!program || !(program->context=sweetgl_create_metal()) || !sweetgl_make_current(program->context))goto failure;
    GLint max_targets=0;glGetIntegerv(GL_MAX_DRAW_BUFFERS,&max_targets);
    render_capabilities(sweetgl_internal_context(program->context));if(render_gl_error("create-capabilities",-1))goto failure;
    for(unsigned i=0;i<8;++i)if(sweet_render_color_mask(config)&(1u<<i)){SweetRenderColorConfig state=sweet_render_color(config,i);
        if(i>=(unsigned)max_targets || state.format==252 || state.format==253 || texture_format(state.format)==PIPE_FORMAT_NONE || state.blend_enabled>1 || state.source_rgb>14 || state.destination_rgb>14 ||
            state.source_alpha>14 || state.destination_alpha>14 || state.operation_rgb>4 || state.operation_alpha>4 || state.write_mask>15){
            if(error && error_bytes)snprintf(error,error_bytes,"Color attachment%u format/blend/mask exceeds actual Mesa limits",i);goto failure;}}
    program->raster_samples=samples;program->ordered_xr_mask=sweet_xr_ordered_mask(config);
    if(samples>1){for(unsigned i=0;i<8;++i)if(sweet_render_color_mask(config)&(1u<<i)){
        SweetRenderColorConfig color=sweet_render_color(config,i);
        if(xr_format(color.format) || !sweetgl_metal_multisample_supported(color.format,samples)){
            if(error && error_bytes)snprintf(error,error_bytes,"Actual A540 sample/format capability is unavailable");goto failure;}}}
    if(program->ordered_xr_mask){struct st_context *context=sweetgl_internal_context(program->context);GLint images=0,units=0;glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS,&images);glGetIntegerv(GL_MAX_IMAGE_UNITS,&units);
        if(!context->screen->is_format_supported(context->screen,PIPE_FORMAT_R32G32B32A32_FLOAT,PIPE_TEXTURE_2D,0,0,PIPE_BIND_SHADER_IMAGE) ||
            !context->screen->get_param(context->screen,PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT) || __builtin_popcount(program->ordered_xr_mask)*2>images ||
            8+32-__builtin_clz(program->ordered_xr_mask)>units){
            if(error && error_bytes)snprintf(error,error_bytes,"Actual Mesa image/attachmentless limits cannot preserve XR blend");goto failure;}
        ordered_vertex=sweet_xr_ordered_vertex(vertex);ordered_fragment=sweet_xr_ordered_fragment(fragment,config,program->ordered_xr_mask);
        if(!ordered_vertex || !ordered_fragment){if(error && error_bytes)snprintf(error,error_bytes,"Actual AIR XR output interface cannot be blended in order");goto failure;}}
    const GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={ordered_vertex?ordered_vertex:vertex,ordered_fragment?ordered_fragment:fragment};
    sweet_metal_program_key(program,vertex,fragment,config);
    // Binary source/config requests presently have no raster-sample field.
    // Keep multi-sample compilation outside that single-sample cache scope.
    bool restored=samples==1 && sweet_metal_program_restore(program);
    if(!restored){
    if(samples==1)sweet_metal_program_request(program,vertex,fragment,config);
    for(unsigned i=0;i<2;++i){const char *stage=i?"fragment":"vertex";shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);
        sweet_drm_log("SWEET AIR RENDER compile_begin stage=%s bytes=%lu hash=%016llx",stage,(unsigned long)strlen(sources[i]),(unsigned long long)source_hash(sources[i]));
        uint64_t wall=render_clock_ns(CLOCK_MONOTONIC),cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);glCompileShader(shaders[i]);
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);render_compile_time(stage,wall,cpu,ok);
        if(!ok){if(error && error_bytes)glGetShaderInfoLog(shaders[i],(GLsizei)error_bytes,NULL,error);goto failure;}}
    program->program=glCreateProgram();glProgramParameteri(program->program,GL_PROGRAM_BINARY_RETRIEVABLE_HINT,GL_TRUE);
    for(unsigned i=0;i<2;++i)glAttachShader(program->program,shaders[i]);
    sweet_drm_log("SWEET AIR RENDER compile_begin stage=link vertex_bytes=%lu fragment_bytes=%lu",(unsigned long)strlen(sources[0]),(unsigned long)strlen(sources[1]));
    uint64_t link_wall=render_clock_ns(CLOCK_MONOTONIC),link_cpu=render_clock_ns(CLOCK_THREAD_CPUTIME_ID);glLinkProgram(program->program);
    glGetProgramiv(program->program,GL_LINK_STATUS,&ok);render_compile_time("link",link_wall,link_cpu,ok);
    if(!ok){if(error && error_bytes)glGetProgramInfoLog(program->program,(GLsizei)error_bytes,NULL,error);goto failure;}
    }
    GLint attributes=0;glGetProgramiv(program->program,GL_ACTIVE_ATTRIBUTES,&attributes);
    for(GLint i=0;i<attributes;++i){char name[256]={0};GLint size=0;GLenum type=0;glGetActiveAttrib(program->program,(GLuint)i,sizeof(name),NULL,&size,&type,name);
        if((strcmp(name,"gl_VertexID") && strcmp(name,"gl_InstanceID")) || glGetAttribLocation(program->program,name)>=0){
            if(error && error_bytes)snprintf(error,error_bytes,"AIR vertex attribute %s requires a descriptor backend",name);goto failure;}}
    for(unsigned i=0;i<2;++i){glDeleteShader(shaders[i]);shaders[i]=0;}program->render=*config;
    program->instance_base=glGetUniformLocation(program->program,"_smtl_instance_base");program->blend_constant=glGetUniformLocation(program->program,"_smtl_blend_constant");
    if(render_gl_error("create-link",-1))goto failure;
    if(!restored && samples==1)sweet_metal_program_store(program,vertex,fragment);
    sweet_drm_log("SWEET AIR RENDER compiled program=%u vertex_mask=%u fragment_mask=%u format=%u vertex_hash=%016llx fragment_hash=%016llx linked=1 robust=1",
        program->program,config->vertex_buffers,config->fragment_buffers,config->format,(unsigned long long)source_hash(vertex),(unsigned long long)source_hash(fragment));
    sweet_drm_log("SWEET METAL MRT pipeline ordered_xr_mask=%u primitive_barrier=1 fp32_image_blend=%u",program->ordered_xr_mask,program->ordered_xr_mask!=0);
    free(ordered_vertex);free(ordered_fragment);sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return program;
failure:
    if(program && program->context){for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program->program)glDeleteProgram(program->program);
        sweetgl_make_current(NULL);sweetgl_destroy(program->context);}free(ordered_vertex);free(ordered_fragment);free(program);sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return NULL;
}
SweetMetalProgram *sweetgl_metal_render_create(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,char *error,size_t error_bytes)
{return render_create_samples(vertex,fragment,config,1,error,error_bytes);}
SweetMetalProgram *sweetgl_metal_render_create_multisample(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,uint32_t samples,char *error,size_t error_bytes)
{if(samples!=2 && samples!=4){if(error && error_bytes)snprintf(error,error_bytes,"Two or four real samples required");return NULL;}return render_create_samples(vertex,fragment,config,samples,error,error_bytes);}
static GLenum blend_factor(uint32_t factor)
{static const GLenum values[]={GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_COLOR,GL_ONE_MINUS_DST_COLOR,GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA,GL_SRC_ALPHA_SATURATE,GL_CONSTANT_COLOR,GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA};
    return factor<sizeof(values)/sizeof(values[0])?values[factor]:0;}
static GLenum blend_operation(uint32_t operation)
{static const GLenum values[]={GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX};return operation<5?values[operation]:0;}
static unsigned metadata_format(uint32_t format)
{switch(format){case 10:return 7;case 30:return 8;case 70:return 1;case 71:return 5;case 80:return 2;case 81:return 6;case 115:return 10;default:return 0;}}
static int bind_texture_metadata(SweetMetalProgram *program,const SweetShaderTextureBinding *textures,uint32_t count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count)
{
    for(unsigned stage=1;stage<=2;++stage){GLuint info[32]={0};
        uint32_t mapping=stage==1?program->render.vertex_texture_slots:program->render.fragment_texture_slots;
        for(unsigned slot=0;slot<8;++slot){unsigned logical=mapping?(mapping>>(slot*4))&15:slot;
            for(unsigned i=0;i<count;++i)if(textures[i].stage==stage && textures[i].index==logical){
                info[slot*4]=textures[i].width;info[slot*4+1]=textures[i].height;info[slot*4+2]=mip_count(&textures[i]);
                info[slot*4+3]=metadata_format(textures[i].format)|(1u<<8);}}
        GLint at=glGetUniformLocation(program->program,stage==1?"_qvertex_texture2d_info":"_qtexture2d_info");if(at>=0)glUniform4uiv(at,8,info);
        GLuint flags[8]={0};for(unsigned i=0;i<sampler_count;++i)if(samplers[i].stage==stage && samplers[i].index<8)
            flags[samplers[i].index]=(samplers[i].address_s==1u?1u:0u)|(samplers[i].address_t==1u?2u:0u);
        at=glGetUniformLocation(program->program,stage==1?"_qvertex_sampler_info":"_qsampler_info");if(at>=0)glUniform1uiv(at,8,flags);
    }
    return glGetError()==GL_NO_ERROR?0:-EIO;
}
static int bind_samples(SweetMetalProgram *program,const SweetShaderTextureBinding *textures,uint32_t count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,GLuint names[32])
{
    GLint active=0,max_length=0;glGetProgramiv(program->program,GL_ACTIVE_UNIFORMS,&active);glGetProgramiv(program->program,GL_ACTIVE_UNIFORM_MAX_LENGTH,&max_length);
    if(max_length>512)return -EINVAL;unsigned assigned=0;GLint available=0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,&available);if(available<1)return -ENOTSUP;
    uint32_t literal_mask[2]={0};SweetRenderConstantSampler literal_states[2][16]={0};
    bool border_clamp=false,anisotropic=false;GLfloat maximum_anisotropy=1;
    GLint extensions=0;glGetIntegerv(GL_NUM_EXTENSIONS,&extensions);
    for(GLint e=0;e<extensions;++e){const char *extension=(const char *)glGetStringi(GL_EXTENSIONS,(GLuint)e);if(!extension)continue;
        if(!strcmp(extension,"GL_EXT_texture_border_clamp") || !strcmp(extension,"GL_OES_texture_border_clamp"))border_clamp=true;
        if(!strcmp(extension,"GL_EXT_texture_filter_anisotropic"))anisotropic=true;}
    if(anisotropic)glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maximum_anisotropy);
    for(GLint i=0;i<active;++i){char name[512]={0};GLsizei bytes=0;GLint elements=0;GLenum type=0;
        glGetActiveUniform(program->program,(GLuint)i,sizeof(name),&bytes,&elements,&type,name);
        unsigned image_binding=~0u;SweetRenderUniformRole role=sweet_render_uniform_role(name,type,elements,&program->render,program->ordered_xr_mask,&image_binding);
        if(role==SWEET_UNIFORM_OWNED)continue;
        if(role==SWEET_UNIFORM_XR_IMAGE){GLint binding=-1,location=glGetUniformLocation(program->program,name);glGetUniformiv(program->program,location,&binding);
            if(binding!=(GLint)image_binding){sweet_drm_log("SWEET METAL MRT uniform reject name=%s type=%#x elements=%d binding=%d expected=%u",name,type,elements,binding,image_binding);return -EINVAL;}
            sweet_drm_log("SWEET METAL MRT uniform image name=%s type=%#x elements=%d binding=%d owned=1",name,type,elements,binding);continue;}
        if(role!=SWEET_UNIFORM_AIR_SAMPLER){sweet_drm_log("SWEET AIR RENDER uniform reject name=%s type=%#x elements=%d ordered_mask=%u",name,type,elements,program->ordered_xr_mask);return -ENOTSUP;}
        SweetRenderSamplerUse use={0};if(!sweet_render_sampler_parse(name,&use))return -ENOTSUP;
        unsigned stage=use.stage,slot=use.texture,unit=0;
        if(elements!=1 || !sweet_render_sampler_unit(assigned,(unsigned)available,&unit))return -EINVAL;
        if(use.literal){unsigned index=use.sampler-8,bank=stage-1;uint32_t bit=1u<<index;
            if((literal_mask[bank]&bit) && memcmp(&literal_states[bank][index],&use.constant,sizeof(use.constant)))return -EINVAL;
            literal_mask[bank]|=bit;literal_states[bank][index]=use.constant;}
        SweetRenderSamplerState state={0};int resolved=sweet_render_sampler_resolve(&use,samplers,sampler_count,&state);if(resolved)return resolved;
        const SweetShaderTextureBinding *texture=NULL;
        uint32_t mapping=stage==1?program->render.vertex_texture_slots:program->render.fragment_texture_slots;
        unsigned logical=mapping?(mapping>>(slot*4))&15:slot;
        for(unsigned j=0;j<count;++j)if(textures[j].index==logical && textures[j].stage==stage){if(texture)return -EINVAL;texture=&textures[j];}
        if(!texture || texture->format==252 || texture->format==253)return -EINVAL;
        glActiveTexture(GL_TEXTURE0+unit);int result=mip_count(texture)>1?bind_mip_chain(sweetgl_internal_context(program->context),texture,&names[assigned],&program->cache):xr_format(texture->format)?
            decode_xr_texture(sweetgl_internal_context(program->context),texture,&names[assigned],&program->cache):
            bind_texture(sweetgl_internal_context(program->context),texture,&names[assigned]);if(result)return result;
        glBindTexture(GL_TEXTURE_2D,names[assigned]);
        result=sweet_render_sampler_parameters(&state,border_clamp,anisotropic,maximum_anisotropy);if(result)return result;
        glUniform1i(glGetUniformLocation(program->program,name),(GLint)unit);++assigned;
        sweet_drm_log("SWEET AIR RENDER sampler stage=%u texture=%u logical=%u unit=%u literal=%u explicit_level=%u min=%u mag=%u mip=%u lod_min=%.6f lod_max=%.6f address_s=%u address_t=%u anisotropy=%u border=%u normalized=%u",
            stage,slot,use.sampler,unit,use.literal,use.explicit_level,state.binding.min_filter,state.binding.mag_filter,state.binding.mip_filter,
            state.binding.minimum_lod,state.binding.maximum_lod,state.binding.address_s,state.binding.address_t,state.max_anisotropy,state.border_colour,state.binding.normalized);
    }
    int result=render_gl_error("sampler-uniforms",-1);return result?result:bind_texture_metadata(program,textures,count,samplers,sampler_count);
}
static int bounded_clear(const SweetRenderPass *pass,SweetMetalShaderCache *cache)
{
    // fd5 fast-clear/GMEM resolve rounds the rectangle into caller padding.
    // A fullscreen shader draw retains ordinary raster/scissor bounds and
    // the driver's system-memory rendering path over these imported pages.
    static const char *vertex="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
    static const char *fragment="#version 310 es\nprecision highp float;\nuniform vec4 colour;layout(location=0) out vec4 out_colour;void main(){out_colour=colour;}";
    unsigned slot=xr_format(pass->target.format)?pass->target.format-551:0;
    GLuint shaders[2]={0},program=cache?cache->clear[slot]:0,vao=0,lut=0;GLint ok=0;int result=-EIO;char *xr_source=NULL;
    bool reused=program!=0;
    const GLenum types[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={vertex,fragment};
    if(!program){
    if(xr_format(pass->target.format)){size_t bytes=strlen(sweet_xr_shader)+512;xr_source=malloc(bytes);if(!xr_source)return -ENOMEM;
        snprintf(xr_source,bytes,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform vec4 colour;layout(location=0) out highp uvec4 out_colour;void main(){out_colour=_smtl_xr_pack(colour,%uu);}",sweet_xr_shader,pass->target.format);sources[1]=xr_source;}
    for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
    program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;
    // These identical clear shaders are owned by this private context's cache.
    // Every call still sets the original colour, viewport/scissor and draws.
    if(cache)cache->clear[slot]=program;
    }
    glUseProgram(program);
    if(bind_xr_lut(program,cache?&cache->lut:&lut))goto done;
    glUniform4f(glGetUniformLocation(program,"colour"),pass->clear[0],pass->clear[1],pass->clear[2],pass->clear[3]);
    glViewport(0,0,pass->target.width,pass->target.height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,pass->target.width,pass->target.height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
    result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glUseProgram(0);if(program && (!cache || cache->clear[slot]!=program))glDeleteProgram(program);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
    if(lut)glDeleteTextures(1,&lut);free(xr_source);
    // All callers hold sweet_metal_shader_gate; keep the additional log bounded.
    static unsigned observations;if(observations<32){++observations;int saved=errno;
        sweet_drm_log("SWEET METAL CLEAR shader format=%u slot=%u reused=%u cached=%u result=%d original_shader=1 original_clear=1",
            pass->target.format,slot,reused,cache && program && cache->clear[slot]==program,result);errno=saved;}
    sweet_drm_log("SWEET AIR RENDER bounded_clear width=%u height=%u shader_draw=1 result=%d",pass->target.width,pass->target.height,result);return result;
}
struct sweet_render_targets {GLuint raw[8],old[8],linear[8],framebuffer,depth;uint32_t mask;
    SweetDepthTiledPlane depthPlane,stencilPlane;struct st_context *context;};
static void retire_targets(struct sweet_render_targets *targets)
{
    if(targets->framebuffer)glDeleteFramebuffers(1,&targets->framebuffer);
    if(targets->depth)glDeleteTextures(1,&targets->depth);
    sweet_depth_tiled_retire(&targets->depthPlane);sweet_depth_tiled_retire(&targets->stencilPlane);
    for(unsigned i=0;i<8;++i){if(targets->raw[i])glDeleteTextures(1,&targets->raw[i]);if(targets->old[i])glDeleteTextures(1,&targets->old[i]);if(targets->linear[i])glDeleteTextures(1,&targets->linear[i]);}
    memset(targets,0,sizeof(*targets));
}
static int snapshot_raw_image(GLuint raw,const SweetShaderTextureBinding *binding,GLuint *copy)
{
    GLuint fbo=0;GLint oldFBO=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&oldFBO);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,raw,0);
    int result=-EIO;if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glActiveTexture(GL_TEXTURE1);glGenTextures(1,copy);glBindTexture(GL_TEXTURE_2D,*copy);
    bool wide=binding->format<554;glTexImage2D(GL_TEXTURE_2D,0,wide?GL_RGBA16UI:GL_R32UI,binding->width,binding->height,0,wide?GL_RGBA_INTEGER:GL_RED_INTEGER,wide?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,binding->width,binding->height);result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)oldFBO);if(fbo)glDeleteFramebuffers(1,&fbo);glActiveTexture(GL_TEXTURE0);return result;
}
static bool color_pass_valid_samples(const SweetRenderPass *pass,unsigned samples)
{
    uint32_t mask=sweet_render_pass_mask(pass);if(pass->reserved || !(mask&1u) || (mask&~255u))return false;
    uint64_t temporary=0;SweetMetalBacking backing;
    for(unsigned i=0;i<8;++i)if(mask&(1u<<i)){SweetRenderColorAttachment a=sweet_render_attachment(pass,i);
        if(a.target.format==252 || a.target.format==253 || a.load_action>2 || a.store_action>1 || (a.target.usage && !(a.target.usage&4)) || a.target.width!=pass->target.width || a.target.height!=pass->target.height ||
            !texture_layout_samples(&a.target,samples,&backing) || (samples>1 && (xr_format(a.target.format) || a.target.mip_count>1 || a.target.reserved)))return false;
        for(unsigned c=0;c<4;++c)if(!isfinite(a.clear[c]))return false;
        if(xr_format(a.target.format))temporary+=(uint64_t)a.target.width*a.target.height*(16+pixel_bytes(a.target.format));
        for(unsigned j=0;j<i;++j)if(mask&(1u<<j)){SweetRenderColorAttachment b=sweet_render_attachment(pass,j);
            if(a.target.handle==b.target.handle){uint64_t end=a.target.offset+(uint64_t)a.target.pitch*a.target.height,bEnd=b.target.offset+(uint64_t)b.target.pitch*b.target.height;
                if(a.target.offset<bEnd && b.target.offset<end)return false;}}
    }return temporary<=(UINT64_C(256)<<20);
}
static int allocate_xr_clear_texture(const SweetShaderTextureBinding *binding,GLuint *texture)
{
    // Clear overwrites every linear colour component. Keep the separate raw
    // snapshot for writeback's unused bits, but do not decode discarded colour.
    GLint previous=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
    glGenTextures(1,texture);glBindTexture(GL_TEXTURE_2D,*texture);
    glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,binding->width,binding->height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    GLenum error=glGetError();int result=*texture && error==GL_NO_ERROR?0:-EIO;
    glBindTexture(GL_TEXTURE_2D,(GLuint)previous);
    if(result && *texture){glDeleteTextures(1,texture);*texture=0;}
    return result;
}
static bool depth_pass_valid_samples(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,unsigned samples)
{
    if(!depth)return true;
    if(depth->target.format!=252 || depth->target.usage!=4 || depth->load_action>2 || depth->store_action>1 ||
        depth->target.width!=pass->target.width || depth->target.height!=pass->target.height || depth->target.mip_count>1 || depth->target.reserved ||
        !isfinite(depth->clear_depth) || depth->clear_depth<0 || depth->clear_depth>1 || !sweet_depth_stencil_valid(&depth->state))return false;
    SweetMetalBacking backing={0};if(!texture_layout_samples(&depth->target,samples,&backing))return false;
    for(unsigned i=0;i<8;++i)if(sweet_render_pass_mask(pass)&(1u<<i)){
        SweetRenderColorAttachment color=sweet_render_attachment(pass,i);
        if(color.target.handle==depth->target.handle){uint64_t cEnd=color.target.offset+(uint64_t)color.target.pitch*color.target.height;
            uint64_t dEnd=depth->target.offset+(uint64_t)depth->target.pitch*depth->target.height;
            if(color.target.offset<dEnd && depth->target.offset<cEnd)return false;}
    }
    return true;
}
static bool stencil_pass_valid_samples(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,unsigned samples)
{
    if(!stencil)return true;
    if(stencil->target.format!=253 || stencil->target.usage!=4 || stencil->target.mip_count>1 || stencil->target.reserved || stencil->reserved ||
        stencil->load_action>2 || stencil->store_action>1 || stencil->clear_stencil>255 || !sweet_depth_stencil_valid(&stencil->state) ||
        stencil->target.width!=pass->target.width || stencil->target.height!=pass->target.height)return false;
    SweetMetalBacking backing={0};if(!texture_layout_samples(&stencil->target,samples,&backing))return false;
    if(depth){uint64_t dEnd=depth->target.offset+(uint64_t)depth->target.pitch*depth->target.height;
        uint64_t sEnd=stencil->target.offset+(uint64_t)stencil->target.pitch*stencil->target.height;
        if(depth->target.handle==stencil->target.handle && depth->target.offset<sEnd && stencil->target.offset<dEnd)return false;
        if(memcmp(&depth->state,&stencil->state,sizeof(depth->state)))return false;}
    for(unsigned i=0;i<8;++i)if(sweet_render_pass_mask(pass)&(1u<<i)){
        SweetRenderColorAttachment color=sweet_render_attachment(pass,i);
        if(color.target.handle==stencil->target.handle){uint64_t cEnd=color.target.offset+(uint64_t)color.target.pitch*color.target.height;
            uint64_t sEnd=stencil->target.offset+(uint64_t)stencil->target.pitch*stencil->target.height;
            if(color.target.offset<sEnd && stencil->target.offset<cEnd)return false;}
    }
    return true;
}
static int prepare_depth_tiled(struct st_context *context,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,
    unsigned samples,struct sweet_render_targets *targets)
{
    struct pipe_resource *primary=NULL,*secondary=NULL;int result=0;
    if(depth){result=sweet_depth_tiled_create(context,&depth->target,samples,depth->load_action,&targets->depthPlane);
        if(result)return result;pipe_resource_reference(&primary,targets->depthPlane.attachment);}
    else{unsigned width=stencil->target.width,height=stencil->target.height;
        struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_Z32_FLOAT,.width0=width,.height0=height,
            .depth0=1,.array_size=1,.nr_samples=samples,.nr_storage_samples=samples,.bind=PIPE_BIND_DEPTH_STENCIL,.usage=PIPE_USAGE_DEFAULT};
        uint64_t modifier=DRM_FORMAT_MOD_LINEAR;primary=context->screen->resource_create_with_modifiers(context->screen,&description,&modifier,1);
        // This unused primary supplies only the original helper's paired format;
        // depth is disabled and has no caller page to read or publish.
    }
    if(!primary)return -ENOMEM;
    if(stencil){result=sweet_depth_tiled_create(context,&stencil->target,samples,stencil->load_action,&targets->stencilPlane);
        if(result)goto done;pipe_resource_reference(&secondary,targets->stencilPlane.attachment);
        if(!sweet_stencil_pair_take_samples(primary,&secondary,samples)){result=-EINVAL;goto done;}}
    if(samples>1)result=describe_multisample_image(context,primary,samples,stencil?GL_DEPTH32F_STENCIL8:GL_DEPTH_COMPONENT32F,
        stencil?MESA_FORMAT_Z32_FLOAT_S8X24_UINT:MESA_FORMAT_Z_FLOAT32,&targets->depth);
    else{glGenTextures(1,&targets->depth);glBindTexture(GL_TEXTURE_2D,targets->depth);
        bool accepted=st_context_teximage(context,GL_TEXTURE_2D,0,primary->format,primary,false);
        if(accepted){struct gl_texture_object *object=_mesa_get_current_tex_object(context->ctx,GL_TEXTURE_2D);_mesa_lock_texture(context->ctx,object);
            struct gl_texture_image *image=_mesa_get_tex_image(context->ctx,object,GL_TEXTURE_2D,0);
            if(!image)accepted=false;else{_mesa_init_teximage_fields(context->ctx,image,primary->width0,primary->height0,1,0,
                stencil?GL_DEPTH32F_STENCIL8:GL_DEPTH_COMPONENT32F,stencil?MESA_FORMAT_Z32_FLOAT_S8X24_UINT:MESA_FORMAT_Z_FLOAT32);
                _mesa_update_texture_object_swizzle(context->ctx,object);_mesa_dirty_texobj(context->ctx,object);}
            _mesa_unlock_texture(context->ctx,object);}
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        result=accepted && glGetError()==GL_NO_ERROR?0:-EIO;
    }
    if(!result){if(depth){SweetMetalBacking backing={0};(void)texture_layout_samples(&depth->target,samples,&backing);
            sweet_drm_log("SWEET METAL DEPTH import handle=%u width=%u height=%u pitch=%u offset=%llu GPU=%#llx original_pages=1",
                depth->target.handle,depth->target.width,depth->target.height,depth->target.pitch,(unsigned long long)depth->target.offset,(unsigned long long)backing.gpu_address);}
        if(stencil){SweetMetalBacking backing={0};(void)texture_layout_samples(&stencil->target,samples,&backing);
            sweet_drm_log("SWEET METAL STENCIL import handle=%u width=%u height=%u pitch=%u offset=%llu GPU=%#llx original_pages=1 separate_z32s8=1",
                stencil->target.handle,stencil->target.width,stencil->target.height,stencil->target.pitch,(unsigned long long)stencil->target.offset,(unsigned long long)backing.gpu_address);}}
done:
    pipe_resource_reference(&secondary,NULL);pipe_resource_reference(&primary,NULL);return result;
}
static int prepare_targets(struct st_context *context,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,unsigned samples,struct sweet_render_targets *targets,SweetMetalShaderCache *cache,uint32_t ordered_xr_mask,unsigned observation)
{
    if(!sweet_multisample_count(samples) || !color_pass_valid_samples(pass,samples) || !depth_pass_valid_samples(pass,depth,samples) || !stencil_pass_valid_samples(pass,depth,stencil,samples))return -EINVAL;targets->mask=sweet_render_pass_mask(pass);
    // Ordered XR uses fragment image side effects. Ordinary late depth tests
    // cannot protect those writes; keep that unimplemented combination rejected.
    if((depth || stencil) && ordered_xr_mask)return -ENOTSUP;
    glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DITHER);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    for(unsigned i=0;i<8;++i)if(targets->mask&(1u<<i)){SweetRenderColorAttachment attachment=sweet_render_attachment(pass,i);
        glActiveTexture(GL_TEXTURE0);struct render_target_observation part=render_target_begin(observation,i,&attachment,"import");
        int result=samples>1?bind_multisample_texture(context,&attachment.target,samples,&targets->raw[i]):bind_texture(context,&attachment.target,&targets->raw[i]);render_target_end(part,result);if(result)return result;
        if(xr_format(attachment.target.format)){part=render_target_begin(observation,i,&attachment,"snapshot");
            result=snapshot_raw_image(targets->raw[i],&attachment.target,&targets->old[i]);render_target_end(part,result);if(result)return result;
            part=render_target_begin(observation,i,&attachment,"linear");
            result=attachment.load_action==2?allocate_xr_clear_texture(&attachment.target,&targets->linear[i]):
                decode_xr_texture_mode(context,&attachment.target,&targets->linear[i],cache,true);render_target_end(part,result);if(result)return result;}
        if(attachment.load_action==2){part=render_target_begin(observation,i,&attachment,"clear");
            GLuint fbo=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,sample_target(samples),targets->linear[i]?targets->linear[i]:targets->raw[i],0);
            SweetRenderPass clear={0};clear.target=attachment.target;clear.load_action=2;clear.store_action=1;memcpy(clear.clear,attachment.clear,sizeof(clear.clear));
            if(targets->linear[i])clear.target.format=115;
            result=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE?bounded_clear(&clear,cache):-EIO;glDeleteFramebuffers(1,&fbo);render_target_end(part,result);if(result)return result;
            if(targets->linear[i])sweet_drm_log("SWEET METAL MRT clear_target index=%u format=%u handle=%u width=%u height=%u result=0 decode_skipped=1 original_code_snapshot=1 original_clear=1",
                i,attachment.target.format,attachment.target.handle,attachment.target.width,attachment.target.height);}
    }
    if(depth || stencil){targets->context=context;int result=prepare_depth_tiled(context,depth,stencil,samples,targets);if(result)return result;
        if(depth && depth->load_action==2){result=sweet_depth32_clear_samples(targets->depth,depth->target.width,depth->target.height,depth->clear_depth,samples,&cache->depth_clear);
            if(result)return result;sweet_drm_log("SWEET METAL DEPTH clear handle=%u width=%u height=%u shader_draw=1 result=0 original_pages=1",
                depth->target.handle,depth->target.width,depth->target.height);}
        if(stencil && stencil->load_action==2){result=sweet_stencil8_clear_samples(targets->depth,stencil->target.width,stencil->target.height,stencil->clear_stencil,samples,&cache->stencil_clear);
            if(result)return result;sweet_drm_log("SWEET METAL STENCIL clear handle=%u width=%u height=%u shader_draw=1 result=0 original_pages=1",
                stencil->target.handle,stencil->target.width,stencil->target.height);}}
    glGenFramebuffers(1,&targets->framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,targets->framebuffer);GLenum buffers[8];unsigned count=0;
    if(depth)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,sample_target(samples),targets->depth,0);
    if(stencil)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,sample_target(samples),targets->depth,0);
    for(unsigned i=0;i<8;++i){buffers[i]=targets->mask&(1u<<i)?GL_COLOR_ATTACHMENT0+i:GL_NONE;
        if(targets->mask&(1u<<i)){count=i+1;if(ordered_xr_mask&(1u<<i))buffers[i]=GL_NONE;
            else glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,sample_target(samples),targets->linear[i]?targets->linear[i]:targets->raw[i],0);}}
    if(ordered_xr_mask==targets->mask){glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_WIDTH,pass->target.width);
        glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_HEIGHT,pass->target.height);
        glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_SAMPLES,0);glReadBuffer(GL_NONE);}
    glDrawBuffers(count,buffers);GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);int result=render_gl_error("prepare-framebuffer",-1);
    if(status!=GL_FRAMEBUFFER_COMPLETE){sweet_drm_log("SWEET METAL MRT framebuffer status=%#x ordered_mask=%u",status,ordered_xr_mask);result=-EIO;}return result;
}
static int writeback_xr(const SweetRenderColorAttachment *attachment,GLuint linear,GLuint original,GLuint raw,uint32_t channels,SweetMetalShaderCache *cache)
{
    unsigned slot=attachment->target.format-552;GLuint shaders[2]={0},program=cache->encode[slot],fbo=0,vao=0;int result=-EIO;char *fragment=NULL;GLint ok=0;
    if(!program){fragment=sweet_xr_writeback_source(attachment->target.format);if(!fragment)return -ENOMEM;
        const GLenum types[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={sweet_mip_vertex,fragment};
        for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
        program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;cache->encode[slot]=program;}
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,raw,0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);if(bind_xr_lut(program,&cache->lut))goto done;
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,linear);glUniform1i(glGetUniformLocation(program,"float_image"),0);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,original);glUniform1i(glGetUniformLocation(program,"original_image"),1);
    glUniform1ui(glGetUniformLocation(program,"channel_mask"),channels);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glViewport(0,0,attachment->target.width,attachment->target.height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,attachment->target.width,attachment->target.height);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);if(fbo)glDeleteFramebuffers(1,&fbo);
    for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program && cache->encode[slot]!=program)glDeleteProgram(program);free(fragment);glActiveTexture(GL_TEXTURE0);
    sweet_drm_log("SWEET METAL MRT writeback format=%u handle=%u mask=%u result=%d GPU_only=1 original_code_snapshot=1",attachment->target.format,attachment->target.handle,channels,result);return result;
}
static int finish_targets(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderPipelineConfig *config,struct sweet_render_targets *targets,SweetMetalShaderCache *cache,uint32_t *sequence)
{
    int result=0;
    if(depth && (config || depth->load_action==2))result=sweet_depth_tiled_copy(targets->context,&targets->depthPlane,true);
    if(!result && stencil && (config || stencil->load_action==2))result=sweet_depth_tiled_copy(targets->context,&targets->stencilPlane,true);
    for(unsigned i=0;i<8 && !result;++i)if(targets->mask&(1u<<i)){SweetRenderColorAttachment a=sweet_render_attachment(pass,i);
        if(!config && a.load_action!=2)continue;
        if(targets->linear[i]){uint32_t mask=config?sweet_render_color(config,i).write_mask:0;
            if(a.load_action==2)mask=15;result=writeback_xr(&a,targets->linear[i],targets->old[i],targets->raw[i],mask,cache);}
        if(!result){uint32_t completed=0;result=sweet_drm_metal_buffer_sequence(a.target.handle,&completed);if(completed>*sequence)*sequence=completed;
            if(!result)result=sweetgl_metal_buffer_synchronize(a.target.handle);
            sweet_drm_log("SWEET METAL MRT retired index=%u handle=%u format=%u result=%d sequence=%u original_pages=1",i,a.target.handle,a.target.format,result,completed);}
    }
    if(!result && depth && (config || depth->load_action==2)){
        uint32_t completed=0;result=sweet_drm_metal_buffer_sequence(depth->target.handle,&completed);if(completed>*sequence)*sequence=completed;
        if(!result)result=sweetgl_metal_buffer_synchronize(depth->target.handle);
        sweet_drm_log("SWEET METAL DEPTH retired handle=%u result=%d sequence=%u original_pages=1",depth->target.handle,result,completed);
    }
    if(!result && stencil && (config || stencil->load_action==2)){
        uint32_t completed=0;result=sweet_drm_metal_buffer_sequence(stencil->target.handle,&completed);if(completed>*sequence)*sequence=completed;
        if(!result)result=sweetgl_metal_buffer_synchronize(stencil->target.handle);
        sweet_drm_log("SWEET METAL STENCIL retired handle=%u result=%d sequence=%u original_pages=1",stencil->target.handle,result,completed);
    }
    return result;
}
static int target_blend_state(struct st_context *context,const SweetRenderPipelineConfig *config,const SweetRenderDraw *draw,uint32_t ordered_xr_mask)
{
    glBlendColor(draw->blend_color[0],draw->blend_color[1],draw->blend_color[2],draw->blend_color[3]);
    for(unsigned i=0;i<context->ctx->Const.MaxDrawBuffers && i<8;++i){SweetRenderColorConfig state=sweet_render_color(config,i);bool enabled=(sweet_render_color_mask(config)&(1u<<i)) && state.blend_enabled && !(ordered_xr_mask&(1u<<i));
        _mesa_set_enablei(context->ctx,GL_BLEND,i,enabled?GL_TRUE:GL_FALSE);
        _mesa_BlendFuncSeparateiARB(i,blend_factor(state.source_rgb),blend_factor(state.destination_rgb),blend_factor(state.source_alpha),blend_factor(state.destination_alpha));
        _mesa_BlendEquationSeparateiARB(i,blend_operation(state.operation_rgb),blend_operation(state.operation_alpha));
        uint32_t writes=state.write_mask;_mesa_ColorMaski(i,writes&8?GL_TRUE:GL_FALSE,writes&4?GL_TRUE:GL_FALSE,writes&2?GL_TRUE:GL_FALSE,writes&1?GL_TRUE:GL_FALSE);
        if(render_gl_error("blend-state",i))return -EIO;}
    return 0;
}
static int render_draw_with_depth(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,unsigned samples,uint32_t sample_mask,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence)
{
    if(sequence)*sequence=0;
    if(!sweet_multisample_count(samples) || !program || program->raster_samples!=samples || !pass || !draw || !sequence || count>16 || texture_count>32 || sampler_count>16 || draw->primitive>4 || !draw->count ||
        draw->first>1048576 || draw->count>1048576-draw->first || !draw->instances || draw->instances>65535 ||
        (uint64_t)draw->count*draw->instances>16777216 || program->render.format!=pass->target.format ||
        sweet_render_color_mask(&program->render)!=sweet_render_pass_mask(pass) || !color_pass_valid_samples(pass,samples) || !depth_pass_valid_samples(pass,depth,samples) || !stencil_pass_valid_samples(pass,depth,stencil,samples) ||
        (count && !buffers) || (texture_count && !textures) || (sampler_count && !samplers) ||
        !isfinite(draw->x) || !isfinite(draw->y) || !isfinite(draw->width) || !isfinite(draw->height) ||
        !isfinite(draw->znear) || !isfinite(draw->zfar) || draw->znear<0 || draw->znear>1 || draw->zfar<0 || draw->zfar>1 ||
        draw->width<=0 || draw->height<=0 || draw->x<0 || draw->y<0 || draw->x>pass->target.width || draw->width>pass->target.width-draw->x ||
        draw->y>pass->target.height || draw->height>pass->target.height-draw->y ||
        draw->x!=floor(draw->x) || draw->y!=floor(draw->y) || draw->width!=floor(draw->width) || draw->height!=floor(draw->height) ||
        draw->winding>1 || draw->cull>2 ||
        draw->scissor_x>pass->target.width || draw->scissor_width>pass->target.width-draw->scissor_x ||
        draw->scissor_y>pass->target.height || draw->scissor_height>pass->target.height-draw->scissor_y)return -EINVAL;
    uint32_t mask=0;for(unsigned i=0;i<count;++i){if(buffers[i].index>=16 || mask&(1u<<buffers[i].index))return -EINVAL;mask|=1u<<buffers[i].index;}
    if(mask!=(program->render.vertex_buffers|(program->render.fragment_buffers<<8)))return -EINVAL;
    for(unsigned i=0;i<8;++i)if(sweet_render_pass_mask(pass)&(1u<<i)){
        if(sweet_render_color(&program->render,i).format!=sweet_render_attachment(pass,i).target.format)return -EINVAL;
        for(unsigned b=0;b<count;++b)if(buffers[b].handle==sweet_render_attachment(pass,i).target.handle)return -EINVAL;
        for(unsigned t=0;t<texture_count;++t)if(textures[t].handle==sweet_render_attachment(pass,i).target.handle)return -EINVAL;}
    for(unsigned i=0;i<4;++i)if(!isfinite(draw->blend_color[i]))return -EINVAL;
    if(depth){for(unsigned i=0;i<count;++i)if(buffers[i].handle==depth->target.handle)return -EINVAL;
        for(unsigned i=0;i<texture_count;++i)if(textures[i].handle==depth->target.handle)return -EINVAL;
        if(program->ordered_xr_mask)return -ENOTSUP;}
    if(stencil){for(unsigned i=0;i<count;++i)if(buffers[i].handle==stencil->target.handle)return -EINVAL;
        for(unsigned i=0;i<texture_count;++i)if(textures[i].handle==stencil->target.handle)return -EINVAL;
        if(program->ordered_xr_mask)return -ENOTSUP;}
    for(unsigned i=0;i<texture_count;++i){if((textures[i].usage && !(textures[i].usage&1)) || textures[i].stage<1 || textures[i].stage>2 || textures[i].index>=16 ||
        textures[i].handle==pass->target.handle)return -EINVAL;
        for(unsigned j=0;j<i;++j)if(textures[j].stage==textures[i].stage && textures[j].index==textures[i].index)return -EINVAL;}
    unsigned observation=render_observation_call();
    if(observation){int saved=errno;sweet_drm_log("SWEET RENDER REQUEST call=%u width=%u height=%u primitive=%u first=%u count=%u instances=%u ordered_mask=%u buffers=%u textures=%u samplers=%u metadata_only=1",
        observation,pass->target.width,pass->target.height,draw->primitive,draw->first,draw->count,draw->instances,program->ordered_xr_mask,count,texture_count,sampler_count);errno=saved;}
    struct render_stage_observation stage=render_stage_begin(observation,"gate");
    pthread_mutex_lock(&sweet_metal_shader_gate);render_stage_end(stage,0);
    struct st_context *previous=st_api_get_current();GLuint buffer_names[16]={0},sample_names[32]={0},vao=0,lut=0;int result=0;
    struct sweet_render_targets targets={0};bool images_bound=false;
    stage=render_stage_begin(observation,"make_current");bool current=sweetgl_make_current(program->context);
    render_stage_end(stage,current?0:-EIO);if(!current){result=-EIO;goto done;}
    if(render_gl_error("draw-entry",-1)){result=-EIO;goto done;}
    stage=render_stage_begin(observation,"targets");result=prepare_targets(sweetgl_internal_context(program->context),pass,depth,stencil,samples,&targets,&program->cache,program->ordered_xr_mask,observation);
    render_stage_end(stage,result);if(result)goto done;
    stage=render_stage_begin(observation,"buffers");
    glUseProgram(program->program);for(unsigned i=0;i<count && !result;++i)result=buffers[i].index<8?
        bind_vertex_words(sweetgl_internal_context(program->context),&buffers[i],program->program,&buffer_names[i]):
        sweet_metal_bind_buffer(sweetgl_internal_context(program->context),&buffers[i],&buffer_names[i]);
    render_stage_end(stage,result);
    if(!result){stage=render_stage_begin(observation,"samples");result=bind_samples(program,textures,texture_count,samplers,sampler_count,sample_names);render_stage_end(stage,result);}if(result)goto done;
    stage=render_stage_begin(observation,"xr_lut");result=bind_xr_lut(program->program,&lut);render_stage_end(stage,result);if(result)goto done;
    stage=render_stage_begin(observation,"state");
    glViewport(draw->x,draw->y,draw->width,draw->height);glDepthRangef(draw->znear,draw->zfar);
    glEnable(GL_SCISSOR_TEST);glScissor(draw->scissor_x,draw->scissor_y,draw->scissor_width,draw->scissor_height);
    if(draw->cull){if(draw->cull>2){result=-EINVAL;goto done;}glEnable(GL_CULL_FACE);glCullFace(draw->cull==1?GL_FRONT:GL_BACK);}else glDisable(GL_CULL_FACE);
    // y inversion reverses clip-space winding while row0 remains Metal row0.
    glFrontFace(draw->winding==1?GL_CW:GL_CCW);
    if(depth || stencil){const SweetDepthStencilState *state=stencil?&stencil->state:&depth->state;
        if(!sweet_depth_stencil_apply(state,depth!=NULL,stencil!=NULL)){result=-EINVAL;goto done;}}
    else {SweetDepthStencilState none=sweet_depth_stencil_default();(void)sweet_depth_stencil_apply(&none,false,false);}
    result=target_blend_state(sweetgl_internal_context(program->context),&program->render,draw,program->ordered_xr_mask);
    if(samples>1){glEnable(GL_SAMPLE_MASK);glSampleMaski(0,sample_mask&((1u<<samples)-1u));}
    render_stage_end(stage,result);if(result)goto done;
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);static const GLenum primitive[]={GL_POINTS,GL_LINES,GL_LINE_STRIP,GL_TRIANGLES,GL_TRIANGLE_STRIP};
    stage=render_stage_begin(observation,"submit");
    if(program->ordered_xr_mask){
        glUniform4f(program->blend_constant,draw->blend_color[0],draw->blend_color[1],draw->blend_color[2],draw->blend_color[3]);
        images_bound=true;for(unsigned i=0;i<8;++i)if(program->ordered_xr_mask&(1u<<i)){
            glBindImageTexture(i,targets.linear[i],0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8+i,targets.linear[i],0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);}
        glMemoryBarrier(GL_ALL_BARRIER_BITS);if(render_gl_error("image-bind",-1)){result=-EIO;goto done;}
        uint32_t n=sweet_ordered_primitive_count(draw->primitive,draw->count);
        for(uint32_t instance=0;instance<draw->instances && !result;++instance){glUniform1i(program->instance_base,instance);
            for(uint32_t index=0;index<n;++index){SweetOrderedPrimitive p=sweet_ordered_primitive(draw->primitive,draw->first,index);
                glFrontFace((draw->winding==1)!=p.reverse_winding?GL_CW:GL_CCW);
                glDrawArraysInstanced(primitive[p.mode],p.first,p.count,1);glMemoryBarrier(GL_ALL_BARRIER_BITS);
                if(render_gl_error("ordered-primitive",(int)index)){result=-EIO;break;}}
        }
        if(result)goto done;
        sweet_drm_log("SWEET METAL MRT ordered_draw image_mask=%u primitives=%u instances=%u fp32=1 per_primitive_XR_round=1 image_barriers=1",program->ordered_xr_mask,n,draw->instances);
    }else{glDrawArraysInstanced(primitive[draw->primitive],(GLint)draw->first,(GLsizei)draw->count,(GLsizei)draw->instances);glMemoryBarrier(GL_ALL_BARRIER_BITS);}
    render_stage_end(stage,result);
    stage=render_stage_begin(observation,"gpu_finish");glFinish();result=render_gl_error("draw-finish",-1);render_stage_end(stage,result);if(result)goto done;
    stage=render_stage_begin(observation,"writeback");result=finish_targets(pass,depth,stencil,&program->render,&targets,&program->cache,sequence);render_stage_end(stage,result);
    stage=render_stage_begin(observation,"fragment_sync");
    for(unsigned i=0;i<count && !result;++i)if(buffers[i].index>=8 && (program->render.fragment_writes&(1u<<(buffers[i].index-8))))
        result=sweetgl_metal_buffer_synchronize(buffers[i].handle);
    render_stage_end(stage,result);
done:
    stage=render_stage_begin(observation,"retire");
    if(images_bound)for(unsigned i=0;i<8;++i)if(program->ordered_xr_mask&(1u<<i)){
        glBindImageTexture(i,0,0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8+i,0,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);}
    if(vao)glDeleteVertexArrays(1,&vao);retire_targets(&targets);
    if(lut)glDeleteTextures(1,&lut);
    for(unsigned i=0;i<32;++i)if(sample_names[i])glDeleteTextures(1,&sample_names[i]);for(unsigned i=0;i<count;++i)if(buffer_names[i]){
        if(buffers[i].index<8)glDeleteTextures(1,&buffer_names[i]);else glDeleteBuffers(1,&buffer_names[i]);}
    sweet_drm_log("SWEET AIR RENDER draw width=%u height=%u primitive=%u first=%u count=%u instances=%u target_handle=%u result=%d sequence=%u original_pages=1",
        pass->target.width,pass->target.height,draw->primitive,draw->first,draw->count,draw->instances,pass->target.handle,result,*sequence);
    sweet_drm_log("SWEET METAL MRT draw color_mask=%u result=%d sequence=%u original_pages=1",sweet_render_pass_mask(pass),result,*sequence);
    if(samples>1)sweet_drm_log("SWEET MSAA draw samples=%u mask=%u color=%u depth=%u stencil=%u result=%d sequence=%u original_pages=1",
        samples,sample_mask,pass->target.handle,depth?depth->target.handle:0,stencil?stencil->target.handle:0,result,*sequence);
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);render_stage_end(stage,result);return result;
}
int sweetgl_metal_render_draw(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence)
{return render_draw_with_depth(program,pass,NULL,NULL,1,UINT32_MAX,draw,buffers,count,textures,texture_count,samplers,sampler_count,sequence);}
int sweetgl_metal_render_draw_depth(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence)
{if(!depth){if(sequence)*sequence=0;return -EINVAL;}return render_draw_with_depth(program,pass,depth,NULL,1,UINT32_MAX,draw,buffers,count,textures,texture_count,samplers,sampler_count,sequence);}
int sweetgl_metal_render_draw_stencil(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence)
{if(!stencil){if(sequence)*sequence=0;return -EINVAL;}return render_draw_with_depth(program,pass,depth,stencil,1,UINT32_MAX,draw,buffers,count,textures,texture_count,samplers,sampler_count,sequence);}
int sweetgl_metal_render_draw_multisample(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t samples,uint32_t sample_mask,uint32_t *sequence)
{if(samples!=2 && samples!=4){if(sequence)*sequence=0;return -EINVAL;}return render_draw_with_depth(program,pass,depth,stencil,samples,sample_mask,draw,buffers,count,textures,texture_count,samplers,sampler_count,sequence);}
static int render_clear_with_depth(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,unsigned samples,uint32_t *sequence)
{
    if(sequence)*sequence=0;if(!pass || !sequence)return -EINVAL;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();struct sweetgl_ctx *context=sweetgl_create_metal();
    struct sweet_render_targets targets={0};SweetMetalShaderCache cache={0};int result=-EIO;
    if(context && sweetgl_make_current(context)){result=prepare_targets(sweetgl_internal_context(context),pass,depth,stencil,samples,&targets,&cache,0,0);
        bool any=(depth && depth->load_action==2)||(stencil && stencil->load_action==2);for(unsigned i=0;i<8;++i)if(sweet_render_pass_mask(pass)&(1u<<i))any|=sweet_render_attachment(pass,i).load_action==2;
        if(!result && any){glFinish();result=finish_targets(pass,depth,stencil,NULL,&targets,&cache,sequence);}else if(!result)result=-ENOTSUP;
        retire_targets(&targets);sweet_metal_shader_cache_dispose(&cache);}
    if(context){sweetgl_make_current(NULL);sweetgl_destroy(context);}
    if(samples>1)sweet_drm_log("SWEET MSAA clear samples=%u color=%u depth=%u stencil=%u result=%d sequence=%u original_pages=1",
        samples,pass->target.handle,depth?depth->target.handle:0,stencil?stencil->target.handle:0,result,*sequence);
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
}
int sweetgl_metal_render_clear(const SweetRenderPass *pass,uint32_t *sequence){return render_clear_with_depth(pass,NULL,NULL,1,sequence);}
int sweetgl_metal_render_clear_depth(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,uint32_t *sequence)
{if(!depth){if(sequence)*sequence=0;return -EINVAL;}return render_clear_with_depth(pass,depth,NULL,1,sequence);}
int sweetgl_metal_render_clear_stencil(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,uint32_t *sequence)
{if(!stencil){if(sequence)*sequence=0;return -EINVAL;}return render_clear_with_depth(pass,depth,stencil,1,sequence);}
int sweetgl_metal_render_clear_multisample(const SweetRenderPass *pass,const SweetRenderDepthPass *depth,const SweetRenderStencilPass *stencil,uint32_t samples,uint32_t *sequence)
{if(samples!=2 && samples!=4){if(sequence)*sequence=0;return -EINVAL;}return render_clear_with_depth(pass,depth,stencil,samples,sequence);}
int sweetgl_metal_multisample_resolve(const SweetMultisampleResolve *request,uint32_t *sequence)
{
    if(sequence)*sequence=0;
    if(!request || !sequence || request->reserved || request->aspect>1 || (request->samples!=2 && request->samples!=4) ||
        (request->aspect==0 && request->filter) || request->filter>2 || request->source.format!=request->target.format ||
        request->source.width!=request->target.width || request->source.height!=request->target.height ||
        request->source.usage!=4 || (request->target.usage && !(request->target.usage&4)) || (request->target.usage&~5u) || request->source.mip_count>1 || request->target.mip_count>1 ||
        request->source.reserved || request->target.reserved || (request->aspect==1)!=(request->source.format==252) ||
        request->source.format==253 || xr_format(request->source.format))return -EINVAL;
    SweetMetalBacking sourceBacking={0},targetBacking={0};
    if(!texture_layout_samples(&request->source,request->samples,&sourceBacking) || !texture_layout(&request->target,&targetBacking))return -EINVAL;
    if(request->source.handle==request->target.handle){uint64_t sourceEnd=request->source.offset+(uint64_t)request->source.pitch*request->source.height;
        uint64_t targetEnd=request->target.offset+(uint64_t)request->target.pitch*request->target.height;
        if(request->source.offset<targetEnd && request->target.offset<sourceEnd)return -EINVAL;}
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();struct sweetgl_ctx *context=sweetgl_create_metal();
    GLuint source=0,target=0;int result=-EIO;struct sweet_render_targets tiled={0};
    if(context && sweetgl_make_current(context)){
        result=bind_multisample_texture(sweetgl_internal_context(context),&request->source,request->samples,&source);
        if(!result && request->aspect){SweetRenderDepthPass pass={0};pass.target=request->target;pass.load_action=0;pass.store_action=1;
            tiled.context=sweetgl_internal_context(context);result=prepare_depth_tiled(tiled.context,&pass,NULL,1,&tiled);target=tiled.depth;}
        else if(!result)result=bind_texture(sweetgl_internal_context(context),&request->target,&target);
        if(!result)result=sweet_multisample_resolve_draw(source,target,request->source.width,request->source.height,request->samples,request->aspect,request->filter);
        if(!result && request->aspect)result=sweet_depth_tiled_copy(tiled.context,&tiled.depthPlane,true);
        if(!result)result=sweet_drm_metal_buffer_sequence(request->target.handle,sequence);
        if(!result)result=sweetgl_metal_buffer_synchronize(request->target.handle);
        if(source)glDeleteTextures(1,&source);if(!request->aspect && target)glDeleteTextures(1,&target);retire_targets(&tiled);
    }
    if(context){sweetgl_make_current(NULL);sweetgl_destroy(context);}sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);
    sweet_drm_log("SWEET MSAA resolve src=%u dst=%u samples=%u aspect=%u filter=%u result=%d sequence=%u original_pages=1 GPU_only=1",
        request->source.handle,request->target.handle,request->samples,request->aspect,request->filter,result,*sequence);return result;
}
