#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "eagl.h"
#include "sweetgl.h"
#include "sweet_drm.h"
void *sweetgl_bridge_probe_create(void);
void sweetgl_bridge_probe_destroy(void *);
bool sweetgl_bridge_probe_surface(void *,IOSurfaceRef,unsigned);
void sweetgl_program_probe_counts(void *,unsigned,unsigned *,unsigned *,unsigned *,unsigned *);
static void number(CFMutableDictionaryRef d,CFStringRef key,unsigned value)
{
    int32_t n=(int32_t)value;CFNumberRef v=CFNumberCreate(NULL,kCFNumberSInt32Type,&n);
    if(v){CFDictionarySetValue(d,key,v);CFRelease(v);}
}
static IOSurfaceRef surface(unsigned selected)
{
    unsigned w=selected?9:7,h=selected?3:5;
    CFMutableDictionaryRef d=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(!d)return NULL;
    number(d,CFSTR("IOSurfaceWidth"),w);number(d,CFSTR("IOSurfaceHeight"),h);
    number(d,CFSTR("IOSurfaceBytesPerRow"),w*4);number(d,CFSTR("IOSurfaceBytesPerElement"),4);
    number(d,CFSTR("IOSurfacePixelFormat"),0x52474241);number(d,CFSTR("IOSurfaceAllocSize"),w*h*4);
    IOSurfaceRef s=IOSurfaceCreate(d);CFRelease(d);if(!s)return NULL;
    if(IOSurfaceLock(s,0,NULL)){CFRelease(s);return NULL;}
    uint8_t *p=IOSurfaceGetBaseAddress(s);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        unsigned at=(y*w+x)*4;p[at]=(uint8_t)(x*17+y*23+selected*79+31);
        p[at+1]=(uint8_t)(x*7+y*13+47);p[at+2]=(uint8_t)(x*11+y*5+selected*67+101);p[at+3]=255;
    }
    if(IOSurfaceUnlock(s,0,NULL)){CFRelease(s);return NULL;}return s;
}
static bool compile(void *c,GLuint sh,const char *source)
{
    const GLcharARB *s=source;sgl_ShaderSourceARB(c,(GLhandleARB)(uintptr_t)sh,1,&s,NULL);
    sgl_CompileShaderARB(c,(GLhandleARB)(uintptr_t)sh);GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);return ok!=0;
}
static bool probe_link(void *c,GLuint program)
{
    sgl_LinkProgramARB(c,(GLhandleARB)(uintptr_t)program);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);return ok!=0;
}
static bool draw(void *c,unsigned selected,uint64_t *aggregate,uint64_t *frame)
{
    uint8_t pixels[8*8*4];sgl_DrawArrays(c,GL_TRIANGLE_STRIP,0,4);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,8,8,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    *frame=14695981039346656037ull;unsigned mismatch=0,w=selected?9:7,h=selected?3:5;
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){
        unsigned tx=((x*2+1)*w)/16,ty=((y*2+1)*h)/16,at=(y*8+x)*4;
        uint8_t expected[4]={(uint8_t)(tx*17+ty*23+selected*79+31),(uint8_t)(tx*7+ty*13+47),(uint8_t)(tx*11+ty*5+selected*67+101),255};
        mismatch+=memcmp(pixels+at,expected,4)!=0;
        for(unsigned b=0;b<4;++b){*frame=(*frame^pixels[at+b])*1099511628211ull;if(aggregate)*aggregate=(*aggregate^pixels[at+b])*1099511628211ull;}
    }
    GLenum error=sgl_GetError(c);if(mismatch||error)sweet_drm_log("sweetgl: program life pixels selected=%u mismatches=%u err=%x",selected,mismatch,error);
    return !mismatch&&!error;
}
static void uniforms(void *c,GLuint program,unsigned selected,bool vector)
{
    GLint sampler=glGetUniformLocation(program,"image"),unit=(GLint)selected;
    if(vector)sgl_Uniform1iv(c,sampler,1,&unit);else sgl_Uniform1i(c,sampler,unit);
    GLint extent=glGetUniformLocation(program,"extent"),extra=glGetUniformLocation(program,"extra");
    if(extent>=0)glUniform2f(extent,selected?9.0f:7.0f,selected?3.0f:5.0f);
    if(extra>=0)glUniform4f(extra,0,0,0,0);
}
bool sweetgl_program_lifecycle_selftest(void)
{
    void *c=sweetgl_bridge_probe_create();IOSurfaceRef s[2]={0};GLuint tex[3]={0},fbo=0,vs=0,fs=0,program=0;
    bool ok=false;uint64_t aggregate=14695981039346656037ull,frame=0;unsigned draws=0,maxsh=0,maxpr=0;
    const GLfloat vertices[]={-1,-1,1,-1,-1,1,1,1};
    const char *vertex="attribute vec2 pos; varying vec2 uv; void main(){ uv=(pos+1.0)*0.5; gl_Position=vec4(pos,0.0,1.0); }";
    const char *fragment[2]={
        "precision highp float; uniform sampler2D image; uniform vec4 extra; varying vec2 uv; void main(){ gl_FragColor=texture2D(image,uv)+extra; }",
        "precision highp float; uniform sampler2DRect image; uniform vec2 extent; uniform vec4 extra; varying vec2 uv; void main(){ gl_FragColor=texture2DRect(image,uv*extent)+extra; }"};
    if(!c)goto done;sgl_bind(c);glGenTextures(3,tex);glGenFramebuffers(1,&fbo);
    glActiveTexture(GL_TEXTURE2);sgl_BindTexture(c,GL_TEXTURE_2D,tex[2]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,8,8,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[2],0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    for(unsigned i=0;i<2;++i){
        s[i]=surface(i);if(!s[i])goto done;glActiveTexture(GL_TEXTURE0+i);
        sgl_BindTexture(c,GL_TEXTURE_2D,tex[i]);sgl_BindTexture(c,0x84f5,tex[i]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        if(!sweetgl_bridge_probe_surface(c,s[i],GL_TEXTURE_2D))goto done;
    }
    vs=(GLuint)(uintptr_t)sgl_CreateShaderObjectARB(c,GL_VERTEX_SHADER);fs=(GLuint)(uintptr_t)sgl_CreateShaderObjectARB(c,GL_FRAGMENT_SHADER);
    program=(GLuint)(uintptr_t)sgl_CreateProgramObjectARB(c);if(!vs||!fs||!program||!compile(c,vs,vertex))goto done;
    glAttachShader(program,vs);glAttachShader(program,fs);glBindAttribLocation(program,0,"pos");
    glViewport(0,0,8,8);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,vertices);glEnableVertexAttribArray(0);
    for(unsigned step=0;step<160;++step){
        unsigned rectangle=(step&1)==0,selected=(step/2)&1,sh=0,pr=0,fetch=0,mask=0;
        if(!compile(c,fs,fragment[rectangle])||!probe_link(c,program))goto done;
        sgl_UseProgramObjectARB(c,(GLhandleARB)(uintptr_t)program);uniforms(c,program,selected,false);
        if(!draw(c,selected,&aggregate,&frame))goto done;++draws;
        sweetgl_program_probe_counts(c,program,&sh,&pr,&fetch,&mask);
        if(sh!=rectangle||pr!=rectangle||fetch||mask!=(0x8000u|(1u<<selected)))goto done;
        if(sh>maxsh)maxsh=sh;if(pr>maxpr)maxpr=pr;
        uniforms(c,program,selected^1,true);sweetgl_program_probe_counts(c,program,&sh,&pr,&fetch,&mask);
        if(mask||!draw(c,selected^1,&aggregate,&frame))goto done;++draws;
        sweetgl_program_probe_counts(c,program,&sh,&pr,&fetch,&mask);
        if(mask!=(0x8000u|(1u<<(selected^1))))goto done;
        if(step%16==0||step==159)sweet_drm_log("sweetgl: program life cycle step=%u context=%llu shader=%u program=%u rectangle=%u shaders=%u programs=%u mask=%x draws=%u pixels=%u mismatches=0 hash=%016llx",
            step,(unsigned long long)sweetgl_current_identity(),fs,program,rectangle,sh,pr,mask,draws,draws*64,(unsigned long long)aggregate);
    }
    /* New source without compile must not replace the attached shader's compiled hints. */
    if(!compile(c,fs,fragment[1])||!probe_link(c,program))goto done;
    const GLcharARB *pending=fragment[0];sgl_ShaderSourceARB(c,(GLhandleARB)(uintptr_t)fs,1,&pending,NULL);
    if(!probe_link(c,program))goto done;sgl_UseProgramObjectARB(c,(GLhandleARB)(uintptr_t)program);uniforms(c,program,1,false);
    if(!draw(c,1,NULL,&frame))goto done;
    sweet_drm_log("sweetgl: program life staged compiled=1 checked=64 mismatches=0 hash=%016llx",(unsigned long long)frame);
    /* Failed relink cache retention is observed here; rendering after failed link is separate. */
    if(compile(c,fs,"precision highp float; bad!")||probe_link(c,program))goto done;
    unsigned sh=0,pr=0,fetch=0,mask=0;sweetgl_program_probe_counts(c,program,&sh,&pr,&fetch,&mask);
    if(pr!=1||mask!=0x8002)goto done;
    sweet_drm_log("sweetgl: program life failed_link cache_retained=1 mask=%x gpu_draw_test=0",mask);
    if(!compile(c,fs,fragment[1])||!probe_link(c,program))goto done;
    sgl_UseProgramObjectARB(c,(GLhandleARB)(uintptr_t)program);uniforms(c,program,1,false);
    sgl_DeleteObjectARB(c,(GLhandleARB)(uintptr_t)fs);
    if(!glIsShader(fs)||!probe_link(c,program))goto done;
    uniforms(c,program,1,false);if(!draw(c,1,NULL,&frame))goto done;
    sweet_drm_log("sweetgl: program life deferred_shader alive=1 checked=64 mismatches=0 hash=%016llx",(unsigned long long)frame);
    GLuint oldshader=fs,oldprogram=program;sgl_DeleteObjectARB(c,(GLhandleARB)(uintptr_t)program);
    if(!glIsProgram(program)||!draw(c,1,NULL,&frame))goto done;
    sweet_drm_log("sweetgl: program life deferred_program alive=1 checked=64 mismatches=0 hash=%016llx",(unsigned long long)frame);
    sgl_UseProgramObjectARB(c,(GLhandleARB)0);
    bool program_alive=glIsProgram(program),shader_alive=glIsShader(fs);
    sweet_drm_log("sweetgl: program life retired shader=%u program=%u shader_alive=%u program_alive=%u",fs,program,shader_alive,program_alive);
    if(program_alive||shader_alive)goto done;
    fs=(GLuint)(uintptr_t)sgl_CreateShaderObjectARB(c,GL_FRAGMENT_SHADER);program=(GLuint)(uintptr_t)sgl_CreateProgramObjectARB(c);
    bool reused=fs==oldshader&&program==oldprogram;
    sweet_drm_log("sweetgl: program life new_names old_shader=%u old_program=%u shader=%u program=%u same_names=%u",oldshader,oldprogram,fs,program,reused);
    if(!fs||!program||!reused||!compile(c,fs,fragment[0]))goto done;
    glAttachShader(program,vs);glAttachShader(program,fs);glBindAttribLocation(program,0,"pos");if(!probe_link(c,program))goto done;
    sgl_UseProgramObjectARB(c,(GLhandleARB)(uintptr_t)program);uniforms(c,program,0,false);if(!draw(c,0,NULL,&frame))goto done;
    sweetgl_program_probe_counts(c,program,&sh,&pr,&fetch,&mask);if(sh||pr||fetch)goto done;
    sweet_drm_log("sweetgl: program life recycled shader=%u program=%u same_names=1 shaders=0 programs=0 fetches=0 checked=64 mismatches=0 hash=%016llx",fs,program,(unsigned long long)frame);
    ok=true;
done:
    if(c){sgl_bind(c);sgl_UseProgramObjectARB(c,(GLhandleARB)0);
        if(glIsProgram(program))sgl_DeleteObjectARB(c,(GLhandleARB)(uintptr_t)program);
        if(glIsShader(vs))sgl_DeleteObjectARB(c,(GLhandleARB)(uintptr_t)vs);
        if(glIsShader(fs))sgl_DeleteObjectARB(c,(GLhandleARB)(uintptr_t)fs);
        sgl_DeleteTextures(c,3,tex);glDeleteFramebuffers(1,&fbo);sweetgl_bridge_probe_destroy(c);}
    for(unsigned i=0;i<2;++i)if(s[i])CFRelease(s[i]);
    sweet_drm_log("sweetgl: program life selftest -> %s cycles=160 draws=%u pixels=%u max_shaders=%u max_programs=%u extra_draws=4 hash=%016llx",
        ok?"PASS":"WRONG",draws,draws*64,maxsh,maxpr,(unsigned long long)aggregate);return ok;
}
