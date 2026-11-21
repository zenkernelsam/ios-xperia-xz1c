/* Exercise the production stencil transfers on host GLES with authored data. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../gpu/sweetgl/StencilSampleCopyGLES.h"
#include "../../gpu/sweetgl/Stencil8ClearGLES.h"
#include "../../gpu/sweetgl/Depth32ClearGLES.h"
#include "../../gpu/sweetgl/DepthMultisampleCopyGLES.h"

enum {W=17,H=9,DW=24,DH=14,WORDS=DW*DH*4};
static unsigned copies,checked,depth_checked,clears,masked_draws;
static unsigned current_phase;
static GLuint load,store[2],depth_load,depth_store,depth_clear,stencil_clear;
static GLuint scalar_depth_reader,constant_stencil;
static const char scalar_depth_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2D image;layout(location=0) out uvec4 value;void main(){value=uvec4(floatBitsToUint(texelFetch(image,ivec2(gl_FragCoord.xy),0).r),0,0,0);}";
/* Author the rectangle in fragment coordinates as well as GL scissor. Host
 * llvmpipe rounds requested 2x to 4x and its empty-fragment stencil path can
 * shift sample1's left scissor edge. Keep that diagnostic; do not change the
 * expected byte rectangle to accommodate it. This is not the transfer shader. */
static const char constant_fragment[]="#version 310 es\nprecision highp float;precision highp int;void main(){ivec2 p=ivec2(gl_FragCoord.xy);if(p.x<3 || p.x>=14 || p.y<2 || p.y>=7)discard;}";

static GLuint program(const char *fragment)
{
    GLuint shaders[2],p=glCreateProgram();const char *sources[]={sweet_stencil_copy_vertex,fragment};
    const GLenum types[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};GLint good;
    for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&good);if(!good){char log[2000];glGetShaderInfoLog(shaders[i],sizeof(log),NULL,log);fprintf(stderr,"%s\n",log);}assert(good);glAttachShader(p,shaders[i]);}
    glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&good);assert(good);for(unsigned i=0;i<2;++i)glDeleteShader(shaders[i]);return p;
}
static GLenum channels(unsigned count)
{return count==1?GL_RED_INTEGER:count==2?GL_RG_INTEGER:GL_RGBA_INTEGER;}
static GLuint integer_texture(unsigned width,unsigned height,GLenum format)
{
    GLuint name;glGenTextures(1,&name);glBindTexture(GL_TEXTURE_2D,name);glTexStorage2D(GL_TEXTURE_2D,1,format,width,height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);return name;
}
static void read_integer(GLuint texture,uint32_t *data)
{
    GLuint fbo;GLint previous;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previous);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,DW,DH,GL_RGBA_INTEGER,GL_UNSIGNED_INT,data);assert(glGetError()==GL_NO_ERROR);
    glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previous);glDeleteFramebuffers(1,&fbo);
}
static void hostile_state(void)
{
    glEnable(GL_SAMPLE_MASK);glSampleMaski(0,0);glEnable(GL_STENCIL_TEST);glStencilMask(0);glStencilFunc(GL_NEVER,9,0);glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_NEVER);glDepthMask(GL_TRUE);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glEnable(GL_SCISSOR_TEST);glScissor(DW,DH,1,1);glActiveTexture(GL_TEXTURE4);
}
static void check_copy(GLuint combined,GLuint output,unsigned samples,const uint8_t *expected,unsigned width,unsigned height)
{
    uint8_t canary[DW*DH*4];memset(canary,0xcd,sizeof(canary));glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,output);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,DW,DH,channels(samples),GL_UNSIGNED_BYTE,canary);
    hostile_state();GLint old_fbo;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&old_fbo);
    assert(!sweet_stencil_sample_copy_draw(output,combined,width,height,samples,true,&store[samples>1]));++copies;
    GLint actual;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&actual);assert(actual==old_fbo);glGetIntegerv(GL_ACTIVE_TEXTURE,&actual);assert(actual==GL_TEXTURE4);
    uint32_t data[WORDS+32];for(unsigned i=0;i<WORDS+32;++i)data[i]=0xe7e7e7e7;read_integer(output,data+16);
    for(unsigned y=0;y<DH;++y)for(unsigned x=0;x<DW;++x)for(unsigned s=0;s<4;++s){
        unsigned want=s<samples?(x<width && y<height?expected[(y*DW+x)*samples+s]:0xcd):(s==3?1:0);
        if(data[16+(y*DW+x)*4+s]!=want){fprintf(stderr,"stencil phase=%u copies=%u samples=%u x=%u y=%u sample=%u got=%u want=%u\n",current_phase,copies,samples,x,y,s,data[16+(y*DW+x)*4+s],want);
            for(unsigned yy=1;yy<8;++yy){for(unsigned xx=1;xx<15;++xx)fprintf(stderr,"%u/%u ",data[16+(yy*DW+xx)*4],data[16+(yy*DW+xx)*4+1]);fprintf(stderr,"\n");}}
        assert(data[16+(y*DW+x)*4+s]==want);++checked;
    }
    for(unsigned i=0;i<16;++i)assert(data[i]==0xe7e7e7e7 && data[WORDS+16+i]==0xe7e7e7e7);
}
static void check_depth(GLuint combined,GLuint output,unsigned samples,const uint32_t *expected)
{
    if(samples>1)assert(!sweet_depth_multisample_copy_draw(output,combined,DW,DH,samples,true,true,&depth_store));
    else{
        GLuint fbo,vao;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glUseProgram(scalar_depth_reader);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,combined);
        glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_DEPTH_COMPONENT);glUniform1i(glGetUniformLocation(scalar_depth_reader,"image"),0);
        glViewport(0,0,DW,DH);glScissor(0,0,DW,DH);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fbo);
    }
    uint32_t data[WORDS+32];for(unsigned i=0;i<WORDS+32;++i)data[i]=0xe7e7e7e7;read_integer(output,data+16);
    for(unsigned y=0;y<DH;++y)for(unsigned x=0;x<DW;++x)for(unsigned s=0;s<4;++s){unsigned want=s<samples?expected[(y*DW+x)*samples+s]:0;
        assert(data[16+(y*DW+x)*4+s]==want);++depth_checked;}
    for(unsigned i=0;i<16;++i)assert(data[i]==0xe7e7e7e7 && data[WORDS+16+i]==0xe7e7e7e7);
}
static void masked_stencil_draw(GLuint combined,unsigned samples,unsigned sample,unsigned value)
{
    GLenum target=samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D;GLuint fbo,vao;
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,target,combined,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,target,combined,0);const GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glUseProgram(constant_stencil);glViewport(0,0,W,H);glEnable(GL_SCISSOR_TEST);glScissor(3,2,11,5);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_STENCIL_TEST);glStencilMask(255);glStencilFunc(GL_ALWAYS,(GLint)value,255);glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
    if(samples>1){glEnable(GL_SAMPLE_MASK);glSampleMaski(0,1u<<sample);}else glDisable(GL_SAMPLE_MASK);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();assert(glGetError()==GL_NO_ERROR);++masked_draws;
    glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fbo);
}
static void run_samples(unsigned samples)
{
    GLenum format=samples==1?GL_R8UI:samples==2?GL_RG8UI:GL_RGBA8UI;
    GLuint input=integer_texture(W,H,format),output=integer_texture(DW,DH,format),depth_output=integer_texture(DW,DH,GL_RGBA32UI),combined;
    GLenum target=samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D;glGenTextures(1,&combined);glBindTexture(target,combined);
    if(samples>1)glTexStorage2DMultisample(target,samples,GL_DEPTH32F_STENCIL8,DW,DH,GL_TRUE);
    else{glTexStorage2D(target,1,GL_DEPTH32F_STENCIL8,DW,DH);glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
    assert(glGetError()==GL_NO_ERROR);GLint actual_samples=1;if(samples>1)glGetTexLevelParameteriv(target,0,GL_TEXTURE_SAMPLES,&actual_samples);fprintf(stderr,"test samples=%u actual_samples=%d\n",samples,actual_samples);uint32_t author_depth[DW*DH*4];
    for(unsigned y=0;y<DH;++y)for(unsigned x=0;x<DW;++x)for(unsigned s=0;s<samples;++s){float value=samples==1?(x<W && y<H?0.3125f:0.8125f):(float)((x+y*DW+s*19)%64)/64.0f;
        memcpy(&author_depth[(y*DW+x)*samples+s],&value,4);}
    if(samples>1){GLuint depth_input=integer_texture(DW,DH,samples==2?GL_RG32UI:GL_RGBA32UI);glBindTexture(GL_TEXTURE_2D,depth_input);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,DW,DH,channels(samples),GL_UNSIGNED_INT,author_depth);
        assert(!sweet_depth_multisample_copy_draw(depth_input,combined,DW,DH,samples,false,true,&depth_load));glDeleteTextures(1,&depth_input);
    }else{assert(!sweet_depth32_clear(combined,DW,DH,0.8125,&depth_clear));assert(!sweet_depth32_clear(combined,W,H,0.3125,&depth_clear));}
    const unsigned clear_values[]={0,3,17,41,90,127,128,171,255};
    for(unsigned phase=0;phase<sizeof(clear_values)/sizeof(clear_values[0]);++phase){
        current_phase=phase;
        uint8_t author[W*H*4],expected[DW*DH*4];memset(expected,0xa6,sizeof(expected));
        assert(!sweet_stencil8_clear_samples(combined,DW,DH,0xa6,samples,&stencil_clear));++clears;
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x)for(unsigned s=0;s<samples;++s){uint8_t value=(uint8_t)(x+W*y+s*59+phase*37);
            author[(y*W+x)*samples+s]=value;expected[(y*DW+x)*samples+s]=value;}
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,input);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,channels(samples),GL_UNSIGNED_BYTE,author);
        hostile_state();assert(!sweet_stencil_sample_copy_draw(input,combined,W,H,samples,false,&load));++copies;
        check_copy(combined,output,samples,expected,W,H);check_copy(combined,output,samples,expected,DW,DH);check_depth(combined,depth_output,samples,author_depth);
        unsigned sample=phase%samples;masked_stencil_draw(combined,samples,sample,90);
        for(unsigned y=2;y<7;++y)for(unsigned x=3;x<14;++x)expected[(y*DW+x)*samples+sample]=90;
        check_copy(combined,output,samples,expected,DW,DH);check_depth(combined,depth_output,samples,author_depth);
        hostile_state();assert(!sweet_stencil8_clear_samples(combined,W,H,clear_values[phase],samples,&stencil_clear));++clears;
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x)for(unsigned s=0;s<samples;++s)expected[(y*DW+x)*samples+s]=(uint8_t)clear_values[phase];
        check_copy(combined,output,samples,expected,W,H);check_copy(combined,output,samples,expected,DW,DH);check_depth(combined,depth_output,samples,author_depth);
    }
    assert(sweet_stencil_sample_copy_draw(input,combined,W,H,3,false,&load)==-EINVAL);
    assert(sweet_stencil_sample_copy_draw(input,input,W,H,samples,false,&load)==-EINVAL);
    assert(sweet_stencil_sample_copy_draw(input,combined,W,H,samples,false,NULL)==-EINVAL);
    glDeleteTextures(1,&input);glDeleteTextures(1,&output);glDeleteTextures(1,&depth_output);glDeleteTextures(1,&combined);
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLConfig config;EGLint count;
    assert(eglChooseConfig(display,attrs,&config,1,&count) && count);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    printf("Host renderer: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    scalar_depth_reader=program(scalar_depth_fragment);constant_stencil=program(constant_fragment);
    run_samples(1);run_samples(2);run_samples(4);assert(glGetError()==GL_NO_ERROR);
    glDeleteProgram(load);glDeleteProgram(store[0]);glDeleteProgram(store[1]);glDeleteProgram(depth_load);glDeleteProgram(depth_store);glDeleteProgram(depth_clear);
    glDeleteProgram(scalar_depth_reader);glDeleteProgram(constant_stencil);assert(stencil_clear!=0);glDeleteProgram(stencil_clear);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
    printf("Stencil sample copy PASS copies=%u checked_uint_words=%u depth_uint_words=%u GPU_bitplane_clears=%u masked_draws=%u bitpatterns=all256 samples=1,2,4 outer_pixels_and_CPU_guards=1 host_only=1\n",copies,checked,depth_checked,clears,masked_draws);return 0;
}
