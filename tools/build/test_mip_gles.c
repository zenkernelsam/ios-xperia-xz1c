/* Actual production mip shaders on host Mesa; this is not the FD540 importer. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/sweetgl/MipColourShader.h"
#include "../../gpu/sweetgl/XRColourShader.h"
#include "../../gpu/sweetgl/XRColourTables.h"
#include "ActualMipFixture.h"
static GLuint program(const char *fragment)
{
    const char *sources[]={sweet_mip_vertex,fragment};GLenum kinds[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};GLuint p=glCreateProgram();
    for(unsigned i=0;i<2;++i){GLuint s=glCreateShader(kinds[i]);glShaderSource(s,1,&sources[i],NULL);glCompileShader(s);GLint valid=0;glGetShaderiv(s,GL_COMPILE_STATUS,&valid);
        if(!valid){char error[8192];glGetShaderInfoLog(s,sizeof(error),NULL,error);fprintf(stderr,"mip shader failure %s\n",error);}assert(valid);glAttachShader(p,s);glDeleteShader(s);}
    glLinkProgram(p);GLint valid=0;glGetProgramiv(p,GL_LINK_STATUS,&valid);assert(valid);return p;
}
static GLuint image(unsigned w,unsigned h,GLenum format,GLenum type,const void *data)
{
    GLuint n;glGenTextures(1,&n);glBindTexture(GL_TEXTURE_2D,n);glTexImage2D(GL_TEXTURE_2D,0,format,w,h,0,format==GL_RGBA16UI?GL_RGBA_INTEGER:GL_RGBA,type,data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);return n;
}
static void draw(GLuint p,GLuint source,unsigned w,unsigned h)
{
    glUseProgram(p);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source);glUniform1i(glGetUniformLocation(p,"source_image"),0);
    glUniform2f(glGetUniformLocation(p,"destination_extent"),w,h);glViewport(0,0,w,h);glScissor(0,0,w,h);glDrawArrays(GL_TRIANGLES,0,3);glFinish();assert(glGetError()==GL_NO_ERROR);
}
static unsigned xr_code(float value)
{unsigned fixed=(unsigned)nearbyintf(value*8192),n=0;while(n+1<640 && sweet_xr_srgb_encode_first[n+1]<=fixed)++n;return 384+n;}
static GLuint decode(GLuint raw,unsigned w,unsigned h,GLuint fbo)
{
    char *source=malloc(strlen(sweet_xr_shader)+512);assert(source);
    sprintf(source,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform highp usampler2D source_image;uniform vec2 destination_extent;layout(location=0) out vec4 colour;void main(){colour=_smtl_xr_unpack(texelFetch(source_image,ivec2(gl_FragCoord.xy),0),553u);}",sweet_xr_shader);
    GLuint p=program(source);free(source);GLuint target=image(w,h,GL_RGBA32F,GL_FLOAT,NULL);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glUseProgram(p);glUniform1i(glGetUniformLocation(p,"_smtl_xr_lut"),16);draw(p,raw,w,h);glDeleteProgram(p);return target;
}
static void linear_colour(unsigned level,unsigned x,unsigned y,unsigned char rgba[4])
{unsigned scale=1u<<level;rgba[0]=(unsigned char)(16+4*x*scale+2*(scale-1));unsigned hscale=level>4?16:scale;
    rgba[1]=(unsigned char)(32+8*y*hscale+4*(hscale-1));rgba[2]=64;rgba[3]=255;}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(display!=EGL_NO_DISPLAY && eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    GLuint vao,fbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glEnable(GL_SCISSOR_TEST);glDisable(GL_DITHER);
    uint16_t tables[1280];for(unsigned i=0;i<640;++i){tables[2*i]=sweet_xr_srgb_decode_steps[i];tables[2*i+1]=sweet_xr_srgb_encode_first[i];}
    GLuint lut;glGenTextures(1,&lut);glActiveTexture(GL_TEXTURE16);glBindTexture(GL_TEXTURE_2D,lut);glTexImage2D(GL_TEXTURE_2D,0,GL_RG16UI,640,1,0,GL_RG_INTEGER,GL_UNSIGNED_SHORT,tables);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glActiveTexture(GL_TEXTURE0);
    unsigned total=0;
    for(unsigned format=0;format<3;++format){GLuint levels[6]={0};unsigned char rgba[2048];uint16_t raw[2048];
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x){unsigned at=(y*32+x)*4;if(format==0)linear_colour(0,x,y,rgba+at);
            else{unsigned white=(x+y)&1;for(unsigned c=0;c<3;++c)rgba[at+c]=white?255:0;rgba[at+3]=255;
                for(unsigned c=0;c<3;++c)raw[at+c]=(uint16_t)((white?894:384)<<6);raw[at+3]=894<<6;}}
        levels[0]=image(32,16,format==2?GL_RGBA16UI:format==1?GL_SRGB8_ALPHA8:GL_RGBA8,format==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_BYTE,format==2?(void *)raw:(void *)rgba);
        char *xr=malloc(strlen(sweet_xr_shader)+512);assert(xr);sprintf(xr,actual_xr_mip_template,sweet_xr_shader,553u);GLuint mip=program(format==2?xr:sweet_mip_fragment);free(xr);
        unsigned w=32,h=16;
        for(unsigned level=1;level<6;++level){unsigned previousW=w,previousH=h;w=w>1?w/2:1;h=h>1?h/2:1;
            GLuint source=format==2?decode(levels[level-1],previousW,previousH,fbo):levels[level-1];
            glBindTexture(GL_TEXTURE_2D,source);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            levels[level]=image(w,h,format==2?GL_RGBA16UI:format==1?GL_SRGB8_ALPHA8:GL_RGBA8,format==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_BYTE,NULL);
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,levels[level],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
            glUseProgram(mip);glUniform1i(glGetUniformLocation(mip,"_smtl_xr_lut"),16);draw(mip,source,w,h);
            unsigned char bytes[2048];uint16_t words[2048];glReadPixels(0,0,w,h,format==2?GL_RGBA_INTEGER:GL_RGBA,format==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_BYTE,format==2?(void *)words:(void *)bytes);assert(glGetError()==GL_NO_ERROR);
            for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned at=(y*w+x)*4;unsigned char wanted[4];linear_colour(level,x,y,wanted);
                for(unsigned c=0;c<4;++c){unsigned expected=format==2?(c==3?894<<6:xr_code(.5f)<<6):format==1?(c==3?255:188):wanted[c];unsigned actual=format==2?words[at+c]:bytes[at+c];
                    int error=abs((int)actual-(int)expected);unsigned tolerance=format==1 && c<3?1:0;
                    if(error>(int)tolerance)fprintf(stderr,"mip mismatch format=%u level=%u x=%u y=%u c=%u actual=%u expected=%u\n",format,level,x,y,c,actual,expected);assert(error<=(int)tolerance);++total;}}
            if(format==2)glDeleteTextures(1,&source);
        }
        // Build a GPU float mip chain and perform actual trilinear level1.5 sampling.
        GLuint chain;glGenTextures(1,&chain);glBindTexture(GL_TEXTURE_2D,chain);glTexStorage2D(GL_TEXTURE_2D,6,GL_RGBA32F,32,16);
        GLuint copy=program(sweet_mip_fragment);w=32;h=16;
        for(unsigned level=0;level<6;++level){GLuint source=format==2?decode(levels[level],w,h,fbo):levels[level];glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,chain,level);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);draw(copy,source,w,h);if(format==2)glDeleteTextures(1,&source);w=w>1?w/2:1;h=h>1?h/2:1;}
        glDeleteProgram(copy);GLuint output=image(32,16,GL_RGBA8,GL_UNSIGNED_BYTE,NULL);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
        const char *sample="#version 310 es\nprecision highp float;uniform highp sampler2D source_image;uniform vec2 destination_extent;layout(location=0) out vec4 colour;void main(){colour=textureLod(source_image,gl_FragCoord.xy/destination_extent,1.5);}";
        GLuint p=program(sample);glBindTexture(GL_TEXTURE_2D,chain);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        draw(p,chain,32,16);unsigned char outputBytes[2048];glReadPixels(0,0,32,16,GL_RGBA,GL_UNSIGNED_BYTE,outputBytes);assert(glGetError()==GL_NO_ERROR);
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x){unsigned char a[4],b[4];linear_colour(1,x/2,y/2,a);linear_colour(2,x/4,y/4,b);
            for(unsigned c=0;c<4;++c){unsigned expected=c==3?255:format==0?(a[c]+b[c])/2:format==1?(unsigned)floor(pow((188/255.0+.055)/1.055,2.4)*255+.5):
                    (unsigned)floor(sweet_xr_srgb_decode_steps[xr_code(.5f)-384]/8192.0*255+.5);assert(abs((int)outputBytes[(y*32+x)*4+c]-(int)expected)<=(format==1 && c<3?1:0));++total;}}
        glDeleteProgram(p);glDeleteProgram(mip);glDeleteTextures(6,levels);glDeleteTextures(1,&chain);glDeleteTextures(1,&output);
        fprintf(stderr,"actual production mip shaders format=%u levels=6 trilinear=1 numeric_tolerance=%u\n",format,format==1?1:0);
    }
    printf("actual production mip shader + GPU float chains: %u components, UNorm/XR exact, sRGB within1LSB, trilinear filtering PASS; host Mesa only\n",total);return 0;
}
