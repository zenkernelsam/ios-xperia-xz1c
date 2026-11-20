/* Actual GLES draws for the eight authored transient-pass pixel recipes.
 * Native objects and FD540 external-page import are checked separately. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../gpu/sweetgl/DepthStencilGLES.h"
#include "../../gpu/sweetgl/Depth32ClearGLES.h"
#include "../../gpu/sweetgl/Stencil8ClearGLES.h"
enum {W=17,H=9,N=W*H};
struct pass {GLuint color,zs,fbo;};
static GLuint draw_program,read_depth_program,read_stencil_program,read_fbo,read_texture;
static GLuint clear_depth_cache,clear_stencil_cache;
static unsigned draws,bytes_checked;
static GLuint compile(GLenum kind,const char *source)
{
    GLuint shader=glCreateShader(kind);GLint valid=0;glShaderSource(shader,1,&source,NULL);glCompileShader(shader);glGetShaderiv(shader,GL_COMPILE_STATUS,&valid);
    if(!valid){char log[2000];glGetShaderInfoLog(shader,sizeof(log),NULL,log);fprintf(stderr,"%s\n",log);}assert(valid);return shader;
}
static GLuint program(const char *fragment)
{
    const char *vertex="#version 310 es\nuniform float z;void main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,z*2.0-1.0,1);}";
    GLuint v=compile(GL_VERTEX_SHADER,vertex),f=compile(GL_FRAGMENT_SHADER,fragment),p=glCreateProgram();GLint valid=0;
    glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&valid);assert(valid);glDeleteShader(v);glDeleteShader(f);return p;
}
static struct pass create_pass(void)
{
    struct pass p={0};glGenTextures(1,&p.color);glBindTexture(GL_TEXTURE_2D,p.color);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,W,H);
    glGenTextures(1,&p.zs);glBindTexture(GL_TEXTURE_2D,p.zs);glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH32F_STENCIL8,W,H);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glGenFramebuffers(1,&p.fbo);glBindFramebuffer(GL_FRAMEBUFFER,p.fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,p.color,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,p.zs,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);return p;
}
static void draw(float z)
{glUseProgram(draw_program);glUniform1f(glGetUniformLocation(draw_program,"z"),z);glDrawArrays(GL_TRIANGLES,0,3);++draws;}
static void render(struct pass p,unsigned phase)
{
    bool depth=phase<2 || phase==3 || phase>=6,stencil=phase==2 || phase==3 || phase>=6;
    glBindFramebuffer(GL_FRAMEBUFFER,p.fbo);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(1,0,1,1);if(phase==5)glClearColor(0,1,1,1);glClear(GL_COLOR_BUFFER_BIT);
    if(depth)assert(!sweet_depth32_clear(p.zs,W,H,phase==1 || phase==7?0.125:0.75,&clear_depth_cache));
    if(stencil)assert(!sweet_stencil8_clear(p.zs,W,H,phase==3?127:phase==7?129:17,&clear_stencil_cache));
    if(phase==5)return;
    SweetDepthStencilState state=sweet_depth_stencil_default();state.compare=depth?1:7;state.write=depth;
    state.front=(SweetStencilFace){1,phase==2?2:7,0,phase==7?5:phase==3?3:0,phase==2?6:2,255,255};
    state.front_reference=phase==2?17:42;state.back=state.front;state.back_reference=state.front_reference;
    assert(sweet_depth_stencil_apply(&state,depth,stencil));glViewport(0,0,W,H);glEnable(GL_SCISSOR_TEST);glScissor(3,2,11,5);glFrontFace(GL_CCW);
    draw(phase==1 || phase==7?0.5f:0.25f);if(phase==0 || phase==2 || phase==3)draw(0.5f);
    glFinish();assert(glGetError()==GL_NO_ERROR);
}
static void checked_pixels(struct pass p,unsigned phase)
{
    unsigned char color[N*4+32];memset(color,0xe7,sizeof(color));glBindFramebuffer(GL_FRAMEBUFFER,p.fbo);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,color+16);assert(glGetError()==GL_NO_ERROR);
    for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){bool box=x>=3 && x<14 && y>=2 && y<7;
        const uint8_t yes[]={64,128,191,255},no[]={255,0,255,255},cyan[]={0,255,255,255};
        const uint8_t *wanted=phase==5?cyan:box && phase!=1 && phase!=7?yes:no;
        assert(!memcmp(color+16+(y*W+x)*4,wanted,4));bytes_checked+=4;}
    for(unsigned i=0;i<16;++i)assert(color[i]==0xe7 && color[sizeof(color)-1-i]==0xe7);
    for(unsigned mode=0;mode<2;++mode){bool present=mode==0?(phase<2 || phase==3 || phase>=6):(phase==2 || phase==3 || phase>=6);if(!present)continue;
        glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDepthMask(GL_FALSE);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,p.zs);glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_STENCIL_TEXTURE_MODE,mode?GL_STENCIL_INDEX:GL_DEPTH_COMPONENT);
        GLuint selected=mode?read_stencil_program:read_depth_program;glUseProgram(selected);glUniform1i(glGetUniformLocation(selected,"source_image"),0);glViewport(0,0,W,H);
        glDrawArrays(GL_TRIANGLES,0,3);++draws;glFinish();memset(color,0xe7,sizeof(color));glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,color+16);assert(glGetError()==GL_NO_ERROR);
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){bool box=x>=3 && x<14 && y>=2 && y<7;uint32_t wanted;
            if(mode){wanted=phase==2?(box?18:17):phase==7?(box?126:129):box?(phase==3?43:42):(phase==3?127:17);}
            else {float z=phase==1 || phase==7?0.125f:box?0.25f:0.75f;memcpy(&wanted,&z,4);}
            uint32_t actual;memcpy(&actual,color+16+(y*W+x)*4,4);assert(actual==wanted);bytes_checked+=4;}
        for(unsigned i=0;i<16;++i)assert(color[i]==0xe7 && color[sizeof(color)-1-i]==0xe7);}
}
static void destroy_pass(struct pass *p){glDeleteTextures(1,&p->color);glDeleteTextures(1,&p->zs);glDeleteFramebuffers(1,&p->fbo);memset(p,0,sizeof(*p));}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count) && count);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));printf("Host renderer: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_DITHER);
    draw_program=program("#version 310 es\nprecision highp float;layout(location=0) out vec4 color;void main(){color=vec4(0.25,0.5,0.75,1);}");
    read_depth_program=program("#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2D source_image;layout(location=0) out vec4 color;void main(){uint raw=floatBitsToUint(texelFetch(source_image,ivec2(gl_FragCoord.xy),0).r);color=vec4(uvec4(raw,raw>>8u,raw>>16u,raw>>24u)&255u)/255.0;}");
    read_stencil_program=program("#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2D source_image;layout(location=0) out vec4 color;void main(){uint raw=texelFetch(source_image,ivec2(gl_FragCoord.xy),0).r;color=vec4(float(raw)/255.0,0,0,0);}");
    glGenTextures(1,&read_texture);glBindTexture(GL_TEXTURE_2D,read_texture);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,W,H);
    glGenFramebuffers(1,&read_fbo);glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,read_texture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    for(unsigned phase=0;phase<6;++phase){struct pass p=create_pass();render(p,phase);checked_pixels(p,phase);destroy_pass(&p);}
    struct pass a=create_pass(),b=create_pass();assert(a.zs!=b.zs && a.color!=b.color);render(a,6);render(b,7);checked_pixels(a,6);checked_pixels(b,7);destroy_pass(&a);destroy_pass(&b);
    glDeleteProgram(draw_program);glDeleteProgram(read_depth_program);glDeleteProgram(read_stencil_program);glDeleteProgram(clear_depth_cache);glDeleteProgram(clear_stencil_cache);
    glDeleteTextures(1,&read_texture);glDeleteFramebuffers(1,&read_fbo);glDeleteVertexArrays(1,&vao);assert(glGetError()==GL_NO_ERROR);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
    printf("Memoryless GLES recipes PASS: phases=8 draws=%u checked_bytes=%u exact_depth_stencil_pixels=1 independent_stores=1 host_only=1\n",draws,bytes_checked);return 0;
}
