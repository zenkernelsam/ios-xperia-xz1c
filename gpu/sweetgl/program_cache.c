/* SPDX-License-Identifier: BSD-2-Clause */
#include "MetalShaderInternals.h"
#include "ProgramCacheStore.h"
#include "ProgramCacheBuildIdentity.h"
#include "sweet_drm.h"
#include "sweetgl.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_manager.h"
#include "pipe/p_screen.h"

static void cache_identity(struct gl_context *ctx,unsigned char out[32])
{
    SweetCacheHash hash;sweet_cache_hash_init(&hash);sweet_cache_hash_add(&hash,sweet_program_cache_build_id,32);
    unsigned char properties[8];sweet_cache_put32(properties,(uint32_t)ctx->API);sweet_cache_put32(properties+4,(uint32_t)ctx->Version);
    sweet_cache_hash_add(&hash,properties,sizeof(properties));const char *name=st_context(ctx)->screen->get_name(st_context(ctx)->screen);
    if(name)sweet_cache_hash_add(&hash,name,strlen(name));sweet_cache_hash_end(&hash,out);
}
// Mesa's disabled disk-cache stub leaves the original driver SHA untouched.
// Its weak platform hook uses these exact build/context inputs instead.
__attribute__((visibility("default"))) void sweetgl_program_binary_identity(struct gl_context *ctx,unsigned char out[20])
{unsigned char digest[32];cache_identity(ctx,digest);memcpy(out,digest,20);}
// Gallium can serialize NIR without a disk cache when the platform supplies
// the immutable driver identity above. Advertise this only for our FD540.
__attribute__((visibility("default"))) bool sweetgl_program_binary_supported(struct pipe_screen *screen)
{const char *name=screen && screen->get_name?screen->get_name(screen):NULL;return name && !strcmp(name,"FD540");}
static const char *cache_root(void)
{const char *root=getenv("SWEETGL_PROGRAM_CACHE_DIR");return root && *root?root:"/private/var/tmp";}
void sweet_metal_program_key(SweetMetalProgram *program,const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config)
{
    cache_identity(sweetgl_internal_context(program->context)->ctx,program->cache_identity);
    sweet_cache_key(program->cache_identity,config,vertex,strlen(vertex),fragment,strlen(fragment),program->cache_key);
}
int sweet_metal_program_restore(SweetMetalProgram *program)
{
    char path[1024];if(sweet_cache_path(cache_root(),program->cache_identity,program->cache_key,SWEET_PROGRAM_CACHE_BINARY,path))return 0;
    unsigned char *bytes=NULL;size_t count=0;int result=sweet_cache_read_file(path,&bytes,&count);
    if(result)return 0;
    if(!sweet_cache_validate(bytes,count,SWEET_PROGRAM_CACHE_BINARY,program->cache_identity,program->cache_key)){
        sweet_drm_log("SWEET PROGRAM CACHE reject kind=binary reason=record_identity_digest");free(bytes);return 0;}
    GLenum format=sweet_cache_get32(bytes+16);GLint formats=0;glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&formats);
    GLint allowed[16]={0};if(formats<1 || formats>16){free(bytes);return 0;}glGetIntegerv(GL_PROGRAM_BINARY_FORMATS,allowed);
    bool found=false;for(GLint i=0;i<formats;++i)if((GLenum)allowed[i]==format)found=true;
    if(!found){free(bytes);return 0;}
    GLuint name=glCreateProgram();glProgramBinary(name,format,bytes+SWEET_PROGRAM_CACHE_HEADER,(GLsizei)sweet_cache_get32(bytes+20));
    GLint linked=0;glGetProgramiv(name,GL_LINK_STATUS,&linked);GLenum error=glGetError();
    if(!linked || error!=GL_NO_ERROR){sweet_drm_log("SWEET PROGRAM CACHE reject kind=binary reason=Mesa_import linked=%d error=%#x",linked,error);glDeleteProgram(name);free(bytes);return 0;}
    program->program=name;program->program_cache_state=1;
    sweet_drm_log("SWEET PROGRAM CACHE hit bytes=%u format=%#x exact_build=1 exact_source_config=1",sweet_cache_get32(bytes+20),format);
    free(bytes);return 1;
}
static int save_record(SweetMetalProgram *program,unsigned kind,unsigned format,const void *payload,size_t count,size_t vs,size_t fs)
{
    if(!count || count>SWEET_PROGRAM_CACHE_MAX)return -EINVAL;
    unsigned char *record=malloc(SWEET_PROGRAM_CACHE_HEADER+count);if(!record)return -ENOMEM;
    if(!sweet_cache_header(record,kind,format,program->cache_identity,program->cache_key,payload,count,vs,fs)){free(record);return -EINVAL;}
    memcpy(record+SWEET_PROGRAM_CACHE_HEADER,payload,count);
    int result=sweet_cache_write_file(cache_root(),program->cache_identity,program->cache_key,kind,record,SWEET_PROGRAM_CACHE_HEADER+count);
    free(record);return result;
}
void sweet_metal_program_request(SweetMetalProgram *program,const char *vertex,const char *fragment,const SweetRenderPipelineConfig *config)
{
    size_t vs=strlen(vertex),fs=strlen(fragment),bytes=sizeof(*config)+vs+fs;unsigned char *payload=malloc(bytes);if(!payload)return;
    memcpy(payload,config,sizeof(*config));memcpy(payload+sizeof(*config),vertex,vs);memcpy(payload+sizeof(*config)+vs,fragment,fs);
    int result=save_record(program,SWEET_PROGRAM_CACHE_REQUEST,0,payload,bytes,vs,fs);free(payload);
    if(result!=-EEXIST)sweet_drm_log("SWEET PROGRAM CACHE request result=%d vertex_bytes=%lu fragment_bytes=%lu immutable=1",result,(unsigned long)vs,(unsigned long)fs);
}
void sweet_metal_program_store(SweetMetalProgram *program,const char *vertex,const char *fragment)
{
    GLint formats=0,bytes=0;glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&formats);
    if(formats<1){sweet_drm_log("SWEET PROGRAM CACHE unavailable reason=formats formats=%d",formats);return;}
    glGetProgramiv(program->program,GL_PROGRAM_BINARY_LENGTH,&bytes);
    if(bytes<1 || bytes>(GLint)SWEET_PROGRAM_CACHE_MAX){sweet_drm_log("SWEET PROGRAM CACHE unavailable reason=length bytes=%d formats=%d",bytes,formats);return;}
    unsigned char *payload=malloc((size_t)bytes);if(!payload)return;GLsizei actual=0;GLenum format=0;
    glGetProgramBinary(program->program,bytes,&actual,&format,payload);GLenum error=glGetError();int result=-EIO;
    if(error==GL_NO_ERROR && actual>0 && actual<=bytes)result=save_record(program,SWEET_PROGRAM_CACHE_BINARY,format,payload,(size_t)actual,strlen(vertex),strlen(fragment));
    if(!result)program->program_cache_state=2;
    sweet_drm_log("SWEET PROGRAM CACHE stored result=%d bytes=%d format=%#x error=%#x exact_build=1",result,actual,format,error);free(payload);
}
int sweetgl_metal_program_cache_state(const SweetMetalProgram *program)
{return program?program->program_cache_state:-EINVAL;}
int sweetgl_metal_program_cache_prime(uint32_t *completed,uint32_t *failed)
{
    if(!completed || !failed)return -EINVAL;*completed=*failed=0;
    unsigned char identity[32];pthread_mutex_lock(&sweet_metal_shader_gate);struct st_context *previous=st_api_get_current();
    struct sweetgl_ctx *context=sweetgl_create_metal();int ready=context && sweetgl_make_current(context);
    if(ready)cache_identity(sweetgl_internal_context(context)->ctx,identity);
    if(context){sweetgl_make_current(NULL);sweetgl_destroy(context);}sweetgl_internal_restore(previous);pthread_mutex_unlock(&sweet_metal_shader_gate);
    if(!ready)return -EIO;
    const char *root=cache_root();DIR *dir=opendir(root);if(!dir)return -errno;char id[33],prefix[40];sweet_cache_hex(identity,16,id);snprintf(prefix,sizeof(prefix),"sgpc-%s-",id);
    struct dirent *entry;unsigned seen=0;
    while((entry=readdir(dir))){size_t n=strlen(entry->d_name);if(strncmp(entry->d_name,prefix,strlen(prefix)) || n<4 || strcmp(entry->d_name+n-4,".req"))continue;
        char path[1024],binary[1024];int length=snprintf(path,sizeof(path),"%s/%s",root,entry->d_name);if(length<=0 || length>=(int)sizeof(path)){++*failed;continue;}
        memcpy(binary,path,(size_t)length+1);memcpy(binary+length-3,"bin",3);struct stat existing;
        if(!lstat(binary,&existing))continue; // Immutable successful checkpoints need no recompile.
        if(++seen>8)break;
        unsigned char *record=NULL;size_t bytes=0;int result=sweet_cache_read_file(path,&record,&bytes);
        if(result || !sweet_cache_validate(record,bytes,SWEET_PROGRAM_CACHE_REQUEST,identity,NULL)){free(record);++*failed;continue;}
        uint32_t vs=sweet_cache_get32(record+24),fs=sweet_cache_get32(record+28);const unsigned char *data=record+SWEET_PROGRAM_CACHE_HEADER;
        SweetRenderPipelineConfig config;memcpy(&config,data,sizeof(config));char *vertex=malloc((size_t)vs+1),*fragment=malloc((size_t)fs+1);
        if(!vertex || !fragment){free(vertex);free(fragment);free(record);++*failed;continue;}
        memcpy(vertex,data+sizeof(config),vs);vertex[vs]=0;memcpy(fragment,data+sizeof(config)+vs,fs);fragment[fs]=0;
        unsigned char key[32];sweet_cache_key(identity,&config,vertex,vs,fragment,fs,key);
        if(memcmp(key,record+72,32)){free(vertex);free(fragment);free(record);++*failed;continue;}
        sweet_drm_log("SWEET PROGRAM CACHE prime_begin vertex_bytes=%u fragment_bytes=%u source_verified=1",vs,fs);
        char error[1024]={0};SweetMetalProgram *program=sweetgl_metal_render_create(vertex,fragment,&config,error,sizeof(error));
        int state=sweetgl_metal_program_cache_state(program);if(program && (state==1 || state==2))++*completed;else ++*failed;
        sweet_drm_log("SWEET PROGRAM CACHE prime_entry result=%u cache_state=%d reason=%s",program && (state==1 || state==2)?0:1,state,error);
        sweetgl_metal_program_destroy(program);free(vertex);free(fragment);free(record);
    }
    closedir(dir);sweet_drm_log("SWEET PROGRAM CACHE PRIME END result=%u compiled=%u failed=%u max_entries=8 original_requests=1",*failed?1:0,*completed,*failed);
    return *failed?-EIO:0;
}
