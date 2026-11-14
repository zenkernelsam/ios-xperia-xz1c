/* Actual XR GLSL encoding/decoding on host GLES; independent code arithmetic. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/sweetgl/XRColourShader.h"
#include "../../gpu/sweetgl/XRColourTables.h"
static uint32_t round_code(float value)
{double exact=(double)fabsf(value)*510.0;double floor_=floor(exact),fraction=exact-floor_;return (uint32_t)floor_+(fraction>0.5 || (fraction==0.5 && ((uint32_t)floor_&1)));}
static uint32_t code(float value,bool alpha,bool srgb)
{
    if(isnan(value))value=0;if(alpha)value=fminf(fmaxf(value,0),1);uint32_t amount;
    if(srgb && !alpha){float fixed=fminf(fabsf(value),2)*8192;unsigned index=(unsigned)nearbyintf(fixed),n=0;while(n<640 && sweet_xr_srgb_encode_first[n]<=index)++n;amount=n?n-1:0;}
    else amount=round_code(fminf(fabsf(value),1.5f));
    return value<0?384-(amount<384?amount:384):(384+amount<1023?384+amount:1023);
}
static GLuint shaderProgram(const char *fragment)
{
    const char *vertex="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
    const char *sources[2]={vertex,fragment};GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};GLuint result=glCreateProgram();
    for(unsigned i=0;i<2;++i){GLuint shader=glCreateShader(kinds[i]);glShaderSource(shader,1,&sources[i],NULL);glCompileShader(shader);GLint ok;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char error[8192];glGetShaderInfoLog(shader,sizeof(error),NULL,error);fprintf(stderr,"XR compile %s\n",error);}assert(ok);glAttachShader(result,shader);glDeleteShader(shader);}
    glLinkProgram(result);GLint ok;glGetProgramiv(result,GL_LINK_STATUS,&ok);if(!ok){char error[8192];glGetProgramInfoLog(result,sizeof(error),NULL,error);fprintf(stderr,"XR link %s\n",error);}assert(ok);return result;
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(display!=EGL_NO_DISPLAY && eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);GLuint framebuffer;glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    uint16_t table[1280];for(unsigned i=0;i<640;++i){table[i*2]=sweet_xr_srgb_decode_steps[i];table[i*2+1]=sweet_xr_srgb_encode_first[i];}
    GLuint lut;glGenTextures(1,&lut);glActiveTexture(GL_TEXTURE16);glBindTexture(GL_TEXTURE_2D,lut);glTexImage2D(GL_TEXTURE_2D,0,GL_RG16UI,640,1,0,GL_RG_INTEGER,GL_UNSIGNED_SHORT,table);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    float input[1024*4];for(unsigned i=0;i<1024;++i){input[i*4]=(float)((double)((int)i-384)/510);input[i*4+1]=(float)((double)((int)((i*13)&1023)-384)/510);input[i*4+2]=((int)(i%19)-9)*0.125f;input[i*4+3]=(i%11)*0.125f;}
    input[1]=0.25f;input[2]=-0.25f;
    for(uint32_t format=552;format<=555;++format){char *fragment=calloc(strlen(sweet_xr_shader)+512,1);assert(fragment);
        sprintf(fragment,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform highp sampler2D input_image;layout(location=0) out highp uvec4 raw_out;void main(){raw_out=_smtl_xr_pack(texelFetch(input_image,ivec2(gl_FragCoord.xy),0),%uu);}",sweet_xr_shader,format);
        GLuint program=shaderProgram(fragment);free(fragment);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"_smtl_xr_lut"),16);
        GLuint source,target;glGenTextures(1,&source);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,1024,1,0,GL_RGBA,GL_FLOAT,input);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glUniform1i(glGetUniformLocation(program,"input_image"),0);
        glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);bool bgra=format<554;glTexImage2D(GL_TEXTURE_2D,0,bgra?GL_RGBA16UI:GL_R32UI,1024,1,0,bgra?GL_RGBA_INTEGER:GL_RED_INTEGER,bgra?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);GLenum complete=glCheckFramebufferStatus(GL_FRAMEBUFFER),err=glGetError();
        fprintf(stderr,"XR fbo format=%u status=%x error=%x\n",format,complete,err);assert(complete==GL_FRAMEBUFFER_COMPLETE && err==GL_NO_ERROR);glBindTexture(GL_TEXTURE_2D,source);
        glViewport(0,0,1024,1);glDrawArrays(GL_TRIANGLES,0,3);glFinish();assert(glGetError()==GL_NO_ERROR);uint32_t output[4096];
        glReadPixels(0,0,1024,1,bgra?GL_RGBA_INTEGER:GL_RED_INTEGER,GL_UNSIGNED_INT,output);assert(glGetError()==GL_NO_ERROR);unsigned mismatches=0;
        for(unsigned i=0;i<1024;++i){uint32_t r=code(input[i*4],false,format&1),g=code(input[i*4+1],false,format&1),b=code(input[i*4+2],false,format&1),a=code(input[i*4+3],true,false);
            uint32_t expected[4]={b<<6,g<<6,r<<6,a<<6};if(!bgra)expected[0]=0xc0000000|(r<<20)|(g<<10)|b;
            for(unsigned c=0;c<(bgra?4:1);++c)if(output[i*(bgra?4:1)+c]!=expected[c]){if(mismatches<8)fprintf(stderr,"XR mismatch format=%u x=%u c=%u actual=%u expected=%u\n",format,i,c,output[i*(bgra?4:1)+c],expected[c]);++mismatches;}}
        fprintf(stderr,"actual XR format=%u pixels=1024 mismatches=%u\n",format,mismatches);assert(!mismatches);glDeleteTextures(1,&source);glDeleteTextures(1,&target);glDeleteProgram(program);
    }
    puts("actual XR shader: 4 formats, exact float product/nearest-even codes, signed linear/sRGB table, raw BGRA/XR packing PASS (host Mesa)");return 0;
}
