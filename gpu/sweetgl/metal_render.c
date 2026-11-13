/* AIR vertex/fragment programs render on original linear Metal backing pages. */
#include "MetalShaderInternals.h"
#include "MetalBackingRules.h"
#include "sweetgl.h"
#include "sweet_drm.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "frontend/winsys_handle.h"
#include "util/u_inlines.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_manager.h"
#include "freedreno/freedreno_resource.h"
#include "drm-uapi/drm_fourcc.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
static enum pipe_format texture_format(uint32_t format)
{
    switch(format){case 10:return PIPE_FORMAT_R8_UNORM;case 30:return PIPE_FORMAT_R8G8_UNORM;
    case 70:return PIPE_FORMAT_R8G8B8A8_UNORM;case 71:return PIPE_FORMAT_R8G8B8A8_SRGB;
    case 80:return PIPE_FORMAT_B8G8R8A8_UNORM;case 81:return PIPE_FORMAT_B8G8R8A8_SRGB;
    case 115:return PIPE_FORMAT_R16G16B16A16_FLOAT;default:return PIPE_FORMAT_NONE;}
}
static bool texture_layout(const SweetShaderTextureBinding *texture,SweetMetalBacking *backing)
{
    enum pipe_format format=texture_format(texture?texture->format:0);
    if(!texture || format==PIPE_FORMAT_NONE || !texture->width || !texture->height || texture->width>16384 || texture->height>16384 ||
        (texture->offset&63) || (texture->pitch&63) || sweet_drm_metal_buffer_view(texture->handle,backing))return false;
    unsigned bytes=texture->format==10?1:texture->format==30?2:texture->format==115?8:4;
    return texture->pitch>=texture->width*bytes && texture->offset<=backing->logical_bytes &&
        (uint64_t)texture->pitch*texture->height<=backing->logical_bytes-texture->offset;
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
static uint64_t source_hash(const char *source)
{uint64_t hash=UINT64_C(14695981039346656037);for(const unsigned char *p=(const unsigned char *)source;*p;++p)hash=(hash^*p)*UINT64_C(1099511628211);return hash;}
SweetMetalProgram *sweetgl_metal_render_create(const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config,char *error,size_t error_bytes)
{
    if(error && error_bytes)error[0]=0;
    if(!vertex || !fragment || !config || config->vertex_buffers>255 || config->fragment_buffers>255 ||
        (config->fragment_writes&~config->fragment_buffers) || texture_format(config->format)==PIPE_FORMAT_NONE ||
        strnlen(vertex,1<<20)==(1<<20) || strnlen(fragment,1<<20)==(1<<20))return NULL;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();
    SweetMetalProgram *program=calloc(1,sizeof(*program));GLuint shaders[2]={0};GLint ok=0;
    if(!program || !(program->context=sweetgl_create_metal()) || !sweetgl_make_current(program->context))goto failure;
    const GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={vertex,fragment};
    for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok){if(error && error_bytes)glGetShaderInfoLog(shaders[i],(GLsizei)error_bytes,NULL,error);goto failure;}}
    program->program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program->program,shaders[i]);glLinkProgram(program->program);
    glGetProgramiv(program->program,GL_LINK_STATUS,&ok);if(!ok){if(error && error_bytes)glGetProgramInfoLog(program->program,(GLsizei)error_bytes,NULL,error);goto failure;}
    GLint attributes=0;glGetProgramiv(program->program,GL_ACTIVE_ATTRIBUTES,&attributes);
    for(GLint i=0;i<attributes;++i){char name[256]={0};GLint size=0;GLenum type=0;glGetActiveAttrib(program->program,(GLuint)i,sizeof(name),NULL,&size,&type,name);
        if((strcmp(name,"gl_VertexID") && strcmp(name,"gl_InstanceID")) || glGetAttribLocation(program->program,name)>=0){
            if(error && error_bytes)snprintf(error,error_bytes,"AIR vertex attribute %s requires a descriptor backend",name);goto failure;}}
    for(unsigned i=0;i<2;++i)glDeleteShader(shaders[i]);program->render=*config;
    sweet_drm_log("SWEET AIR RENDER compiled program=%u vertex_mask=%u fragment_mask=%u format=%u vertex_hash=%016llx fragment_hash=%016llx linked=1 robust=1",
        program->program,config->vertex_buffers,config->fragment_buffers,config->format,(unsigned long long)source_hash(vertex),(unsigned long long)source_hash(fragment));
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return program;
failure:
    if(program && program->context){for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program->program)glDeleteProgram(program->program);
        sweetgl_make_current(NULL);sweetgl_destroy(program->context);}free(program);sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return NULL;
}
static GLenum blend_factor(uint32_t factor)
{static const GLenum values[]={GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_COLOR,GL_ONE_MINUS_DST_COLOR,GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA,GL_SRC_ALPHA_SATURATE,GL_CONSTANT_COLOR,GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA};
    return factor<sizeof(values)/sizeof(values[0])?values[factor]:0;}
static GLenum blend_operation(uint32_t operation)
{static const GLenum values[]={GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX};return operation<5?values[operation]:0;}
static GLenum address_mode(uint32_t mode)
{return mode==0?GL_CLAMP_TO_EDGE:mode==2?GL_REPEAT:mode==3?GL_MIRRORED_REPEAT:0;}
static unsigned metadata_format(uint32_t format)
{switch(format){case 10:return 7;case 30:return 8;case 70:return 1;case 71:return 5;case 80:return 2;case 81:return 6;case 115:return 10;default:return 0;}}
static int bind_texture_metadata(SweetMetalProgram *program,const SweetShaderTextureBinding *textures,uint32_t count)
{
    for(unsigned stage=1;stage<=2;++stage){GLuint info[32]={0};
        uint32_t mapping=stage==1?program->render.vertex_texture_slots:program->render.fragment_texture_slots;
        for(unsigned slot=0;slot<8;++slot){unsigned logical=mapping?(mapping>>(slot*4))&15:slot;
            for(unsigned i=0;i<count;++i)if(textures[i].stage==stage && textures[i].index==logical){
                info[slot*4]=textures[i].width;info[slot*4+1]=textures[i].height;info[slot*4+2]=1;
                info[slot*4+3]=metadata_format(textures[i].format)|(1u<<8);}}
        GLint at=glGetUniformLocation(program->program,stage==1?"_qvertex_texture2d_info":"_qtexture2d_info");if(at>=0)glUniform4uiv(at,8,info);
        GLuint flags[8]={0};at=glGetUniformLocation(program->program,stage==1?"_qvertex_sampler_info":"_qsampler_info");if(at>=0)glUniform1uiv(at,8,flags);
    }
    return glGetError()==GL_NO_ERROR?0:-EIO;
}
static int bind_samples(SweetMetalProgram *program,const SweetShaderTextureBinding *textures,uint32_t count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,GLuint names[32])
{
    GLint active=0,max_length=0;glGetProgramiv(program->program,GL_ACTIVE_UNIFORMS,&active);glGetProgramiv(program->program,GL_ACTIVE_UNIFORM_MAX_LENGTH,&max_length);
    if(max_length>512)return -EINVAL;unsigned assigned=0;
    for(GLint i=0;i<active;++i){char name[512]={0};GLsizei bytes=0;GLint elements=0;GLenum type=0;
        glGetActiveUniform(program->program,(GLuint)i,sizeof(name),&bytes,&elements,&type,name);
        if(type!=GL_SAMPLER_2D){
            if(!strncmp(name,"_smtl_vertex_buffer",19))continue;
            if(!strncmp(name,"_qtexture2d_info",16) || !strncmp(name,"_qvertex_texture2d_info",23) ||
                !strncmp(name,"_qsampler_info",14) || !strncmp(name,"_qvertex_sampler_info",21))continue;
            return -ENOTSUP;
        }
        unsigned stage=2,slot=0,sampler=0;int used=0;
        if(sscanf(name,"_qsample%u_%u%n",&slot,&sampler,&used)!=2){stage=1;used=0;if(sscanf(name,"_qvertex_sample%u_%u%n",&slot,&sampler,&used)!=2)return -ENOTSUP;}
        if(name[used] && strcmp(name+used,"_level"))return -ENOTSUP;
        if(elements!=1 || assigned>=8 || sampler>=8 || slot>=8)return -EINVAL;
        const SweetShaderTextureBinding *texture=NULL;const SweetShaderSamplerBinding *state=NULL;
        uint32_t mapping=stage==1?program->render.vertex_texture_slots:program->render.fragment_texture_slots;
        unsigned logical=mapping?(mapping>>(slot*4))&15:slot;
        for(unsigned j=0;j<count;++j)if(textures[j].index==logical && textures[j].stage==stage){if(texture)return -EINVAL;texture=&textures[j];}
        for(unsigned j=0;j<sampler_count;++j)if(samplers[j].index==sampler && samplers[j].stage==stage){if(state)return -EINVAL;state=&samplers[j];}
        if(!texture || !state || !state->normalized || state->min_filter>1 || state->mag_filter>1 || state->mip_filter!=0 || !address_mode(state->address_s) || !address_mode(state->address_t))return -ENOTSUP;
        glActiveTexture(GL_TEXTURE0+assigned);int result=bind_texture(sweetgl_internal_context(program->context),texture,&names[assigned]);if(result)return result;
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,state->min_filter?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,state->mag_filter?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,address_mode(state->address_s));glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,address_mode(state->address_t));
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_LOD,state->minimum_lod);glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_LOD,state->maximum_lod);
        glUniform1i(glGetUniformLocation(program->program,name),(GLint)assigned++);
    }
    return bind_texture_metadata(program,textures,count);
}
static int bounded_clear(const SweetRenderPass *pass)
{
    // fd5 fast-clear/GMEM resolve rounds the rectangle into caller padding.
    // A fullscreen shader draw retains ordinary raster/scissor bounds and
    // the driver's system-memory rendering path over these imported pages.
    static const char *vertex="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
    static const char *fragment="#version 310 es\nprecision highp float;\nuniform vec4 colour;layout(location=0) out vec4 out_colour;void main(){out_colour=colour;}";
    GLuint shaders[2]={0},program=0,vao=0;GLint ok=0;int result=-EIO;
    const GLenum types[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[2]={vertex,fragment};
    for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
    program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;glUseProgram(program);
    glUniform4f(glGetUniformLocation(program,"colour"),pass->clear[0],pass->clear[1],pass->clear[2],pass->clear[3]);
    glViewport(0,0,pass->target.width,pass->target.height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,pass->target.width,pass->target.height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
    result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glUseProgram(0);if(program)glDeleteProgram(program);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
    sweet_drm_log("SWEET AIR RENDER bounded_clear width=%u height=%u shader_draw=1 result=%d",pass->target.width,pass->target.height,result);return result;
}
static int prepare_pass(struct st_context *st,const SweetRenderPass *pass,GLuint *texture,GLuint *framebuffer)
{
    if(pass->load_action>2 || (pass->store_action!=0 && pass->store_action!=1) || !(pass->target.usage&4))return -EINVAL;
    glActiveTexture(GL_TEXTURE0);int result=bind_texture(st,&pass->target,texture);if(result)return result;
    glGenFramebuffers(1,framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,*framebuffer);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,*texture,0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return -EIO;
    glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_DITHER);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    if(pass->load_action==2){result=bounded_clear(pass);if(result)return result;}
    return glGetError()==GL_NO_ERROR?0:-EIO;
}
int sweetgl_metal_render_draw(SweetMetalProgram *program,const SweetRenderPass *pass,const SweetRenderDraw *draw,
    const SweetShaderBufferBinding *buffers,uint32_t count,const SweetShaderTextureBinding *textures,uint32_t texture_count,
    const SweetShaderSamplerBinding *samplers,uint32_t sampler_count,uint32_t *sequence)
{
    if(sequence)*sequence=0;
    if(!program || !pass || !draw || !sequence || count>16 || texture_count>32 || sampler_count>16 || draw->primitive>4 || !draw->count ||
        draw->first>1048576 || draw->count>1048576-draw->first || !draw->instances || draw->instances>65535 ||
        (uint64_t)draw->count*draw->instances>16777216 || program->render.format!=pass->target.format ||
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
    for(unsigned i=0;i<texture_count;++i){if(!(textures[i].usage&1) || textures[i].stage<1 || textures[i].stage>2 || textures[i].index>=16 ||
        textures[i].handle==pass->target.handle)return -EINVAL;
        for(unsigned j=0;j<i;++j)if(textures[j].stage==textures[i].stage && textures[j].index==textures[i].index)return -EINVAL;}
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();GLuint buffer_names[16]={0},samples[32]={0},target=0,framebuffer=0,vao=0;int result=0;
    if(!sweetgl_make_current(program->context)){result=-EIO;goto done;}
    result=prepare_pass(sweetgl_internal_context(program->context),pass,&target,&framebuffer);if(result)goto done;
    glUseProgram(program->program);for(unsigned i=0;i<count && !result;++i)result=buffers[i].index<8?
        bind_vertex_words(sweetgl_internal_context(program->context),&buffers[i],program->program,&buffer_names[i]):
        sweet_metal_bind_buffer(sweetgl_internal_context(program->context),&buffers[i],&buffer_names[i]);
    if(!result)result=bind_samples(program,textures,texture_count,samplers,sampler_count,samples);if(result)goto done;
    glViewport(draw->x,draw->y,draw->width,draw->height);glDepthRangef(draw->znear,draw->zfar);
    glEnable(GL_SCISSOR_TEST);glScissor(draw->scissor_x,draw->scissor_y,draw->scissor_width,draw->scissor_height);
    if(draw->cull){if(draw->cull>2){result=-EINVAL;goto done;}glEnable(GL_CULL_FACE);glCullFace(draw->cull==1?GL_FRONT:GL_BACK);}else glDisable(GL_CULL_FACE);
    // y inversion reverses clip-space winding while row0 remains Metal row0.
    glFrontFace(draw->winding==1?GL_CW:GL_CCW);
    if(program->render.blend_enabled){glEnable(GL_BLEND);glBlendFuncSeparate(blend_factor(program->render.source_rgb),blend_factor(program->render.destination_rgb),
        blend_factor(program->render.source_alpha),blend_factor(program->render.destination_alpha));glBlendEquationSeparate(blend_operation(program->render.operation_rgb),blend_operation(program->render.operation_alpha));}
    else glDisable(GL_BLEND);uint32_t writes=program->render.write_mask;glColorMask(writes&8?GL_TRUE:GL_FALSE,writes&4?GL_TRUE:GL_FALSE,writes&2?GL_TRUE:GL_FALSE,writes&1?GL_TRUE:GL_FALSE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);static const GLenum primitive[]={GL_POINTS,GL_LINES,GL_LINE_STRIP,GL_TRIANGLES,GL_TRIANGLE_STRIP};
    glDrawArraysInstanced(primitive[draw->primitive],(GLint)draw->first,(GLsizei)draw->count,(GLsizei)draw->instances);glMemoryBarrier(GL_ALL_BARRIER_BITS);glFinish();
    if(glGetError()!=GL_NO_ERROR){result=-EIO;goto done;}
    result=sweet_drm_metal_buffer_sequence(pass->target.handle,sequence);if(!result)result=sweetgl_metal_buffer_synchronize(pass->target.handle);
    for(unsigned i=0;i<count && !result;++i)if(buffers[i].index>=8 && (program->render.fragment_writes&(1u<<(buffers[i].index-8))))
        result=sweetgl_metal_buffer_synchronize(buffers[i].handle);
done:
    if(vao)glDeleteVertexArrays(1,&vao);if(framebuffer)glDeleteFramebuffers(1,&framebuffer);if(target)glDeleteTextures(1,&target);
    for(unsigned i=0;i<32;++i)if(samples[i])glDeleteTextures(1,&samples[i]);for(unsigned i=0;i<count;++i)if(buffer_names[i]){
        if(buffers[i].index<8)glDeleteTextures(1,&buffer_names[i]);else glDeleteBuffers(1,&buffer_names[i]);}
    sweet_drm_log("SWEET AIR RENDER draw width=%u height=%u primitive=%u first=%u count=%u instances=%u target_handle=%u result=%d sequence=%u original_pages=1",
        pass->target.width,pass->target.height,draw->primitive,draw->first,draw->count,draw->instances,pass->target.handle,result,*sequence);
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
}
int sweetgl_metal_render_clear(const SweetRenderPass *pass,uint32_t *sequence)
{
    if(sequence)*sequence=0;if(!pass || !sequence)return -EINVAL;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();struct sweetgl_ctx *context=sweetgl_create_metal();
    GLuint texture=0,framebuffer=0;int result=-EIO;
    if(context && sweetgl_make_current(context)){result=prepare_pass(sweetgl_internal_context(context),pass,&texture,&framebuffer);
        if(!result && pass->load_action==2){glFinish();result=sweet_drm_metal_buffer_sequence(pass->target.handle,sequence);
            if(!result)result=sweetgl_metal_buffer_synchronize(pass->target.handle);}else if(!result)result=-ENOTSUP;
        if(framebuffer)glDeleteFramebuffers(1,&framebuffer);if(texture)glDeleteTextures(1,&texture);}
    if(context){sweetgl_make_current(NULL);sweetgl_destroy(context);}
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
}
