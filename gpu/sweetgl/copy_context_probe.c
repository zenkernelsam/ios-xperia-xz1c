#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <GLES3/gl3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sweetgl.h"
#include "sweet_drm.h"
#include "CopyRefreshProbeUtils.h"
void *sweetgl_copy_probe_create(IOSurfaceRef,unsigned,unsigned);
int sweetgl_copy_probe_update(void *,unsigned *,uint32_t *);
void sweetgl_copy_probe_counts(void *,unsigned *,unsigned *,uint64_t *);
void sweetgl_copy_probe_destroy(void *);
static void number(CFMutableDictionaryRef d,CFStringRef key,unsigned value)
{int32_t n=(int32_t)value;CFNumberRef v=CFNumberCreate(NULL,kCFNumberSInt32Type,&n);if(v){CFDictionarySetValue(d,key,v);CFRelease(v);}}
bool sweetgl_copy_context_selftest(void)
{
    enum {W=8,H=8,PITCH=32};
    struct sweetgl_ctx *ctx[2]={NULL,NULL};GLuint tex[2]={0},fbo[2]={0};uint64_t owner[2]={0};
    IOSurfaceRef surface=NULL;void *probe=NULL;bool ok=false;uint8_t readback[W*H*4];
    CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(!d)goto done;
    number(d,CFSTR("IOSurfaceWidth"),W);number(d,CFSTR("IOSurfaceHeight"),H);number(d,CFSTR("IOSurfaceBytesPerRow"),PITCH);
    number(d,CFSTR("IOSurfaceBytesPerElement"),4);number(d,CFSTR("IOSurfacePixelFormat"),0x52474241);number(d,CFSTR("IOSurfaceAllocSize"),W*H*4);
    surface=IOSurfaceCreate(d);CFRelease(d);if(!surface)goto done;
    if(IOSurfaceLock(surface,0,NULL))goto done;crp_fill(IOSurfaceGetBaseAddress(surface),W,H,PITCH,1,4);
    if(IOSurfaceUnlock(surface,0,NULL))goto done;
    probe=sweetgl_copy_probe_create(surface,GL_RGBA,GL_UNSIGNED_BYTE);if(!probe)goto done;
    ctx[0]=sweetgl_create(2,NULL);ctx[1]=sweetgl_create(2,NULL);if(!ctx[0]||!ctx[1])goto done;
    unsigned uploads=0;uint32_t seed=0;
    for(unsigned step=0;step<4;++step){
        unsigned selected=step==1?1:0;if(!sweetgl_make_current(ctx[selected]))goto done;
        uint64_t current=sweetgl_current_identity();if(!current)goto done;
        if(step<2){owner[selected]=current;glGenTextures(1,&tex[selected]);glGenFramebuffers(1,&fbo[selected]);}
        if(step==1&&(owner[0]==owner[1]||tex[0]!=tex[1]))goto done;
        if(step==3){if(IOSurfaceLock(surface,0,NULL))goto done;crp_fill(IOSurfaceGetBaseAddress(surface),W,H,PITCH,2,4);if(IOSurfaceUnlock(surface,0,NULL))goto done;}
        glBindTexture(GL_TEXTURE_2D,tex[selected]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        int result=sweetgl_copy_probe_update(probe,&uploads,&seed);unsigned images=0,subimages=0;uint64_t cached=0;
        sweetgl_copy_probe_counts(probe,&images,&subimages,&cached);
        if(result!=0||uploads!=step+1||cached!=current||images!=1||subimages!=(step==3))goto done;
        glBindFramebuffer(GL_FRAMEBUFFER,fbo[selected]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[selected],0);
        GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);if(status!=GL_FRAMEBUFFER_COMPLETE)goto done;
        glPixelStorei(GL_PACK_ALIGNMENT,1);memset(readback,0xa5,sizeof(readback));glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,readback);
        GLenum error=glGetError();uint64_t hash=0;unsigned mismatch=crp_check(readback,W,H,step==3?2:1,4,&hash);
        sweet_drm_log("sweetgl: copy context step=%u context=%llu texture=%u images=%u subimages=%u uploads=%u checked=64 mismatches=%u fbo=%x err=%x hash=%016llx",
            step,(unsigned long long)current,tex[selected],images,subimages,uploads,mismatch,status,error,(unsigned long long)hash);
        if(error||mismatch)goto done;
    }
    ok=true;
done:
    for(unsigned i=0;i<2;++i)if(ctx[i]){sweetgl_make_current(ctx[i]);glDeleteFramebuffers(1,&fbo[i]);glDeleteTextures(1,&tex[i]);sweetgl_make_current(NULL);sweetgl_destroy(ctx[i]);}
    sweetgl_copy_probe_destroy(probe);if(surface)CFRelease(surface);
    sweet_drm_log("sweetgl: copy context selftest -> %s",ok?"PASS":"WRONG");return ok;
}
