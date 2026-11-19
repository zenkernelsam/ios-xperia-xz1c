/* Production rewritten matrix shaders on actual host Mesa; no device or cache writes. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../gpu/sweetgl/CAMatrixProbeValues.h"
#include "../../gpu/sweetgl/CAMatrixShaders.h"
#include "ProductionCAMatrix.h"

static GLuint compile(GLenum stage,const char *source)
{
    GLuint shader=glCreateShader(stage);glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[2048];glGetShaderInfoLog(shader,sizeof(log),NULL,log);fprintf(stderr,"%s\n%s\n",log,source);}
    assert(ok);return shader;
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
    EGLConfig config;EGLint count=0;assert(eglChooseConfig(display,attributes,&config,1,&count) && count==1);
    EGLint size[]={EGL_WIDTH,SWEET_CA_WIDTH,EGL_HEIGHT,SWEET_CA_HEIGHT,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,size);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    assert(surface!=EGL_NO_SURFACE && context!=EGL_NO_CONTEXT && eglMakeCurrent(display,surface,surface,context));
    uint8_t source[SWEET_CA_BYTES],target[SWEET_CA_BYTES],readback[SWEET_CA_BYTES+32];sweet_ca_input(source,target);
    GLuint textures[3]={0},fbo=0;glGenTextures(3,textures);glGenFramebuffers(1,&fbo);
    for(unsigned i=0;i<3;++i){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SWEET_CA_WIDTH,SWEET_CA_HEIGHT,0,GL_RGBA,GL_UNSIGNED_BYTE,i==0?source:target);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[2],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    const char *vs="attribute vec2 position;varying highp vec2 uv;void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}";
    const char *fs[2]={
        "precision highp float;uniform sampler2D image;varying highp vec2 uv;void main(){lowp vec4 s0=texture2D(image,uv);gl_FragColor=sgl_cm4(s0);}",
        "#extension GL_EXT_shader_framebuffer_fetch : require\nprecision highp float;uniform sampler2D image;varying highp vec2 uv;void main(){lowp vec4 s0=texture2D(image,uv);gl_FragColor=sgl_vcm(s0);}"};
    const GLfloat positions[]={-1,-1,1,-1,-1,1,1,1};
    glViewport(0,0,SWEET_CA_WIDTH,SWEET_CA_HEIGHT);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,positions);glEnableVertexAttribArray(0);
    glDisable(GL_BLEND);glDisable(GL_DITHER);
    for(unsigned phase=0;phase<SWEET_CA_PHASES;++phase){
        char *rewritten=texfn_rewrite(strdup(fs[phase>=3]));if(phase>=3)rewritten=fbfetch_rewrite(rewritten);
        GLuint vertex=compile(GL_VERTEX_SHADER,vs),fragment=compile(GL_FRAGMENT_SHADER,rewritten),program=glCreateProgram();free(rewritten);
        glAttachShader(program,vertex);glAttachShader(program,fragment);glBindAttribLocation(program,0,"position");glLinkProgram(program);
        GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);
        glUniform1i(glGetUniformLocation(program,"image"),0);glUniform1i(glGetUniformLocation(program,"sgl_fb"),1);
        glUniform2f(glGetUniformLocation(program,"sgl_fb_scale"),1.0f/SWEET_CA_WIDTH,1.0f/SWEET_CA_HEIGHT);
        float matrix[20];sweet_ca_matrix(phase,matrix);glUniform4fv(glGetUniformLocation(program,"color_matrix"),5,matrix);
        memset(readback,0xa5,sizeof(readback));glDrawArrays(GL_TRIANGLE_STRIP,0,4);glFinish();glReadPixels(0,0,SWEET_CA_WIDTH,SWEET_CA_HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,readback+16);
        unsigned maximum=0;
        for(unsigned byte=0;byte<SWEET_CA_BYTES;++byte){unsigned expected=sweet_ca_reference(phase,byte/4,byte%4),actual=readback[16+byte];
            unsigned difference=expected>actual?expected-actual:actual-expected;if(difference>maximum)maximum=difference;assert(difference<=SWEET_CA_QUANTIZATION);}
        for(unsigned i=0;i<16;++i)assert(readback[i]==0xa5 && readback[16+SWEET_CA_BYTES+i]==0xa5);
        assert(glGetError()==GL_NO_ERROR);printf("CA matrix host phase=%u checked=256 max_diff=%u guard=32 PASS\n",phase,maximum);
        glUseProgram(0);glDeleteProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);
    }
    glDeleteTextures(3,textures);glDeleteFramebuffers(1,&fbo);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    puts("CA matrix production rewrite GLES PASS: six phases, original bytes/guards, <=1LSB");return 0;
}
