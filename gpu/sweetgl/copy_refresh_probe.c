#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <GLES3/gl3.h>
#include <mach/mach.h>
#include <mach/vm_page_size.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sweetgl.h"
#include "sweet_drm.h"
#include "CopyRefreshProbeUtils.h"
void *sweetgl_copy_probe_create(IOSurfaceRef,unsigned,unsigned);
int sweetgl_copy_probe_update(void *,unsigned *,uint32_t *);
void sweetgl_copy_probe_destroy(void *);
void sweetgl_copy_probe_counts(void *,unsigned *,unsigned *,uint64_t *);
static void number(CFMutableDictionaryRef d,CFStringRef key,uint64_t value)
{
    int64_t n=(int64_t)value;CFNumberRef v=CFNumberCreate(NULL,kCFNumberSInt64Type,&n);
    if(v){CFDictionarySetValue(d,key,v);CFRelease(v);}
}
static GLuint shader(GLenum kind,const char *source)
{GLuint s=glCreateShader(kind);glShaderSource(s,1,&source,NULL);glCompileShader(s);return s;}
static bool case_test(unsigned w,unsigned h,unsigned pitch,unsigned bpe,GLuint program)
{
    size_t span=(size_t)(h-1)*pitch+(size_t)w*bpe;
    vm_size_t cover=(vm_size_t)((span+vm_page_mask)&~((uint64_t)vm_page_mask));vm_address_t source_vm=0;
    IOSurfaceRef surface=NULL;void *probe=NULL;GLuint tex[2]={0},fbo=0;uint8_t *readback=NULL;
    bool locked=false,ok=false;const char *phase_name="allocate";unsigned uploads=0;uint32_t seed=0,previous_seed=0;
    if(vm_allocate(mach_task_self(),&source_vm,cover,VM_FLAGS_ANYWHERE))goto done;
    memset((void *)(uintptr_t)source_vm,0,(size_t)cover);
    CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(!d)goto done;
    number(d,CFSTR("IOSurfaceWidth"),w);number(d,CFSTR("IOSurfaceHeight"),h);
    number(d,CFSTR("IOSurfaceBytesPerRow"),pitch);number(d,CFSTR("IOSurfaceBytesPerElement"),bpe);
    number(d,CFSTR("IOSurfacePixelFormat"),bpe==2?UINT32_C(0x4c413038):UINT32_C(0x52474241));
    number(d,CFSTR("IOSurfaceAllocSize"),span);number(d,CFSTR("IOSurfaceAddress"),source_vm);
    surface=IOSurfaceCreate(d);CFRelease(d);phase_name="surface";
    if(!surface||!IOSurfaceGetBaseAddress(surface)||IOSurfaceGetBaseAddress(surface)==(void *)(uintptr_t)source_vm)goto done;
    probe=sweetgl_copy_probe_create(surface,bpe==2?GL_LUMINANCE_ALPHA:GL_RGBA,GL_UNSIGNED_BYTE);if(!probe)goto done;
    glGenTextures(2,tex);glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex[1]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[1],0);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);if(status!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glBindTexture(GL_TEXTURE_2D,tex[0]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glViewport(0,0,w,h);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"sourceImage"),0);
    glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT,1);readback=malloc((size_t)w*h*4);if(!readback)goto done;
    for(unsigned phase=0;phase<4;++phase){
        if(phase==3){
            int result=vm_deallocate(mach_task_self(),source_vm,cover);
            sweet_drm_log("sweetgl: copy refresh source release id=%u width=%u height=%u bytes=%llu result=%x held_surface=1 held_copy=1",
                IOSurfaceGetID(surface),w,h,(unsigned long long)cover,result);
            if(result)goto done;source_vm=0;
        }
        phase_name="writer";if(IOSurfaceLock(surface,0,NULL))goto done;locked=true;
        uint8_t *source=phase==3?IOSurfaceGetBaseAddress(surface):(void *)(uintptr_t)source_vm;
        crp_fill(source,w,h,pitch,phase,bpe);
        if(IOSurfaceUnlock(surface,0,NULL))goto done;locked=false;
        uint32_t current_seed=IOSurfaceGetSeed(surface);if(phase&&current_seed==previous_seed)goto done;
        phase_name="upload";int updated=sweetgl_copy_probe_update(probe,&uploads,&seed);
        if(updated!=0||uploads!=phase+1||seed!=current_seed)goto done;
        unsigned images=0,subimages=0;uint64_t owner=0;
        sweetgl_copy_probe_counts(probe,&images,&subimages,&owner);
        if(images!=1||subimages!=phase||owner!=sweetgl_current_identity())goto done;
        sweet_drm_log("sweetgl: copy storage id=%u phase=%u context=%llu images=%u subimages=%u uploads=%u",
            IOSurfaceGetID(surface),phase,(unsigned long long)owner,images,subimages,uploads);
        memset(readback,0xa5,(size_t)w*h*4);glDrawArrays(GL_TRIANGLES,0,3);
        glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,readback);GLenum error=glGetError();uint64_t hash=0;
        unsigned mismatch=crp_check(readback,w,h,phase,bpe,&hash);
        sweet_drm_log("sweetgl: copy refresh draw id=%u width=%u height=%u bpe=%u pitch=%u raw=%zu phase=%u seed=%u uploads=%u checked=%u mismatches=%u fbo=%x err=%x hash=%016llx source_vm_released=%u",
            IOSurfaceGetID(surface),w,h,bpe,pitch,span,phase,seed,uploads,w*h,mismatch,status,error,(unsigned long long)hash,phase==3);
        phase_name="pixels";if(error||mismatch)goto done;
        unsigned count=uploads;int skipped=sweetgl_copy_probe_update(probe,&uploads,&seed);
        if(skipped!=1||uploads!=count)goto done;
        memset(readback,0xa5,(size_t)w*h*4);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,readback);
        error=glGetError();mismatch=crp_check(readback,w,h,phase,bpe,&hash);
        sweet_drm_log("sweetgl: copy refresh unchanged id=%u phase=%u seed=%u uploads=%u result=%d checked=%u mismatches=%u err=%x hash=%016llx",
            IOSurfaceGetID(surface),phase,seed,uploads,skipped,w*h,mismatch,error,(unsigned long long)hash);
        if(error||mismatch)goto done;previous_seed=current_seed;
    }
    sweet_drm_log("sweetgl: copy storage case id=%u images=1 subimages=3",IOSurfaceGetID(surface));
    ok=true;phase_name="complete";
done:
    if(locked)IOSurfaceUnlock(surface,0,NULL);
    sweetgl_copy_probe_destroy(probe);glDeleteFramebuffers(1,&fbo);glDeleteTextures(2,tex);free(readback);
    if(surface)CFRelease(surface);if(source_vm)vm_deallocate(mach_task_self(),source_vm,cover);
    sweet_drm_log("sweetgl: copy refresh case -> %s width=%u height=%u bpe=%u pitch=%u phase=%s uploads=%u",ok?"PASS":"WRONG",w,h,bpe,pitch,phase_name,uploads);
    return ok;
}
bool sweetgl_copy_refresh_selftest(void)
{
    struct sweetgl_ctx *context=sweetgl_create(2,NULL);GLuint program=0,vbo=0;bool ok=false;
    if(!context||!sweetgl_make_current(context))goto done;
    program=glCreateProgram();GLuint vs=shader(GL_VERTEX_SHADER,
        "attribute vec2 position;varying highp vec2 uv;void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}");
    GLuint fs=shader(GL_FRAGMENT_SHADER,
        "precision highp float;varying highp vec2 uv;uniform sampler2D sourceImage;void main(){gl_FragColor=texture2D(sourceImage,uv);}");
    glAttachShader(program,vs);glAttachShader(program,fs);glBindAttribLocation(program,0,"position");glLinkProgram(program);
    glDeleteShader(vs);glDeleteShader(fs);GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);if(!linked)goto done;
    const GLfloat vertices[]={-1,-1,3,-1,-1,3};glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);glEnableVertexAttribArray(0);
    ok=case_test(256,1,672,2,program)&&case_test(60,60,240,4,program)&&case_test(22,27,88,4,program);
done:
    glDeleteBuffers(1,&vbo);glDeleteProgram(program);sweetgl_make_current(NULL);if(context)sweetgl_destroy(context);
    sweet_drm_log("sweetgl: copy refresh selftest -> %s",ok?"PASS":"WRONG");return ok;
}
