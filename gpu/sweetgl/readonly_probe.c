/* Real readonly caller-page sampling and lifetime, without attempting a write. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <GLES3/gl3.h>
#include <mach/mach.h>
#include <mach/vm_region.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sweetgl.h"
#include "sweet_drm.h"
#include "SurfaceProbeUtils.h"
void *sweetgl_sampler_probe_create(IOSurfaceRef);
int sweetgl_sampler_probe_bind(void *);
void sweetgl_sampler_probe_destroy(void *);
static void number(CFMutableDictionaryRef d,CFStringRef key,uint64_t v){
    int64_t value=v;CFNumberRef n=CFNumberCreate(NULL,kCFNumberSInt64Type,&value);
    if(n){CFDictionarySetValue(d,key,n);CFRelease(n);}
}
static GLuint shader(GLenum type,const char *source){
    GLuint n=glCreateShader(type);glShaderSource(n,1,&source,NULL);glCompileShader(n);return n;
}
static bool readonly_case(unsigned offset){
    enum {W=400,H=400,PITCH=1600,COVER=655360};
    bool ok=false,locked=false;const char *phase="context";
    struct sweetgl_ctx *ctx=sweetgl_create(3,NULL);vm_address_t source=0;IOSurfaceRef surface=NULL;
    uint8_t *pixels=NULL,*view=NULL;GLuint tex[2]={0},fbo=0,program=0;void *sampler=NULL;
    if(!ctx||!sweetgl_make_current(ctx))goto done;
    phase="allocate";if(vm_allocate(mach_task_self(),&source,COVER,VM_FLAGS_ANYWHERE))goto done;
    memset((void *)(uintptr_t)source,0,COVER);
    uint8_t *seed=(uint8_t *)(uintptr_t)source+offset;
    for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){uint8_t p[4];sp_pattern(p,x,y);
        uint8_t *d=seed+y*PITCH+x*4;d[0]=p[2];d[1]=p[1];d[2]=p[0];d[3]=255;}
    phase="protect";if(vm_protect(mach_task_self(),source,COVER,true,VM_PROT_READ))goto done;
    CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(!d)goto done;
    number(d,CFSTR("IOSurfaceWidth"),W);number(d,CFSTR("IOSurfaceHeight"),H);
    number(d,CFSTR("IOSurfaceBytesPerElement"),4);number(d,CFSTR("IOSurfaceBytesPerRow"),PITCH);
    number(d,CFSTR("IOSurfacePixelFormat"),0x42475241);number(d,CFSTR("IOSurfaceAllocSize"),PITCH*H);
    number(d,CFSTR("IOSurfaceAddress"),source+offset);
    phase="create";surface=IOSurfaceCreate(d);CFRelease(d);if(!surface)goto done;
    view=IOSurfaceGetBaseAddress(surface);if(!view||view==seed)goto done;
    vm_address_t address=(uintptr_t)view;vm_size_t region_size=0;natural_t depth=0;
    vm_region_submap_info_data_64_t info={0};mach_msg_type_number_t count=VM_REGION_SUBMAP_INFO_COUNT_64;
    int region=vm_region_recurse_64(mach_task_self(),&address,&region_size,&depth,(vm_region_recurse_info_t)&info,&count);
    int write_lock=IOSurfaceLock(surface,0,NULL);
    if(!write_lock){IOSurfaceUnlock(surface,0,NULL);goto done;}
    int read_lock=IOSurfaceLock(surface,kIOSurfaceLockReadOnly,NULL);locked=!read_lock;
    uint64_t hash=0;unsigned mismatch=locked?sp_check_pattern(view,W,H,PITCH,true,&hash):UINT32_MAX;
    sweet_drm_log("sweetgl: readonly view id=%u width=400 height=400 source=%#llx returned=%p region=%x prot=%x max=%x write_lock=%x read_lock=%x checked=160000 mismatches=%u hash=%016llx",
        IOSurfaceGetID(surface),(uint64_t)seed,view,region,info.protection,info.max_protection,write_lock,read_lock,mismatch,hash);
    phase="view";if(region||info.protection!=VM_PROT_READ||info.max_protection!=VM_PROT_READ||mismatch)goto done;
    if(IOSurfaceUnlock(surface,kIOSurfaceLockReadOnly,NULL))goto done;locked=false;
    glGenTextures(2,tex);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex[0]);
    phase="bind";sampler=sweetgl_sampler_probe_create(surface);
    if(!sampler||sweetgl_sampler_probe_bind(sampler))goto done;
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,tex[1]);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,W,H);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[1],0);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
    program=glCreateProgram();GLuint vs=shader(GL_VERTEX_SHADER,"#version 300 es\nout highp vec2 uv;\nvoid main(){vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);uv=(p+1.0)*0.5;gl_Position=vec4(p,0,1);}\n");
    GLuint fs=shader(GL_FRAGMENT_SHADER,"#version 300 es\nprecision highp float;in highp vec2 uv;uniform sampler2D image;out vec4 colour;void main(){colour=texture(image,uv);}\n");
    glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
    glBindTexture(GL_TEXTURE_2D,tex[0]);glViewport(0,0,W,H);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);pixels=malloc(PITCH*H);if(!pixels)goto done;
    for(unsigned pass=0;pass<2;++pass){
        if(pass){int kr=vm_deallocate(mach_task_self(),source,COVER);if(kr)goto done;source=0;}
        glBindTexture(GL_TEXTURE_2D,tex[0]);
        if(sweetgl_sampler_probe_bind(sampler))goto done;
        memset(pixels,0xa5,PITCH*H);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        GLenum error=glGetError();mismatch=sp_check_pattern(pixels,W,H,PITCH,false,&hash);
        sweet_drm_log("sweetgl: readonly sampler id=%u offset=%u phase=%u checked=160000 mismatches=%u fbo=%x linked=%d err=%x hash=%016llx source_vm_released=%u",
            IOSurfaceGetID(surface),offset,pass,mismatch,status,linked,error,hash,pass);
        phase=pass?"held_sampler":"sampler";if(mismatch||error||!linked||status!=GL_FRAMEBUFFER_COMPLETE)goto done;
    }
    ok=true;
done:
    if(locked)IOSurfaceUnlock(surface,kIOSurfaceLockReadOnly,NULL);
    sweetgl_sampler_probe_destroy(sampler);
    if(ctx){glDeleteProgram(program);glDeleteFramebuffers(1,&fbo);glDeleteTextures(2,tex);sweetgl_make_current(NULL);sweetgl_destroy(ctx);}
    if(surface)CFRelease(surface);if(source)vm_deallocate(mach_task_self(),source,COVER);free(pixels);
    sweet_drm_log("sweetgl: readonly case offset=%u -> %s phase=%s",offset,ok?"PASS":"WRONG",phase);return ok;
}
bool sweetgl_readonly_selftest(void){
    /* Finish the bounded alignment comparison even if one case is wrong. */
    bool a=readonly_case(32),b=readonly_case(64),c=readonly_case(128);
    sweet_drm_log("sweetgl: readonly selftest -> %s phase=held_sampler",a&&b&&c?"PASS":"WRONG");
    return a&&b&&c;
}
