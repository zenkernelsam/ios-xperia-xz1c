#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "eagl.h"
#include "sweetgl.h"
#include "sweet_drm.h"
void *sweetgl_bridge_probe_create(void);
void sweetgl_bridge_probe_destroy(void *);
bool sweetgl_bridge_probe_surface(void *,IOSurfaceRef,unsigned);
void sweetgl_bridge_probe_counts(void *,unsigned,unsigned *,unsigned *);
void sweetgl_bridge_probe_swizzle(void *,bool);

static void number(CFMutableDictionaryRef d,CFStringRef key,unsigned value)
{
    int32_t n=(int32_t)value;CFNumberRef v=CFNumberCreate(NULL,kCFNumberSInt32Type,&n);
    if(v){CFDictionarySetValue(d,key,v);CFRelease(v);}
}
static IOSurfaceRef make_surface(unsigned w,unsigned h)
{
    CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(!d)return NULL;
    number(d,CFSTR("IOSurfaceWidth"),w);number(d,CFSTR("IOSurfaceHeight"),h);
    number(d,CFSTR("IOSurfaceBytesPerRow"),w*4);number(d,CFSTR("IOSurfaceBytesPerElement"),4);
    number(d,CFSTR("IOSurfacePixelFormat"),0x52474241);number(d,CFSTR("IOSurfaceAllocSize"),w*h*4);
    IOSurfaceRef s=IOSurfaceCreate(d);CFRelease(d);return s;
}
static bool fill(IOSurfaceRef s,unsigned selected,unsigned pass)
{
    if(IOSurfaceLock(s,0,NULL))return false;
    uint8_t *p=IOSurfaceGetBaseAddress(s);
    for(unsigned i=0;i<IOSurfaceGetWidth(s)*IOSurfaceGetHeight(s);++i){
        unsigned x=i%IOSurfaceGetWidth(s),y=i/IOSurfaceGetWidth(s);
        p[i*4]=(uint8_t)(x*17+y*23+selected*79+31);
        p[i*4+1]=(uint8_t)(x*7+y*13+pass*113+47);
        p[i*4+2]=(uint8_t)(x*11+y*5+selected*67+101);p[i*4+3]=255;
    }
    return !IOSurfaceUnlock(s,0,NULL);
}
static GLuint shader(void *c,GLenum type,const char *source)
{
    GLuint s=glCreateShader(type);const GLcharARB *p=source;
    sgl_ShaderSourceARB(c,(GLhandleARB)(uintptr_t)s,1,&p,NULL);
    sgl_CompileShaderARB(c,(GLhandleARB)(uintptr_t)s);
    GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;
}

bool sweetgl_bridge_state_selftest(void)
{
    enum {OUT=8};void *ctx[2]={0};IOSurfaceRef surface[2]={0};
    GLuint tex[2][2]={{0}},fbo[2]={0},program[2]={0},vs[2]={0},fs[2]={0};
    uint64_t identities[2]={0};bool ok=false;uint8_t pixels[OUT*OUT*4];
    const GLfloat vertices[]={-1,-1, 1,-1, -1,1, 1,1};
    const char *vertex="attribute vec2 pos; varying vec2 uv; void main(){ uv=(pos+1.0)*0.5; gl_Position=vec4(pos,0.0,1.0); }";
    const char *fragment[2]={
        "precision highp float; uniform sampler2DRect image; varying vec2 uv; void main(){ gl_FragColor=texture2DRect(image,uv*vec2(7.0,5.0)); }",
        "precision highp float; uniform sampler2D image; varying vec2 uv; void main(){ gl_FragColor=texture2D(image,uv); }"};
    for(unsigned i=0;i<2;++i){
        ctx[i]=sweetgl_bridge_probe_create();if(!ctx[i])goto done;sgl_bind(ctx[i]);identities[i]=sweetgl_current_identity();
        if(!identities[i]||(i&&identities[0]==identities[1]))goto done;
        glGenTextures(2,tex[i]);glGenFramebuffers(1,&fbo[i]);
        glBindTexture(GL_TEXTURE_2D,tex[i][1]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,OUT,OUT,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo[i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[i][1],0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
        surface[i]=make_surface(i?9:7,i?3:5);if(!surface[i]||!fill(surface[i],i,0))goto done;
        glActiveTexture(GL_TEXTURE0);
        sgl_BindTexture(ctx[i],i?GL_TEXTURE_2D:0x84F5,tex[i][0]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        if(!sweetgl_bridge_probe_surface(ctx[i],surface[i],i?GL_TEXTURE_2D:0x84F5))goto done;
        sweetgl_bridge_probe_swizzle(ctx[i],i==0);
        vs[i]=shader(ctx[i],GL_VERTEX_SHADER,vertex);fs[i]=shader(ctx[i],GL_FRAGMENT_SHADER,fragment[i]);
        if(!vs[i]||!fs[i])goto done;program[i]=glCreateProgram();glAttachShader(program[i],vs[i]);glAttachShader(program[i],fs[i]);
        glBindAttribLocation(program[i],0,"pos");sgl_LinkProgramARB(ctx[i],(GLhandleARB)(uintptr_t)program[i]);
        GLint linked=0;glGetProgramiv(program[i],GL_LINK_STATUS,&linked);if(!linked)goto done;
        glUseProgram(program[i]);glUniform1i(glGetUniformLocation(program[i],"image"),0);
        glViewport(0,0,OUT,OUT);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,vertices);glEnableVertexAttribArray(0);
        if(sgl_GetError(ctx[i]))goto done;
    }
    if(tex[0][0]!=tex[1][0]||program[0]!=program[1]||vs[0]!=vs[1]||fs[0]!=fs[1])goto done;
    for(unsigned step=0;step<6;++step){
        unsigned i=step&1,pass=step>=4;sgl_bind(ctx[i]);
        if(pass&&!fill(surface[i],i,1))goto done;
        /* No rebinding: only production draw_prep can select and refresh its own records. */
        sgl_DrawArrays(ctx[i],GL_TRIANGLE_STRIP,0,4);
        memset(pixels,0xa5,sizeof(pixels));glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,OUT,OUT,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        unsigned mismatch=0,images=0,updates=0;
        uint64_t hash=14695981039346656037ull;
        for(unsigned p=0;p<OUT*OUT;++p){
            unsigned w=i?9:7,h=i?3:5;
            unsigned x=((2*(p%OUT)+1)*w)/(2*OUT),y=((2*(p/OUT)+1)*h)/(2*OUT);
            uint8_t expected[4]={(uint8_t)(x*17+y*23+i*79+31),(uint8_t)(x*7+y*13+pass*113+47),(uint8_t)(x*11+y*5+i*67+101),255};
            if(i==0){uint8_t r=expected[0],g=expected[1];expected[0]=255;expected[1]=expected[2];expected[2]=g;expected[3]=r;}
            mismatch+=memcmp(pixels+p*4,expected,4)!=0;
            for(unsigned b=0;b<4;++b)hash=(hash^pixels[p*4+b])*1099511628211ull;
        }
        sweetgl_bridge_probe_counts(ctx[i],tex[i][0],&images,&updates);
        GLenum err=sgl_GetError(ctx[i]);
        sweet_drm_log("sweetgl: bridge state step=%u context=%llu texture=%u program=%u width=%u height=%u images=%u updates=%u checked=64 mismatches=%u err=%x hash=%016llx",
            step,(unsigned long long)sweetgl_current_identity(),tex[i][0],program[i],i?9:7,i?3:5,images,updates,mismatch,err,(unsigned long long)hash);
        if(err||mismatch||images!=1||updates!=pass)goto done;
    }
    ok=true;
done:
    for(unsigned i=0;i<2;++i){
        if(ctx[i]){
            sgl_bind(ctx[i]);sgl_DeleteTextures(ctx[i],2,tex[i]);glDeleteFramebuffers(1,&fbo[i]);
            if(program[i])sgl_DeleteObjectARB(ctx[i],(GLhandleARB)(uintptr_t)program[i]);
            if(vs[i])sgl_DeleteObjectARB(ctx[i],(GLhandleARB)(uintptr_t)vs[i]);
            if(fs[i])sgl_DeleteObjectARB(ctx[i],(GLhandleARB)(uintptr_t)fs[i]);
            sweetgl_bridge_probe_destroy(ctx[i]);
        }
        if(surface[i])CFRelease(surface[i]);
    }
    sweet_drm_log("sweetgl: bridge state selftest -> %s",ok?"PASS":"WRONG");return ok;
}
