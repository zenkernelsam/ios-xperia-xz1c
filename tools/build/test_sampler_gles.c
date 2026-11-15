/* SPDX-License-Identifier: BSD-2-Clause
 * Real GLES sampling through the same sampler parser/state core as production.
 * Host pixels do not replace FD540 original-page/fence acceptance.
 */
#include "../../gpu/sweetgl/RenderSamplerRules.h"
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"sampler check failed line=%d: %s GL=%#x EGL=%#x\n",__LINE__,#x,glGetError(),eglGetError());exit(1);}}while(0)
static GLuint shader(GLenum kind,const char *source)
{
    GLuint name=glCreateShader(kind);GLint ok=0;glShaderSource(name,1,&source,NULL);glCompileShader(name);glGetShaderiv(name,GL_COMPILE_STATUS,&ok);
    if(!ok){char error[2048];glGetShaderInfoLog(name,sizeof(error),NULL,error);fprintf(stderr,"%s\n",error);}CHECK(ok);return name;
}
static bool extension(const char *wanted)
{
    GLint count=0;glGetIntegerv(GL_NUM_EXTENSIONS,&count);for(GLint i=0;i<count;++i){const char *s=(const char *)glGetStringi(GL_EXTENSIONS,(GLuint)i);
        if(s && !strcmp(s,wanted))return true;}return false;
}
static unsigned checks=0,pixels=0;
static void draw(const char *name,float x,float y,float lod,unsigned ordinal,const unsigned char expected[4],
    GLuint texture,bool border,bool anisotropic,float maximum)
{
    SweetRenderSamplerUse use={0};CHECK(sweet_render_sampler_parse(name,&use));SweetRenderSamplerState state={0};
    SweetShaderSamplerBinding ordinary={use.sampler,use.stage,0,0,0,0,0,1,0,16};
    CHECK(sweet_render_sampler_resolve(&use,use.literal?NULL:&ordinary,use.literal?0:1,&state)==0);
    char fragment[1024];snprintf(fragment,sizeof(fragment),"#version 310 es\nprecision highp float;uniform sampler2D %s;uniform vec2 coordinate;uniform float level;layout(location=0) out vec4 colour;void main(){colour=%s(%s,coordinate%s);}",
        name,use.explicit_level?"textureLod":"texture",name,use.explicit_level?",level":"");
    GLuint vs=shader(GL_VERTEX_SHADER,"#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}");
    GLuint fs=shader(GL_FRAGMENT_SHADER,fragment),program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked);glDeleteShader(vs);glDeleteShader(fs);
    GLint active=0;glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&active);bool seen=false;
    for(GLint i=0;i<active;++i){char actual[512];GLint count=0;GLenum type=0;glGetActiveUniform(program,(GLuint)i,sizeof(actual),NULL,&count,&type,actual);
        if(type==GL_SAMPLER_2D){SweetRenderSamplerUse parsed;CHECK(count==1 && !strcmp(name,actual) && sweet_render_sampler_parse(actual,&parsed));seen=true;}}
    CHECK(seen);GLint available=0;glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,&available);unsigned unit=0;
    CHECK(sweet_render_sampler_unit(ordinal,(unsigned)available,&unit));CHECK(unit<8 || unit>16);
    glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,texture);
    CHECK(sweet_render_sampler_parameters(&state,border,anisotropic,maximum)==0);
    GLfloat minimum=0,max_lod=0;glGetTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_MIN_LOD,&minimum);glGetTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_MAX_LOD,&max_lod);
    CHECK(minimum==state.binding.minimum_lod && max_lod==state.binding.maximum_lod);
    glUseProgram(program);glUniform1i(glGetUniformLocation(program,name),(GLint)unit);glUniform2f(glGetUniformLocation(program,"coordinate"),x,y);
    GLint at=glGetUniformLocation(program,"level");if(at>=0)glUniform1f(at,lod);
    GLint bound=-1;glGetUniformiv(program,glGetUniformLocation(program,name),&bound);CHECK(bound==(GLint)unit);
    glViewport(0,0,1,1);glDrawArrays(GL_TRIANGLES,0,3);glFinish();unsigned char result[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,result);CHECK(glGetError()==GL_NO_ERROR);
    if(memcmp(result,expected,4))fprintf(stderr,"%s got=%u,%u,%u,%u expected=%u,%u,%u,%u\n",name,result[0],result[1],result[2],result[3],expected[0],expected[1],expected[2],expected[3]);
    CHECK(!memcmp(result,expected,4));++pixels;glDeleteProgram(program);++checks;
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(display!=EGL_NO_DISPLAY);CHECK(eglInitialize(display,NULL,NULL));CHECK(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint wanted[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig config;EGLint count=0;CHECK(eglChooseConfig(display,wanted,&config,1,&count) && count==1);
    EGLint size[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,size);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);CHECK(context!=EGL_NO_CONTEXT && surface!=EGL_NO_SURFACE);
    CHECK(eglMakeCurrent(display,surface,surface,context));fprintf(stderr,"actual host renderer=%s\n",glGetString(GL_RENDERER));
    bool border=extension("GL_EXT_texture_border_clamp") || extension("GL_OES_texture_border_clamp"),anisotropic=extension("GL_EXT_texture_filter_anisotropic");
    GLfloat maximum=1;if(anisotropic)glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maximum);
    GLuint texture=0,target=0,fbo=0,vao=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexStorage2D(GL_TEXTURE_2D,2,GL_RGBA8,2,2);
    const unsigned char colours[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255},blue[]={0,0,255,255};
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,colours);glTexSubImage2D(GL_TEXTURE_2D,1,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,blue);
    glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,1,1);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_BLEND);glDisable(GL_DITHER);
    const unsigned char red[]={255,0,0,255},green[]={0,255,0,255},white[]={255,255,255,255},zero[]={0,0,0,0};
    draw("_qsample0_8_c_3_0_4095_0_1_0_1",.25f,.25f,0,0,red,texture,border,anisotropic,maximum);
    draw("_qsample0_8_c_0_0_4095_0_1_0_1",1.25f,.25f,0,0,green,texture,border,anisotropic,maximum);
    draw("_qsample0_9_c_0_0_4095_146_1_0_1",1.25f,.25f,0,0,red,texture,border,anisotropic,maximum);
    draw("_qsample0_10_c_0_0_4095_219_1_0_1",1.25f,.25f,0,0,green,texture,border,anisotropic,maximum);
    draw("_qsample0_11_c_8_0_4095_0_1_0_1_level",.25f,.25f,1,0,blue,texture,border,anisotropic,maximum);
    draw("_qsample0_12_c_8_0_0_0_1_0_1_level",.25f,.25f,1,0,red,texture,border,anisotropic,maximum);
    draw("_qsample0_13_c_8_256_256_0_1_0_1",.25f,.25f,0,0,blue,texture,border,anisotropic,maximum);
    draw("_qsample7_23_c_0_0_4095_0_1_0_1",.25f,.25f,0,8,red,texture,border,anisotropic,maximum);
    draw("_qsample0_3",.25f,.25f,0,0,red,texture,border,anisotropic,maximum);
    if(border){draw("_qsample0_14_c_0_0_4095_365_1_2_1",1.25f,.25f,0,0,white,texture,border,anisotropic,maximum);
        draw("_qsample0_15_c_0_0_4095_292_1_0_1",1.25f,.25f,0,0,zero,texture,border,anisotropic,maximum);}
    SweetRenderSamplerUse use;const char *bad[]={"_qsample0_8","_qsample0_7_c_0_0_4095_0_1_0_1","_qsample8_8_c_0_0_4095_0_1_0_1",
        "_qsample0_24_c_0_0_4095_0_1_0_1","_qsample0_8_c_16_0_4095_0_1_0_1","_qsample0_8_c_0_0_4096_0_1_0_1",
        "_qsample0_8_c_0_0_4095_0_0_0_1","_qsample0_8_c_0_0_4095_0_1_3_1","_qsample0_8_c_0_0_4095_0_1_0_2",
        "_qsample0_8_c_0_0_4095_0_1_0_1_level_extra","_qsample-1_0","_qsample+1_0","_qsample4294967296_0","_qsample0_4294967296"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i){CHECK(!sweet_render_sampler_parse(bad[i],&use));++checks;}
    CHECK(sweet_render_sampler_parse("_qvertex_sample7_23_c_3_0_4095_0_1_0_1",&use) && use.stage==1);
    CHECK(sweet_render_sampler_parse("_qsample0_8_c_0_0_4095_0_1_0_0",&use));SweetRenderSamplerState state;
    CHECK(sweet_render_sampler_resolve(&use,NULL,0,&state)==-ENOTSUP);
    CHECK(sweet_render_sampler_parse("_qsample0_0",&use));SweetShaderSamplerBinding duplicate[2]={{0,2,0,0,0,0,0,1,0,16},{0,2,0,0,0,0,0,1,0,16}};
    CHECK(sweet_render_sampler_resolve(&use,duplicate,2,&state)==-EINVAL);CHECK(sweet_render_sampler_resolve(&use,NULL,0,&state)==-EINVAL);
    unsigned unit=0;CHECK(!sweet_render_sampler_unit(23,192,&unit));CHECK(!sweet_render_sampler_unit(8,17,&unit));
    glDeleteVertexArrays(1,&vao);glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&texture);glDeleteTextures(1,&target);
    CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    printf("sampler host PASS cases=%u exact_pixels=%u border_cap=%u anisotropic_cap=%u literal_without_external_state=1 integer_overflow_refused=1 reserved_units_preserved=1\n",checks,pixels,border,anisotropic);return 0;
}
