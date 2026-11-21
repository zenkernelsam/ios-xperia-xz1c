/* Actual GLES sample transfers and combined stencil clear; host author data. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/sweetgl/DepthMultisampleCopyGLES.h"
#include "../../gpu/sweetgl/Stencil8ClearGLES.h"

enum {W=17,H=9,DW=24,DH=14};
static unsigned checked,stencil_checked,copies;
static GLuint stencil_reader;
static const char stencil_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2DMS image;uniform int sample_index;layout(location=0) out uvec4 value;void main(){value=uvec4(texelFetch(image,ivec2(gl_FragCoord.xy),sample_index).r,0,0,0);}";
static GLuint shader(GLenum kind,const char *source)
{GLuint name=glCreateShader(kind);GLint good;glShaderSource(name,1,&source,NULL);glCompileShader(name);glGetShaderiv(name,GL_COMPILE_STATUS,&good);if(!good){char log[2000];glGetShaderInfoLog(name,sizeof(log),NULL,log);fprintf(stderr,"%s\n",log);}assert(good);return name;}
static GLuint make_program(void)
{GLuint v=shader(GL_VERTEX_SHADER,sweet_depth_ms_copy_vertex),f=shader(GL_FRAGMENT_SHADER,stencil_fragment),p=glCreateProgram();GLint good;glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&good);assert(good);glDeleteShader(v);glDeleteShader(f);return p;}
static GLuint integer_texture(unsigned width,unsigned height,GLenum format)
{GLuint name;glGenTextures(1,&name);glBindTexture(GL_TEXTURE_2D,name);glTexStorage2D(GL_TEXTURE_2D,1,format,width,height);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);return name;}
static void read_integer(GLuint output,uint32_t *bytes)
{GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,DW,DH,GL_RGBA_INTEGER,GL_UNSIGNED_INT,bytes);assert(glGetError()==GL_NO_ERROR);glDeleteFramebuffers(1,&fbo);}
static void check_stencil(GLuint depth,GLuint output,unsigned count,unsigned value)
{
    GLuint fbo,vao;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glUseProgram(stencil_reader);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,depth);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_STENCIL_INDEX);glUniform1i(glGetUniformLocation(stencil_reader,"image"),0);
    glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glDisable(GL_BLEND);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glViewport(0,0,W,H);glEnable(GL_SCISSOR_TEST);glScissor(0,0,W,H);glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    for(unsigned sample=0;sample<count;++sample){glUniform1i(glGetUniformLocation(stencil_reader,"sample_index"),sample);glDrawArrays(GL_TRIANGLES,0,3);glFinish();uint32_t bytes[DW*DH*4];read_integer(output,bytes);
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){assert(bytes[(y*DW+x)*4]==value);++stencil_checked;}
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);}
    glDeleteVertexArrays(1,&vao);glDeleteFramebuffers(1,&fbo);
}
static void run_samples(unsigned count,bool combined)
{
    GLuint input=integer_texture(W,H,count==2?GL_RG32UI:GL_RGBA32UI),output=integer_texture(DW,DH,GL_RGBA32UI),depth;
    glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,depth);glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,count,combined?GL_DEPTH32F_STENCIL8:GL_DEPTH_COMPONENT32F,W,H,GL_TRUE);assert(glGetError()==GL_NO_ERROR);
    GLuint load=0,store=0,stencil_clear=0;
    for(unsigned phase=0;phase<5;++phase){uint32_t author[W*H*4],canary[DW*DH*4];unsigned stencil_value=(unsigned[]){3,17,127,171,255}[phase];
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x)for(unsigned s=0;s<count;++s){float value=(float)((x+y*W+s*13+phase*7)%64)/64.0f;memcpy(&author[(y*W+x)*count+s],&value,4);}
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,input);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,count==2?GL_RG_INTEGER:GL_RGBA_INTEGER,GL_UNSIGNED_INT,author);
        if(combined)assert(!sweet_stencil8_clear_samples(depth,W,H,stencil_value,count,&stencil_clear));
        assert(!sweet_depth_multisample_copy_draw(input,depth,W,H,count,false,combined,&load));++copies;
        if(combined)check_stencil(depth,output,count,stencil_value);
        for(unsigned pass=0;pass<2;++pass){
            if(pass && combined){stencil_value^=255;assert(!sweet_stencil8_clear_samples(depth,W,H,stencil_value,count,&stencil_clear));}
            for(unsigned i=0;i<DW*DH*4;++i)canary[i]=0xcdcdcdcd;glBindTexture(GL_TEXTURE_2D,output);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,DW,DH,GL_RGBA_INTEGER,GL_UNSIGNED_INT,canary);
            assert(!sweet_depth_multisample_copy_draw(output,depth,W,H,count,true,combined,&store));++copies;
            uint32_t actual[DW*DH*4+32];for(unsigned i=0;i<DW*DH*4+32;++i)actual[i]=0xe7e7e7e7;read_integer(output,actual+16);
            for(unsigned y=0;y<DH;++y)for(unsigned x=0;x<DW;++x)for(unsigned s=0;s<4;++s){uint32_t expected=0xcdcdcdcd;
                if(x<W && y<H)expected=s<count?author[(y*W+x)*count+s]:0;assert(actual[16+(y*DW+x)*4+s]==expected);++checked;}
            for(unsigned i=0;i<16;++i)assert(actual[i]==0xe7e7e7e7 && actual[DW*DH*4+16+i]==0xe7e7e7e7);
            if(combined)check_stencil(depth,output,count,stencil_value);
        }
    }
    assert(sweet_depth_multisample_copy_draw(input,depth,W,H,3,false,true,&load)==-EINVAL);
    glDeleteProgram(load);glDeleteProgram(store);glDeleteProgram(stencil_clear);glDeleteTextures(1,&input);glDeleteTextures(1,&output);glDeleteTextures(1,&depth);
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLConfig config;EGLint count;
    assert(eglChooseConfig(display,attrs,&config,1,&count) && count);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    printf("Host renderer: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);stencil_reader=make_program();
    run_samples(2,false);run_samples(4,false);run_samples(2,true);run_samples(4,true);assert(glGetError()==GL_NO_ERROR);glDeleteProgram(stencil_reader);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
    printf("Depth MSAA copy PASS copies=%u checked_uint_words=%u stencil_samples=%u float_bits_exact=1 depth_and_stencil_preserved=1 destination_outer_pixels_and_CPU_guards=1 samples=2,4 host_only=1\n",copies,checked,stencil_checked);return 0;
}
