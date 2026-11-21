/* Actual shared depth/stencil state on host Mesa. No FD540 page importer proof. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/sweetgl/DepthStencilGLES.h"
#include "../../gpu/sweetgl/Depth32ClearGLES.h"
#include "../../gpu/sweetgl/Stencil8ClearGLES.h"

enum { WIDTH=17,HEIGHT=9,LEFT=3,TOP=2,BOX_WIDTH=11,BOX_HEIGHT=5 };
static GLuint pipeline;
static unsigned pixel_checks,draws;
static GLuint shader(GLenum kind,const char *source)
{
    GLuint shader=glCreateShader(kind);glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    GLint valid=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&valid);
    if(!valid){char error[4096];glGetShaderInfoLog(shader,sizeof(error),NULL,error);fprintf(stderr,"%s\n",error);}assert(valid);return shader;
}
static bool comparison(unsigned compare,double reference,double stored)
{
    switch(compare){case 0:return false;case 1:return reference<stored;case 2:return reference==stored;
    case 3:return reference<=stored;case 4:return reference>stored;case 5:return reference!=stored;
    case 6:return reference>=stored;default:assert(compare==7);return true;}
}
static void clear(unsigned stencil,float depth)
{
    glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);glStencilMask(255);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(1,0,1,1);glClearDepthf(depth);glClearStencil((GLint)stencil);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);assert(glGetError()==GL_NO_ERROR);
}
static void draw(float depth,bool back,const float colour[4],bool box)
{
    glUseProgram(pipeline);glUniform1f(glGetUniformLocation(pipeline,"z"),depth);
    glUniform1i(glGetUniformLocation(pipeline,"back"),back);glUniform4fv(glGetUniformLocation(pipeline,"colour"),1,colour);
    glViewport(0,0,WIDTH,HEIGHT);glFrontFace(GL_CCW);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);
    if(box){glEnable(GL_SCISSOR_TEST);glScissor(LEFT,TOP,BOX_WIDTH,BOX_HEIGHT);}else glDisable(GL_SCISSOR_TEST);
    glDrawArrays(GL_TRIANGLES,0,3);glFinish();assert(glGetError()==GL_NO_ERROR);++draws;
}
static const float green[]={0,1,0,1},red[]={1,0,0,1},blue[]={0,0,1,1};
static void check(bool inside,bool outside,const uint8_t accepted[4],const uint8_t rejected[4])
{
    uint8_t bytes[WIDTH*HEIGHT*4+32];memset(bytes,0xe7,sizeof(bytes));glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,WIDTH,HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,bytes+16);assert(glGetError()==GL_NO_ERROR);
    for(unsigned i=0;i<16;++i)assert(bytes[i]==0xe7 && bytes[sizeof(bytes)-1-i]==0xe7);
    for(unsigned y=0;y<HEIGHT;++y)for(unsigned x=0;x<WIDTH;++x){bool box=x>=LEFT && x<LEFT+BOX_WIDTH && y>=TOP && y<TOP+BOX_HEIGHT;
        const uint8_t *expected=(box?inside:outside)?accepted:rejected;
        if(memcmp(bytes+16+(y*WIDTH+x)*4,expected,4)){fprintf(stderr,"pixel mismatch x=%u y=%u accepted=%u\n",x,y,box?inside:outside);abort();}
        ++pixel_checks;}
}
static void depth_comparisons(void)
{
    const uint8_t yes[]={0,255,0,255},no[]={255,0,255,255};
    for(unsigned compare=0;compare<8;++compare)for(unsigned z=0;z<3;++z){float incoming=(z+1)*0.25f;clear(0,0.5f);
        SweetDepthStencilState state=sweet_depth_stencil_default();state.compare=compare;
        assert(sweet_depth_stencil_apply(&state,true,false));draw(incoming,false,green,true);
        check(comparison(compare,incoming,0.5),false,yes,no);}
    clear(0,0.75f);SweetDepthStencilState state=sweet_depth_stencil_default();state.compare=1;state.write=1;
    assert(sweet_depth_stencil_apply(&state,true,false));draw(0.25f,false,red,true);
    state.write=0;assert(sweet_depth_stencil_apply(&state,true,false));draw(0.5f,false,green,true);
    const uint8_t written[]={255,0,0,255};check(true,false,written,no);
    state=sweet_depth_stencil_default();assert(sweet_depth_stencil_apply(&state,true,false));draw(0.9f,false,blue,true);
    state.compare=1;assert(sweet_depth_stencil_apply(&state,true,false));draw(0.3f,false,green,true);
    const uint8_t unchanged_depth[]={0,0,255,255};check(true,false,unchanged_depth,no);
    state.write=1;assert(sweet_depth_stencil_apply(&state,false,false));assert(!glIsEnabled(GL_DEPTH_TEST));
    GLboolean write=1;glGetBooleanv(GL_DEPTH_WRITEMASK,&write);assert(!write);
}
static SweetStencilFace face(unsigned compare,unsigned fail,unsigned depth_fail,unsigned pass,unsigned read,unsigned write)
{return (SweetStencilFace){1,compare,fail,depth_fail,pass,read,write};}
static void stencil_comparisons(void)
{
    const uint8_t yes[]={0,255,0,255},no[]={255,0,255,255};
    for(unsigned back=0;back<2;++back)for(unsigned compare=0;compare<8;++compare)for(unsigned ref=2;ref<=4;++ref){
        clear(3,0.5f);SweetDepthStencilState state=sweet_depth_stencil_default();
        if(back){state.back=face(compare,0,0,0,255,0);state.back_reference=ref;}
        else{state.front=face(compare,0,0,0,255,0);state.front_reference=ref;}
        assert(sweet_depth_stencil_apply(&state,true,true));draw(0.75f,back,green,true);
        check(comparison(compare,ref,3),false,yes,no);}
}
static unsigned operation(unsigned kind,unsigned value,unsigned reference)
{
    switch(kind){case 0:return value;case 1:return 0;case 2:return reference;
    case 3:return value<255?value+1:255;case 4:return value?value-1:0;case 5:return value^255;
    case 6:return (value+1)&255;default:assert(kind==7);return (value-1)&255;}
}
static void stencil_updates(void)
{
    const unsigned initial[]={0,1,127,255},masks[]={0,15,240,255};
    const uint8_t yes[]={0,255,0,255},no[]={255,0,255,255};
    for(unsigned back=0;back<2;++back)for(unsigned branch=0;branch<3;++branch)for(unsigned op=0;op<8;++op)
    for(unsigned which=0;which<4;++which)for(unsigned mask=0;mask<4;++mask){
        unsigned stored=initial[which],write=masks[mask],reference=42;clear(stored,0.5f);
        SweetDepthStencilState state=sweet_depth_stencil_default();state.compare=branch==1?0:7;
        SweetStencilFace selected=face(branch==0?0:7,branch==0?op:0,branch==1?op:0,branch==2?op:0,255,write);
        if(back){state.back=selected;state.back_reference=reference;}else{state.front=selected;state.front_reference=reference;}
        assert(sweet_depth_stencil_apply(&state,true,true));draw(0.75f,back,blue,true);
        unsigned next=(operation(op,stored,reference)&write)|(stored&(~write&255));
        glDisable(GL_SCISSOR_TEST);glClearColor(1,0,1,1);glClear(GL_COLOR_BUFFER_BIT);
        state=sweet_depth_stencil_default();state.front=state.back=face(2,0,0,0,255,0);state.front_reference=state.back_reference=next;
        assert(sweet_depth_stencil_apply(&state,true,true));draw(0.25f,false,green,false);
        check(true,next==stored,yes,no);
        // A wrong stencil result must also be rejected; equality alone cannot
        // accidentally pass when testing a bitmask or clamp/wrap boundary.
        state.front_reference=state.back_reference=next^1;assert(sweet_depth_stencil_apply(&state,true,true));
        glDisable(GL_SCISSOR_TEST);glClearColor(1,0,1,1);glClear(GL_COLOR_BUFFER_BIT);draw(0.25f,false,green,false);
        check(false,(next^1)==stored,yes,no);
    }
}
static void rejected_state_preserves_current_GL(void)
{
    SweetDepthStencilState good=sweet_depth_stencil_default();good.compare=1;good.write=1;
    assert(sweet_depth_stencil_apply(&good,true,true));
    for(unsigned which=0;which<8;++which){SweetDepthStencilState bad=good;
        switch(which){case 0:bad.compare=8;break;case 1:bad.write=2;break;case 2:bad.front_reference=256;break;
        case 3:bad.front.enabled=2;break;case 4:bad.back.compare=8;break;case 5:bad.front.fail=8;break;
        case 6:bad.back.depth_fail=8;break;default:bad.back.write_mask=256;break;}
        assert(!sweet_depth_stencil_apply(&bad,true,true));GLint compare=0;glGetIntegerv(GL_DEPTH_FUNC,&compare);
        assert(compare==GL_LESS && glIsEnabled(GL_DEPTH_TEST));}
    assert(!sweet_depth_stencil_apply(NULL,true,true));
}
static void depth_clear_pixels(GLuint texture)
{
    GLuint cache=0;const uint8_t yes[]={0,255,0,255},no[]={255,0,255,255};
    for(unsigned pass=0;pass<2;++pass){clear(0,pass?0.25f:0.75f);
        assert(!sweet_depth32_clear(texture,WIDTH,HEIGHT,pass?0.75:0.25,&cache) && cache);
        SweetDepthStencilState state=sweet_depth_stencil_default();state.compare=1;
        assert(sweet_depth_stencil_apply(&state,true,false));draw(0.5f,false,green,true);
        check(pass!=0,false,yes,no);}
    assert(sweet_depth32_clear(texture,WIDTH,HEIGHT,-0.5,&cache)==-EINVAL);
    glDeleteProgram(cache);
}
static void stencil_clear_pixels(GLuint texture)
{
    GLuint cache=0;const unsigned values[]={0,1,127,255};const uint8_t yes[]={0,255,0,255},no[]={255,0,255,255};
    for(unsigned i=0;i<4;++i){clear(values[i]^255,0.25f);
        assert(!sweet_stencil8_clear(texture,WIDTH,HEIGHT,values[i],&cache) && !cache);
        SweetDepthStencilState state=sweet_depth_stencil_default();state.front=state.back=face(2,0,0,0,255,0);state.front_reference=state.back_reference=values[i];
        assert(sweet_depth_stencil_apply(&state,true,true));draw(0.5f,false,green,true);check(true,false,yes,no);
        state.compare=1;assert(sweet_depth_stencil_apply(&state,true,true));
        glDisable(GL_SCISSOR_TEST);glClearColor(1,0,1,1);glClear(GL_COLOR_BUFFER_BIT);draw(0.5f,false,green,true);check(false,false,yes,no);
        // Template clearing did not replace the Depth32 plane; Less with .5
        // still fails against the original .25 depth value.
    }
    assert(sweet_stencil8_clear(texture,WIDTH,HEIGHT,256,&cache)==-EINVAL);glDeleteProgram(cache);
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(display!=EGL_NO_DISPLAY && eglInitialize(display,NULL,NULL));
    assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
    EGLConfig config;EGLint count;assert(eglChooseConfig(display,attrs,&config,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    printf("Host renderer: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    const char *vertex="#version 310 es\nuniform float z;uniform bool back;void main(){int i=back?(gl_VertexID==1?2:gl_VertexID==2?1:0):gl_VertexID;vec2 p=i==0?vec2(-1,-1):i==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,z*2.0-1.0,1);}";
    const char *fragment="#version 310 es\nprecision highp float;uniform vec4 colour;layout(location=0) out vec4 result;void main(){result=colour;}";
    GLuint vs=shader(GL_VERTEX_SHADER,vertex),fs=shader(GL_FRAGMENT_SHADER,fragment);pipeline=glCreateProgram();glAttachShader(pipeline,vs);glAttachShader(pipeline,fs);
    glLinkProgram(pipeline);GLint valid=0;glGetProgramiv(pipeline,GL_LINK_STATUS,&valid);assert(valid);glDeleteShader(vs);glDeleteShader(fs);
    GLuint vao,fbo,colour,depth;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenTextures(1,&colour);glBindTexture(GL_TEXTURE_2D,colour);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,WIDTH,HEIGHT);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,colour,0);
    glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH32F_STENCIL8,WIDTH,HEIGHT);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,depth,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glDisable(GL_DITHER);
    depth_comparisons();stencil_comparisons();stencil_updates();rejected_state_preserves_current_GL();depth_clear_pixels(depth);stencil_clear_pixels(depth);
    assert(glGetError()==GL_NO_ERROR);glDeleteTextures(1,&depth);glDeleteTextures(1,&colour);glDeleteFramebuffers(1,&fbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(pipeline);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
    printf("Depth/stencil GLES PASS: draws=%u checked_pixels=%u all_compare_ops=8 all_stencil_ops=8 front_back=1 scissor_masks_guards=1 host_only=1\n",draws,pixel_checks);
    return 0;
}
