/* AIR frontend's ESSL executes on the existing Mesa FD540 screen/context. */
#include "MetalShaderAPI.h"
#include "MetalBackingRules.h"
#include "sweetgl.h"
#include "sweet_drm.h"
#include "MetalShaderInternals.h"
#include <GLES3/gl31.h>
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "frontend/winsys_handle.h"
#include "util/u_inlines.h"
#include "main/bufferobj.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_manager.h"
#include "freedreno/freedreno_resource.h"
#include "drm-uapi/drm_fourcc.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
pthread_mutex_t sweet_metal_shader_gate=PTHREAD_MUTEX_INITIALIZER;
static void diagnostic(char *buffer,size_t bytes,const char *message)
{if(buffer && bytes)snprintf(buffer,bytes,"%s",message);}
SweetMetalProgram *sweetgl_metal_compute_create(const char *source,uint32_t buffer_mask,uint32_t write_mask,char *error,size_t error_bytes)
{
    if(error && error_bytes)error[0]=0;
    if(!source || strnlen(source,1<<20)==(1<<20) || buffer_mask>255 || (write_mask&~buffer_mask)){
        diagnostic(error,error_bytes,"Invalid ESSL input/masks");return NULL;}
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();
    SweetMetalProgram *result=calloc(1,sizeof(*result));GLuint shader=0;GLint compiled=0,linked=0;
    if(!result || !(result->context=sweetgl_create_metal()) || !sweetgl_make_current(result->context)){
        diagnostic(error,error_bytes,"FD540 robust context unavailable");goto failure;}
    for(unsigned i=0;i<16 && glGetError()!=GL_NO_ERROR;++i){}
    shader=glCreateShader(GL_COMPUTE_SHADER);glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    if(!compiled){if(error && error_bytes)glGetShaderInfoLog(shader,(GLsizei)error_bytes,NULL,error);goto failure;}
    result->program=glCreateProgram();glAttachShader(result->program,shader);glLinkProgram(result->program);
    glGetProgramiv(result->program,GL_LINK_STATUS,&linked);
    if(!linked){if(error && error_bytes)glGetProgramInfoLog(result->program,(GLsizei)error_bytes,NULL,error);goto failure;}
    glDeleteShader(shader);result->buffer_mask=buffer_mask;result->write_mask=write_mask;
    uint64_t hash=UINT64_C(14695981039346656037);size_t bytes=strlen(source);
    for(size_t i=0;i<bytes;++i)hash=(hash^(unsigned char)source[i])*UINT64_C(1099511628211);
    sweet_drm_log("SWEET AIR MESA compiled stage=compute program=%u buffer_mask=%u write_mask=%u source_bytes=%lu source_hash=%016llx linked=1 robust=1",
        result->program,buffer_mask,write_mask,(unsigned long)bytes,(unsigned long long)hash);
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
failure:
    if(result && result->context){if(result->program)glDeleteProgram(result->program);if(shader)glDeleteShader(shader);
        sweetgl_make_current(NULL);sweetgl_destroy(result->context);}
    free(result);sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return NULL;
}
int sweet_metal_bind_buffer(struct st_context *st,const SweetShaderBufferBinding *binding,GLuint *name)
{
    SweetMetalBacking backing={0};if(sweet_drm_metal_buffer_view(binding->handle,&backing))return -EINVAL;
    if(binding->index>=16 || !binding->bytes || binding->offset>backing.logical_bytes || binding->bytes>backing.logical_bytes-binding->offset ||
        (binding->offset&3) || (binding->bytes&3))return -EINVAL;
    struct pipe_resource template={.target=PIPE_BUFFER,.format=PIPE_FORMAT_R8_UNORM,.width0=(uint32_t)backing.allocated_bytes,
        .height0=1,.depth0=1,.array_size=1,.bind=PIPE_BIND_SHADER_BUFFER|PIPE_BIND_SHARED,.usage=PIPE_USAGE_DEFAULT};
    struct winsys_handle handle={.type=WINSYS_HANDLE_TYPE_KMS,.handle=binding->handle,.stride=(unsigned)backing.allocated_bytes,
        .offset=0,.format=PIPE_FORMAT_R8_UNORM,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=st->screen->resource_from_handle(st->screen,&template,&handle,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(!resource)return -ENOMEM;
    if(sweet_drm_metal_buffer_view_keep(binding->handle)){pipe_resource_reference(&resource,NULL);return -EINVAL;}
    glGenBuffers(1,name);glBindBuffer(GL_SHADER_STORAGE_BUFFER,*name);
    struct gl_buffer_object *object=_mesa_lookup_bufferobj(st->ctx,*name);
    if(!object){pipe_resource_reference(&resource,NULL);return -ENOMEM;}
    _mesa_bufferobj_release_buffer(object);object->buffer=resource;object->Size=(GLsizeiptr)backing.allocated_bytes;
    object->Immutable=true;object->Usage=GL_DYNAMIC_DRAW;object->StorageFlags=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_DYNAMIC_STORAGE_BIT;
    struct fd_resource *fd=fd_resource(resource);util_range_add(resource,&fd->valid_buffer_range,0,(unsigned)backing.allocated_bytes);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER,binding->index,*name,(GLintptr)binding->offset,(GLsizeiptr)binding->bytes);
    sweet_drm_log("SWEET AIR MESA binding index=%u handle=%u gpu=%#llx cpu=%p offset=%llu bytes=%llu logical=%llu original_pages=1",
        binding->index,binding->handle,(unsigned long long)backing.gpu_address,backing.contents,
        (unsigned long long)binding->offset,(unsigned long long)binding->bytes,(unsigned long long)backing.logical_bytes);
    return glGetError()==GL_NO_ERROR?0:-EIO;
}
int sweetgl_metal_compute_dispatch(SweetMetalProgram *program,const SweetShaderBufferBinding *bindings,uint32_t count,
    const uint32_t groups[3],const uint32_t local[3],const uint32_t grid[3],uint32_t *sequence)
{
    if(sequence)*sequence=0;
    if(!program || !bindings || count>8 || !groups || !local || !grid || !sequence)return -EINVAL;
    uint32_t mask=0;for(unsigned i=0;i<count;++i){if(bindings[i].index>=8 || (mask&(1u<<bindings[i].index)))return -EINVAL;mask|=1u<<bindings[i].index;}
    if(mask!=program->buffer_mask)return -EINVAL;
    uint64_t invocations=1,total=1;for(unsigned i=0;i<3;++i){if(!groups[i] || groups[i]>65535 || !local[i] || local[i]>1024 ||
        !grid[i] || grid[i]>(uint64_t)groups[i]*local[i] || (uint64_t)groups[i]*local[i]>16777216/total)return -EINVAL;
        invocations*=local[i];total*=(uint64_t)groups[i]*local[i];}
    if(invocations>1024)return -EINVAL;
    pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();GLuint names[8]={0};int result=0;
    if(!sweetgl_make_current(program->context)){result=-EIO;goto done;}
    struct st_context *st=sweetgl_internal_context(program->context);GLint actual_local[3]={0};
    glGetProgramiv(program->program,GL_COMPUTE_WORK_GROUP_SIZE,actual_local);
    for(unsigned i=0;i<3;++i)if(actual_local[i]!=(GLint)local[i]){result=-EINVAL;goto done;}
    for(unsigned i=0;i<count && !result;++i)result=sweet_metal_bind_buffer(st,&bindings[i],&names[i]);
    if(result)goto done;
    glUseProgram(program->program);GLuint info[16]={0};info[3]=local[0];info[7]=local[1];info[11]=local[2];
    for(unsigned i=0;i<3;++i){info[8+i]=grid[i];info[12+i]=groups[i];}
    GLint location=glGetUniformLocation(program->program,"_qdispatch_info");if(location>=0)glUniform4uiv(location,4,info);
    glDispatchCompute(groups[0],groups[1],groups[2]);glMemoryBarrier(GL_ALL_BARRIER_BITS);glFinish();
    if(glGetError()!=GL_NO_ERROR){result=-EIO;goto done;}
    for(unsigned i=0;i<count && !result;++i){uint32_t completed=0;
        result=sweet_drm_metal_buffer_sequence(bindings[i].handle,&completed);if(completed>*sequence)*sequence=completed;
        if(!result && (program->write_mask&(1u<<bindings[i].index)))result=sweetgl_metal_buffer_synchronize(bindings[i].handle);}
done:
    for(unsigned i=0;i<count;++i)if(names[i])glDeleteBuffers(1,&names[i]);
    sweet_drm_log("SWEET AIR MESA dispatch groups=%u,%u,%u local=%u,%u,%u result=%d sequence=%u original_buffers=%u",
        groups[0],groups[1],groups[2],local[0],local[1],local[2],result,*sequence,count);
    sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);return result;
}
void sweetgl_metal_program_destroy(SweetMetalProgram *program)
{
    if(!program)return;pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();
    sweetgl_make_current(program->context);glDeleteProgram(program->program);sweet_metal_shader_cache_dispose(&program->cache);sweetgl_make_current(NULL);sweetgl_destroy(program->context);
    sweetgl_internal_restore(previous);free(program);pthread_mutex_unlock(&sweet_metal_shader_gate);
}
