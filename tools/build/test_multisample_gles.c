/* Real GL sample-mask draws, individual sample readback and production resolve.
 * No native Metal or FD540 original external-page importer is invoked here. */
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
#include "../../gpu/sweetgl/MultisampleResolveGLES.h"
enum {W=17,H=9,N=W*H};
static GLuint draw_program,sample_program,depth_program,stencil_program,single_program;
static GLuint read_fbo,read_color;
static unsigned draws,checked_samples,checked_bytes,resolves;static bool actual_AIR;
static const uint8_t colors[4][4]={{32,64,96,255},{96,128,160,255},{160,192,224,255},{224,64,32,255}};
static const float sample_depths[4]={0.5f,0.125f,0.75f,0.25f};
static GLuint shader(GLenum kind,const char *source)
{GLuint s=glCreateShader(kind);GLint good=0;glShaderSource(s,1,&source,NULL);glCompileShader(s);glGetShaderiv(s,GL_COMPILE_STATUS,&good);if(!good){char text[2000];glGetShaderInfoLog(s,sizeof(text),NULL,text);fprintf(stderr,"%s\n",text);}assert(good);return s;}
static GLuint program(const char *source)
{
    const char *vertex="#version 310 es\nuniform float z;void main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,z*2.0-1.0,1);}";
    GLuint v=shader(GL_VERTEX_SHADER,vertex),f=shader(GL_FRAGMENT_SHADER,source),p=glCreateProgram();GLint good=0;glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&good);assert(good);glDeleteShader(v);glDeleteShader(f);return p;
}
static char *shader_file(const char *directory,const char *name)
{
    char path[2048];assert(snprintf(path,sizeof(path),"%s/%s",directory,name)<(int)sizeof(path));FILE *file=fopen(path,"rb");assert(file);
    fseek(file,0,SEEK_END);long count=ftell(file);assert(count>0 && count<1048576);rewind(file);char *text=calloc(count+1,1);assert(text && fread(text,1,count,file)==(size_t)count);fclose(file);return text;
}
static GLuint generated_program(const char *directory)
{
    char *vertex=shader_file(directory,"multisample_position.vert"),*fragment=shader_file(directory,"multisample_color.frag");
    GLuint v=shader(GL_VERTEX_SHADER,vertex),f=shader(GL_FRAGMENT_SHADER,fragment),p=glCreateProgram();GLint good=0;glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glGetProgramiv(p,GL_LINK_STATUS,&good);assert(good);
    glDeleteShader(v);glDeleteShader(f);free(vertex);free(fragment);return p;
}
static void set_draw_input(unsigned sample)
{
    float z=sample_depths[sample],rgba[4];for(unsigned i=0;i<4;++i)rgba[i]=colors[sample][i]/255.0f;
    if(!actual_AIR){glUniform1f(glGetUniformLocation(draw_program,"z"),z);glUniform4fv(glGetUniformLocation(draw_program,"color"),1,rgba);return;}
    const float positions[12]={-1,1,z,1,3,1,z,1,-1,-3,z,1};uint32_t words[4096]={0};memcpy(words,positions,sizeof(positions));
    GLuint image=0,buffer=0;glGenTextures(1,&image);glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,image);glTexStorage2D(GL_TEXTURE_2D,1,GL_R32UI,4096,1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4096,1,GL_RED_INTEGER,GL_UNSIGNED_INT,words);
    glUniform1i(glGetUniformLocation(draw_program,"_smtl_vertex_buffer0"),8);glUniform2ui(glGetUniformLocation(draw_program,"_smtl_vertex_buffer_info[0]"),0,12);
    glGenBuffers(1,&buffer);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(rgba),rgba,GL_STATIC_DRAW);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,8,buffer);
    glDrawArrays(GL_TRIANGLES,0,3);glFinish();glDeleteBuffers(1,&buffer);glDeleteTextures(1,&image);glActiveTexture(GL_TEXTURE0);
}
static void output_bytes(uint8_t data[N*4+32])
{memset(data,0xe7,N*4+32);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,data+16);assert(glGetError()==GL_NO_ERROR);for(unsigned i=0;i<16;++i)assert(data[i]==0xe7 && data[N*4+31-i]==0xe7);}
static GLuint texture(GLenum format,unsigned samples)
{
    GLenum target=samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D;GLuint result=0;glGenTextures(1,&result);glBindTexture(target,result);
    if(samples>1)glTexStorage2DMultisample(target,samples,format,W,H,GL_TRUE);
    else{glTexStorage2D(target,1,format,W,H);glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
    assert(glGetError()==GL_NO_ERROR);return result;
}
static void sample_read(GLuint source,GLuint pipeline,unsigned sample,uint8_t data[N*4+32],bool stencil)
{
    glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glDisable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,source);glUseProgram(pipeline);
    if(stencil)glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_STENCIL_INDEX);
    glUniform1i(glGetUniformLocation(pipeline,"image"),0);glUniform1i(glGetUniformLocation(pipeline,"sample_index"),sample);glViewport(0,0,W,H);
    glDrawArrays(GL_TRIANGLES,0,3);glFinish();++draws;output_bytes(data);
}
static void verify_source_samples(GLuint color,GLuint depth,unsigned samples)
{
    uint8_t data[N*4+32];
    for(unsigned sample=0;sample<samples;++sample){
        sample_read(color,sample_program,sample,data,false);
        for(unsigned i=0;i<N;++i){if(memcmp(data+16+i*4,colors[sample],4))fprintf(stderr,"sample=%u pixel=%u actual=%u,%u,%u,%u expected=%u,%u,%u,%u AIR=%u\n",sample,i,data[16+i*4],data[17+i*4],data[18+i*4],data[19+i*4],colors[sample][0],colors[sample][1],colors[sample][2],colors[sample][3],actual_AIR);assert(!memcmp(data+16+i*4,colors[sample],4));++checked_samples;checked_bytes+=4;}
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,depth);glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_DEPTH_COMPONENT);
        sample_read(depth,depth_program,sample,data,false);float z=sample_depths[sample];uint32_t wanted;memcpy(&wanted,&z,4);
        for(unsigned i=0;i<N;++i){uint32_t value;memcpy(&value,data+16+i*4,4);assert(value==wanted);++checked_samples;checked_bytes+=4;}
        sample_read(depth,stencil_program,sample,data,true);
        for(unsigned i=0;i<N;++i){assert(data[16+i*4]==40+sample);++checked_samples;checked_bytes+=4;}
    }
}
static void test_samples(unsigned samples)
{
    GLuint color=texture(GL_RGBA8,samples),depth=texture(GL_DEPTH32F_STENCIL8,samples),fbo=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D_MULTISAMPLE,color,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D_MULTISAMPLE,depth,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    GLuint cache=0;assert(!sweet_depth32_clear_samples(depth,W,H,0.875,samples,&cache));glDeleteProgram(cache);cache=0;
    assert(!sweet_stencil8_clear_samples(depth,W,H,17,samples,&cache));glDeleteProgram(cache);
    // Select one real sample on each draw; a one-sample implementation fails.
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);glUseProgram(draw_program);glViewport(0,0,W,H);glEnable(GL_SAMPLE_MASK);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);
    glEnable(GL_STENCIL_TEST);glStencilMask(255);glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);glDisable(GL_SCISSOR_TEST);
    for(unsigned sample=0;sample<samples;++sample){glSampleMaski(0,1u<<sample);glStencilFunc(GL_ALWAYS,40+sample,255);
        set_draw_input(sample);if(!actual_AIR)glDrawArrays(GL_TRIANGLES,0,3);++draws;}
    glFinish();assert(glGetError()==GL_NO_ERROR);
    uint8_t data[N*4+32];verify_source_samples(color,depth,samples);
    GLuint resolved=texture(GL_RGBA8,1);glDisable(GL_SAMPLE_MASK);assert(!sweet_multisample_resolve_draw(color,resolved,W,H,samples,0,0));++resolves;
    glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,resolved,0);output_bytes(data);
    for(unsigned i=0;i<N;++i)for(unsigned c=0;c<4;++c){unsigned expected=0;for(unsigned s=0;s<samples;++s)expected+=colors[s][c];assert(data[16+i*4+c]==expected/samples);++checked_bytes;}
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,read_color,0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,depth);glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_DEPTH_COMPONENT);
    GLuint ztarget=texture(GL_DEPTH_COMPONENT32F,1);
    for(unsigned filter=0;filter<3;++filter){assert(!sweet_multisample_resolve_draw(depth,ztarget,W,H,samples,1,filter));++resolves;
        glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glBindTexture(GL_TEXTURE_2D,ztarget);glUseProgram(single_program);
        glUniform1i(glGetUniformLocation(single_program,"image"),0);glDrawArrays(GL_TRIANGLES,0,3);glFinish();output_bytes(data);++draws;
        float z=sample_depths[0];for(unsigned sample=1;sample<samples && filter;++sample)z=filter==1?fminf(z,sample_depths[sample]):fmaxf(z,sample_depths[sample]);uint32_t expected;memcpy(&expected,&z,4);
        for(unsigned i=0;i<N;++i){uint32_t value;memcpy(&value,data+16+i*4,4);assert(value==expected);checked_bytes+=4;}}
    // Re-read every original color/depth/stencil sample after all resolves.
    // Destination success alone does not prove the source remained intact.
    verify_source_samples(color,depth,samples);
    assert(sweet_multisample_resolve_draw(color,resolved,W,H,1,0,0)==-EINVAL);
    assert(sweet_multisample_resolve_draw(color,resolved,W,H,samples,0,1)==-EINVAL);
    glDeleteTextures(1,&color);glDeleteTextures(1,&depth);glDeleteTextures(1,&resolved);glDeleteTextures(1,&ztarget);glDeleteFramebuffers(1,&fbo);
}
int main(void)
{
    assert(sweet_multisample_layout(17,9,4,4,320,64,2944));assert(!sweet_multisample_layout(17,9,4,4,64,64,2944));
    assert(!sweet_multisample_layout(17,9,4,3,320,64,2944));assert(!sweet_multisample_layout(17,9,4,4,320,64,2943));
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLConfig config;EGLint count;
    assert(eglChooseConfig(display,attrs,&config,1,&count) && count);EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    printf("Host renderer: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    const char *generated=getenv("SWEET_MSAA_GENERATED_DIR");actual_AIR=generated && *generated;
    draw_program=actual_AIR?generated_program(generated):program("#version 310 es\nprecision highp float;uniform vec4 color;layout(location=0) out vec4 out_color;void main(){out_color=color;}");
    sample_program=program("#version 310 es\nprecision highp float;uniform highp sampler2DMS image;uniform int sample_index;layout(location=0) out vec4 color;void main(){color=texelFetch(image,ivec2(gl_FragCoord.xy),sample_index);}");
    depth_program=program("#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2DMS image;uniform int sample_index;layout(location=0) out vec4 color;void main(){uint raw=floatBitsToUint(texelFetch(image,ivec2(gl_FragCoord.xy),sample_index).r);color=vec4(uvec4(raw,raw>>8u,raw>>16u,raw>>24u)&255u)/255.0;}");
    stencil_program=program("#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2DMS image;uniform int sample_index;layout(location=0) out vec4 color;void main(){uint raw=texelFetch(image,ivec2(gl_FragCoord.xy),sample_index).r;color=vec4(float(raw)/255.0,0,0,0);}");
    single_program=program("#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2D image;layout(location=0) out vec4 color;void main(){uint raw=floatBitsToUint(texelFetch(image,ivec2(gl_FragCoord.xy),0).r);color=vec4(uvec4(raw,raw>>8u,raw>>16u,raw>>24u)&255u)/255.0;}");
    read_color=texture(GL_RGBA8,1);glGenFramebuffers(1,&read_fbo);glBindFramebuffer(GL_FRAMEBUFFER,read_fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,read_color,0);
    test_samples(2);test_samples(4);assert(glGetError()==GL_NO_ERROR);
    glDeleteProgram(draw_program);glDeleteProgram(sample_program);glDeleteProgram(depth_program);glDeleteProgram(stencil_program);glDeleteProgram(single_program);glDeleteTextures(1,&read_color);glDeleteFramebuffers(1,&read_fbo);glDeleteVertexArrays(1,&vao);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
    printf("Multisample GLES PASS: draws=%u individual_samples=%u checked_bytes=%u resolves=%u samples=2,4 depth_filters=sample0,min,max source_samples_unchanged=1 host_only=1\n",draws,checked_samples,checked_bytes,resolves);return 0;
}
