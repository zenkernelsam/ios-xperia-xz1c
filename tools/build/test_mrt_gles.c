/* Actual AIR-generated multiple outputs + production XR writeback shader.
 * Host textures do not exercise the FD540 original-page importer/padding. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/diagnostics/MRTProbeValues.h"
#include "../../gpu/sweetgl/MultiTargetShader.h"
#include "../../gpu/sweetgl/XRBlendShader.h"
#include "../../gpu/sweetgl/RenderUniformRules.h"
#include "../../gpu/sweetgl/MipColourShader.h"
#define mrt_vertex_source sweet_mip_vertex
typedef void (*IndexedEnable)(GLenum,GLuint);
typedef void (*IndexedBlend)(GLuint,GLenum,GLenum,GLenum,GLenum);
typedef void (*IndexedMask)(GLuint,GLboolean,GLboolean,GLboolean,GLboolean);
static IndexedEnable enablei,disablei;static IndexedBlend blend;static IndexedMask mask;
static char *read_file(const char *directory,const char *name)
{char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);assert(n>0 && n<1048576);rewind(f);char *s=calloc(n+1,1);assert(s && fread(s,1,n,f)==(size_t)n);fclose(f);return s;}
static GLuint shader(GLenum kind,const char *source)
{GLuint n=glCreateShader(kind);glShaderSource(n,1,&source,NULL);glCompileShader(n);GLint valid=0;glGetShaderiv(n,GL_COMPILE_STATUS,&valid);
    if(!valid){char error[8192];glGetShaderInfoLog(n,sizeof(error),NULL,error);fprintf(stderr,"MRT shader error %s\n",error);}assert(valid);return n;}
static GLuint program(const char *vertex,const char *fragment)
{GLuint p=glCreateProgram(),v=shader(GL_VERTEX_SHADER,vertex),f=shader(GL_FRAGMENT_SHADER,fragment);glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint valid=0;glGetProgramiv(p,GL_LINK_STATUS,&valid);
    if(!valid){char error[8192];glGetProgramInfoLog(p,sizeof(error),NULL,error);fprintf(stderr,"MRT link error %s\n",error);}assert(valid);glDeleteShader(v);glDeleteShader(f);return p;}
static bool uniform_contract(GLuint p,const SweetRenderPipelineConfig *config,uint32_t ordered_mask)
{
    GLint count=0;glGetProgramiv(p,GL_ACTIVE_UNIFORMS,&count);unsigned images=0;
    for(GLint i=0;i<count;++i){char name[512];GLint elements=0;GLenum type=0;glGetActiveUniform(p,i,sizeof(name),NULL,&elements,&type,name);
        unsigned expected=~0u;SweetRenderUniformRole role=sweet_render_uniform_role(name,type,elements,config,ordered_mask,&expected);
        if(role==SWEET_UNIFORM_REJECT)return false;
        if(role==SWEET_UNIFORM_XR_IMAGE){GLint actual=-1;glGetUniformiv(p,glGetUniformLocation(p,name),&actual);if(actual!=(GLint)expected)return false;++images;}
    }
    if(images!=2u*(unsigned)__builtin_popcount(ordered_mask))return false;
    assert(glGetError()==GL_NO_ERROR);fprintf(stderr,"actual production uniform scan XR_targets=%u owned_images=%u verified=1\n",(unsigned)__builtin_popcount(ordered_mask),images);return true;
}
static GLuint image(unsigned format,const void *data)
{GLuint n;glGenTextures(1,&n);glBindTexture(GL_TEXTURE_2D,n);GLenum internal=format>=552?(format<554?GL_RGBA16UI:GL_R32UI):format==115?GL_RGBA32F:GL_RGBA8;
    GLenum channels=format>=552?(format<554?GL_RGBA_INTEGER:GL_RED_INTEGER):GL_RGBA,type=format>=552?(format<554?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT):format==115?GL_FLOAT:GL_UNSIGNED_BYTE;
    if(format==115){glTexStorage2D(GL_TEXTURE_2D,1,internal,17,9);if(data)glTexSubImage2D(GL_TEXTURE_2D,0,0,0,17,9,channels,type,data);}
    else glTexImage2D(GL_TEXTURE_2D,0,internal,17,9,0,channels,type,data);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);return n;}
static void single(GLuint fbo,GLuint texture)
{glBindFramebuffer(GL_FRAMEBUFFER,fbo);for(unsigned i=1;i<8;++i)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_TEXTURE_2D,0,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);GLenum output=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&output);glReadBuffer(GL_COLOR_ATTACHMENT0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);}
static void no_blend(void)
{glDisable(GL_BLEND);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);for(unsigned i=0;i<8;++i){disablei(GL_BLEND,i);mask(i,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);}glDisable(GL_SCISSOR_TEST);}
static void fullscreen(GLuint p)
{glUseProgram(p);glViewport(0,0,17,9);glDrawArrays(GL_TRIANGLES,0,3);glFinish();GLenum err=glGetError();if(err)fprintf(stderr,"MRT draw error=%x\n",err);assert(err==GL_NO_ERROR);}
static GLuint decode(GLuint raw,unsigned format,GLuint fbo)
{char *fragment=malloc(strlen(sweet_xr_shader)+512);assert(fragment);sprintf(fragment,"#version 310 es\nprecision highp float;precision highp int;\n%s\nuniform highp usampler2D raw_image;layout(location=0) out vec4 colour;void main(){colour=_smtl_xr_unpack(texelFetch(raw_image,ivec2(gl_FragCoord.xy),0),%uu);}",sweet_xr_shader,format);
    GLuint p=program(mrt_vertex_source,fragment);free(fragment);GLuint target=image(115,NULL);single(fbo,target);no_blend();glUseProgram(p);glUniform1i(glGetUniformLocation(p,"_smtl_xr_lut"),16);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,raw);glUniform1i(glGetUniformLocation(p,"raw_image"),0);fullscreen(p);glDeleteProgram(p);return target;}
static void writeback(GLuint raw,GLuint linear,GLuint old,unsigned format,unsigned channels,GLuint fbo)
{char *fragment=sweet_xr_writeback_source(format);assert(fragment);GLuint p=program(mrt_vertex_source,fragment);free(fragment);single(fbo,raw);no_blend();glUseProgram(p);glUniform1i(glGetUniformLocation(p,"_smtl_xr_lut"),16);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,linear);glUniform1i(glGetUniformLocation(p,"float_image"),0);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,old);glUniform1i(glGetUniformLocation(p,"original_image"),1);
    glUniform1ui(glGetUniformLocation(p,"channel_mask"),channels);fullscreen(p);glDeleteProgram(p);glActiveTexture(GL_TEXTURE0);}
static void vertex_words(GLuint p,const float *coords,unsigned bytes)
{unsigned char page[16384]={0};memcpy(page,coords,bytes);GLuint t;glGenTextures(1,&t);glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32UI,4096,1,0,GL_RED_INTEGER,GL_UNSIGNED_INT,page);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glUseProgram(p);glUniform1i(glGetUniformLocation(p,"_smtl_vertex_buffer0"),8);glUniform2ui(glGetUniformLocation(p,"_smtl_vertex_buffer_info[0]"),0,bytes/4);glActiveTexture(GL_TEXTURE0);}
static void vertex_buffer(GLuint p)
{float coords[12]={-1,1,.5f,1,3,1,.5f,1,-1,-3,.5f,1};vertex_words(p,coords,sizeof(coords));}
static unsigned check(GLuint texture,unsigned format,unsigned phase,unsigned index,GLuint fbo)
{single(fbo,texture);unsigned mismatch=0,pixel=format>=552?(format<554?8:4):4;unsigned char bytes[17*9*8];
    if(format>=552){uint32_t words[17*9*4];glReadPixels(0,0,17,9,format<554?GL_RGBA_INTEGER:GL_RED_INTEGER,GL_UNSIGNED_INT,words);
        if(format<554){for(unsigned i=0;i<17*9*4;++i){uint16_t value=(uint16_t)words[i];memcpy(bytes+i*2,&value,2);}}else memcpy(bytes,words,17*9*4);
    }else glReadPixels(0,0,17,9,GL_RGBA,GL_UNSIGNED_BYTE,bytes);assert(glGetError()==GL_NO_ERROR);
    for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){unsigned char expected[8];if(format>=552)mrt_xr_pixel(format,phase,x,y,expected);else {mrt_bgra_pixel(phase,x,y,expected);unsigned char b=expected[0];expected[0]=expected[2];expected[2]=b;}
        for(unsigned c=0;c<pixel;++c)if(bytes[(y*17+x)*pixel+c]!=expected[c]){if(mismatch<8)fprintf(stderr,"MRT mismatch format=%u phase=%u index=%u x=%u y=%u c=%u actual=%02x expected=%02x\n",format,phase,index,x,y,c,bytes[(y*17+x)*pixel+c],expected[c]);++mismatch;}}
    return mismatch;
}
int main(int argc,char **argv)
{assert(argc==2);EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));EGLConfig cfg;EGLint count;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,attrs,&cfg,1,&count) && count);EGLint cv[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,cv);assert(ctx!=EGL_NO_CONTEXT && eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,ctx));
    enablei=(IndexedEnable)eglGetProcAddress("glEnableiEXT");disablei=(IndexedEnable)eglGetProcAddress("glDisableiEXT");blend=(IndexedBlend)eglGetProcAddress("glBlendFuncSeparateiEXT");mask=(IndexedMask)eglGetProcAddress("glColorMaskiEXT");assert(enablei && disablei && blend && mask);
    GLuint vao,fbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenFramebuffers(1,&fbo);glDisable(GL_DITHER);uint16_t data[1280];for(unsigned i=0;i<640;++i){data[2*i]=sweet_xr_srgb_decode_steps[i];data[2*i+1]=sweet_xr_srgb_encode_first[i];}
    GLuint lut;glGenTextures(1,&lut);glActiveTexture(GL_TEXTURE16);glBindTexture(GL_TEXTURE_2D,lut);glTexImage2D(GL_TEXTURE_2D,0,GL_RG16UI,640,1,0,GL_RG_INTEGER,GL_UNSIGNED_SHORT,data);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glActiveTexture(GL_TEXTURE0);
    char *vertex=read_file(argv[1],"mrt_position.vert"),*color=read_file(argv[1],"mrt_colour.frag"),*changed=read_file(argv[1],"mrt_changed.frag");
    const char *clear="#version 310 es\nprecision highp float;uniform vec4 c;layout(location=0) out vec4 colour;void main(){colour=c;}";GLuint clearProgram=program(mrt_vertex_source,clear);
    for(unsigned format=552;format<=555;++format){unsigned pixel=format<554?8:4;unsigned char pixels[17*9*8];memset(pixels,0xa5,sizeof(pixels));GLuint raw=image(format,pixels),target=image(80,NULL);
        SweetRenderPipelineConfig config={0};config.vertex_buffers=1;config.color_mask=3;config.format=format;config.blend_enabled=1;config.source_rgb=4;config.destination_rgb=5;config.source_alpha=0;config.destination_alpha=1;config.write_mask=15;
        char *ordered_vertex=sweet_xr_ordered_vertex(vertex),*ordered_alpha=sweet_xr_ordered_fragment(color,&config,1);assert(ordered_vertex && ordered_alpha);
        GLuint alpha=program(ordered_vertex,ordered_alpha);assert(uniform_contract(alpha,&config,1));
        if(format==552){char *bad=strstr(ordered_alpha,"binding=8)");assert(bad);bad[8]='7';GLuint wrong=program(ordered_vertex,ordered_alpha);assert(!uniform_contract(wrong,&config,1));glDeleteProgram(wrong);bad[8]='8';
            fprintf(stderr,"actual production uniform scan incorrect image binding rejected=1\n");}
        free(ordered_alpha);config.destination_rgb=1;config.write_mask=10;
        char *ordered_masked=sweet_xr_ordered_fragment(changed,&config,1);assert(ordered_masked);GLuint masked=program(ordered_vertex,ordered_masked);assert(uniform_contract(masked,&config,1));free(ordered_masked);free(ordered_vertex);vertex_buffer(alpha);vertex_buffer(masked);
        glUseProgram(alpha);glUniform1i(glGetUniformLocation(alpha,"_smtl_xr_lut"),16);glUseProgram(masked);glUniform1i(glGetUniformLocation(masked,"_smtl_xr_lut"),16);
        for(unsigned phase=0;phase<3;++phase){if(phase==1){unsigned char seeded[16384];memset(seeded,0xa5,sizeof(seeded));mrt_seed_old_codes(seeded,format);
                for(unsigned y=0;y<9;++y)memcpy(pixels+y*17*pixel,seeded+64+y*(format<554?192:128),17*pixel);glBindTexture(GL_TEXTURE_2D,raw);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,17,9,format<554?GL_RGBA_INTEGER:GL_RED_INTEGER,format<554?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,pixels);}
            GLuint old=image(format,NULL);single(fbo,raw);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,old);glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,17,9);glActiveTexture(GL_TEXTURE0);assert(glGetError()==GL_NO_ERROR);
            GLuint linear=decode(raw,format,fbo);
            if(!phase){single(fbo,linear);no_blend();glUseProgram(clearProgram);glUniform4f(glGetUniformLocation(clearProgram,"c"),0,0,1,1);fullscreen(clearProgram);
                single(fbo,target);glUseProgram(clearProgram);glUniform4f(glGetUniformLocation(clearProgram,"c"),1,0,0,1);fullscreen(clearProgram);}
            else {glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,0,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,target,0);
                GLenum outputs[]={GL_NONE,GL_COLOR_ATTACHMENT1};glDrawBuffers(2,outputs);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
                no_blend();glBindImageTexture(0,linear,0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8,linear,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);glMemoryBarrier(GL_ALL_BARRIER_BITS);
                if(phase==2){enablei(GL_BLEND,1);blend(1,GL_ONE,GL_ZERO,GL_ONE,GL_ZERO);}mask(1,GL_FALSE,GL_TRUE,phase==2?GL_TRUE:GL_FALSE,GL_FALSE);
                glEnable(GL_SCISSOR_TEST);glScissor(3,2,11,5);fullscreen(phase==1?alpha:masked);glMemoryBarrier(GL_ALL_BARRIER_BITS);
                glBindImageTexture(0,0,0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8,0,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);}
            writeback(raw,linear,old,format,phase==2?10:15,fbo);assert(!check(raw,format,phase,0,fbo) && !check(target,80,phase,1,fbo));glDeleteTextures(1,&linear);glDeleteTextures(1,&old);
            fprintf(stderr,"actual MRT host format=%u phase=%u zero_mismatch=1 original_lowbits_preserved=1\n",format,phase);
        }glDeleteTextures(1,&raw);glDeleteTextures(1,&target);glDeleteProgram(alpha);glDeleteProgram(masked);
    }
    char *eightSource=read_file(argv[1],"mrt_eight.frag");GLuint eight=program(vertex,eightSource);free(eightSource);vertex_buffer(eight);GLuint targets[8];glBindFramebuffer(GL_FRAMEBUFFER,fbo);GLenum outputs[8];
    for(unsigned i=0;i<8;++i){targets[i]=image(80,NULL);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_TEXTURE_2D,targets[i],0);outputs[i]=GL_COLOR_ATTACHMENT0+i;}glDrawBuffers(8,outputs);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);no_blend();fullscreen(eight);
    for(unsigned i=0;i<8;++i){glReadBuffer(GL_COLOR_ATTACHMENT0+i);unsigned char pixels[612];glReadPixels(0,0,17,9,GL_RGBA,GL_UNSIGNED_BYTE,pixels);assert(glGetError()==GL_NO_ERROR);unsigned r=(unsigned)floor(i*.125*255+.5);
        for(unsigned n=0;n<153;++n)assert(pixels[n*4]==r && pixels[n*4+1]==64 && pixels[n*4+2]==128 && pixels[n*4+3]==255);}
    char *orderVertex=read_file(argv[1],"mrt_order_position.vert"),*orderColor=read_file(argv[1],"mrt_order_colour.frag");float coords[36];
    const float triangle[12]={-1,1,.5f,1,3,1,.5f,1,-1,-3,.5f,1};for(unsigned i=0;i<3;++i)memcpy(coords+i*12,triangle,sizeof(triangle));
    for(unsigned format=552;format<=555;++format){unsigned pixel=format<554?8:4;unsigned char seeded[16384],pixels[17*9*8];memset(seeded,0xa5,sizeof(seeded));mrt_seed_old_codes(seeded,format);
        for(unsigned y=0;y<9;++y)memcpy(pixels+y*17*pixel,seeded+64+y*(format<554?192:128),17*pixel);
        GLuint raw=image(format,pixels),old=image(format,pixels),linear=decode(raw,format,fbo),empty;glGenFramebuffers(1,&empty);glBindFramebuffer(GL_FRAMEBUFFER,empty);
        glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_WIDTH,17);glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_HEIGHT,9);glFramebufferParameteri(GL_FRAMEBUFFER,GL_FRAMEBUFFER_DEFAULT_SAMPLES,0);
        GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
        SweetRenderPipelineConfig config={0};config.vertex_buffers=1;config.format=format;config.blend_enabled=1;config.source_rgb=4;config.destination_rgb=5;config.source_alpha=0;config.destination_alpha=1;config.write_mask=14;
        char *vs=sweet_xr_ordered_vertex(orderVertex),*fs=sweet_xr_ordered_fragment(orderColor,&config,1);assert(vs && fs);GLuint p=program(vs,fs);assert(uniform_contract(p,&config,1));free(vs);free(fs);vertex_words(p,coords,sizeof(coords));glUseProgram(p);glUniform1i(glGetUniformLocation(p,"_smtl_xr_lut"),16);
        no_blend();glBindImageTexture(0,linear,0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8,linear,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glViewport(0,0,17,9);glEnable(GL_SCISSOR_TEST);glScissor(3,2,11,5);
        for(unsigned instance=0;instance<2;++instance){glUniform1i(glGetUniformLocation(p,"_smtl_instance_base"),instance);
            for(unsigned index=0;index<sweet_ordered_primitive_count(3,6);++index){SweetOrderedPrimitive primitive=sweet_ordered_primitive(3,3,index);
                glDrawArraysInstanced(GL_TRIANGLES,primitive.first,primitive.count,1);glMemoryBarrier(GL_ALL_BARRIER_BITS);assert(glGetError()==GL_NO_ERROR);}}
        glFinish();glBindImageTexture(0,0,0,GL_FALSE,0,GL_READ_ONLY,GL_RGBA32F);glBindImageTexture(8,0,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA32F);writeback(raw,linear,old,format,14,fbo);single(fbo,raw);
        uint32_t words[17*9*4];glReadPixels(0,0,17,9,format<554?GL_RGBA_INTEGER:GL_RED_INTEGER,GL_UNSIGNED_INT,words);assert(glGetError()==GL_NO_ERROR);
        if(format<554){for(unsigned i=0;i<17*9*4;++i){uint16_t value=(uint16_t)words[i];memcpy(pixels+i*2,&value,2);}}else memcpy(pixels,words,17*9*4);
        unsigned mismatch=0;for(unsigned y=0;y<9;++y)for(unsigned x=0;x<17;++x){unsigned char wanted[8];mrt_order_pixel(format,x,y,wanted);mismatch+=memcmp(pixels+(y*17+x)*pixel,wanted,pixel)!=0;}
        if(mismatch)fprintf(stderr,"ordered format=%u mismatch=%u\n",format,mismatch);assert(!mismatch);
        fprintf(stderr,"actual AIR XR ordered format=%u four_overlapping_primitives=1 instance_ID=1 first3=1 negative_and_above_one=1 zero_mismatch=1\n",format);
        glDeleteProgram(p);glDeleteFramebuffers(1,&empty);glDeleteTextures(1,&raw);glDeleteTextures(1,&old);glDeleteTextures(1,&linear);
    }
    free(vertex);free(color);free(changed);free(orderVertex);free(orderColor);
    puts("actual AIR simultaneous MRT, per-target independent blend/masks, fourXR FP32 image blending and writeback old-code bits, eight outputs and ordered overlapping HDR/instances PASS (host Mesa; device original pages still required)");return 0;
}
