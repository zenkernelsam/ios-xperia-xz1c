/* Actual generated ESSL compute + original Aegir vertex results on host Mesa. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../gpu/diagnostics/NarrowProbeValues.h"
static char *read_file(const char *root,const char *name)
{char path[1024];snprintf(path,sizeof(path),"%s/%s",root,name);FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);assert(n>0 && n<1048576);rewind(f);char *s=calloc(n+1,1);assert(s && fread(s,1,n,f)==n);fclose(f);return s;}
static GLuint shader(GLenum kind,const char *text)
{GLuint s=glCreateShader(kind);glShaderSource(s,1,&text,NULL);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char err[8192];glGetShaderInfoLog(s,sizeof(err),NULL,err);fprintf(stderr,"shader error %s\n",err);}assert(ok);return s;}
static GLuint link_program(GLuint a,GLuint b)
{GLuint p=glCreateProgram();glAttachShader(p,a);if(b)glAttachShader(p,b);return p;}
static void linked(GLuint p)
{glLinkProgram(p);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char err[8192];glGetProgramInfoLog(p,sizeof(err),NULL,err);fprintf(stderr,"link error %s\n",err);}assert(ok);}
static void original_vertex(const char *directory)
{
    char *source=read_file(directory,"aegir_pass_vsh.vert");const char *prefix="out vec2 ";char names[3][128]={"gl_Position"};unsigned found=0;
    for(char *p=source;(p=strstr(p,prefix));p+=strlen(prefix)){char *start=p+strlen(prefix),*end=strchr(start,';');assert(end && (size_t)(end-start)<128 && found<2);memcpy(names[++found],start,end-start);}
    assert(found==2);GLuint vs=shader(GL_VERTEX_SHADER,source),fs=shader(GL_FRAGMENT_SHADER,"#version 310 es\nprecision highp float;out vec4 colour;void main(){colour=vec4(1);}");
    GLuint program=link_program(vs,fs);const char *varyings[]={names[0],names[1],names[2]};glTransformFeedbackVaryings(program,3,varyings,GL_INTERLEAVED_ATTRIBS);linked(program);glUseProgram(program);free(source);
    GLuint fbo,target;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    uint32_t vertices[4096]={0},uniforms[4096]={0};const int16_t coords[12]={-32768,-32768,32767,-1,0,1,-12345,19000,32767,32767,1,-32768};memcpy(vertices,coords,sizeof(coords));
    const float offset[2]={.25f,-.5f},aspect[2]={2,3};memcpy((char *)uniforms+336,offset,8);memcpy((char *)uniforms+344,aspect,8);
    GLuint textures[2];glGenTextures(2,textures);
    for(unsigned i=0;i<2;++i){glActiveTexture(GL_TEXTURE8+i);glBindTexture(GL_TEXTURE_2D,textures[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_R32UI,4096,1,0,GL_RED_INTEGER,GL_UNSIGNED_INT,i?uniforms:vertices);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        char n[128];snprintf(n,sizeof(n),"_smtl_vertex_buffer%u",i);glUniform1i(glGetUniformLocation(program,n),8+i);snprintf(n,sizeof(n),"_smtl_vertex_buffer_info[%u]",i);glUniform2ui(glGetUniformLocation(program,n),0,i?108:6);}
    GLuint output;glGenBuffers(1,&output);glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,output);float guard[56];for(unsigned i=0;i<56;++i)guard[i]=12345;
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,sizeof(guard),guard,GL_STATIC_READ);glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,0,output,16,48*4);glEnable(GL_RASTERIZER_DISCARD);
    GLenum error=glGetError();if(error)fprintf(stderr,"TF setup error=%x\n",error);assert(error==GL_NO_ERROR);
    glBeginTransformFeedback(GL_POINTS);error=glGetError();if(error)fprintf(stderr,"TF begin error=%x\n",error);assert(error==GL_NO_ERROR);
    glDrawArrays(GL_POINTS,0,6);error=glGetError();if(error)fprintf(stderr,"TF draw error=%x\n",error);assert(error==GL_NO_ERROR);
    glEndTransformFeedback();glFinish();glDisable(GL_RASTERIZER_DISCARD);assert(glGetError()==GL_NO_ERROR);
    const float *values=glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,0,sizeof(guard),GL_MAP_READ_BIT);assert(values);unsigned mismatch=0;
    for(unsigned i=0;i<56;++i){float wanted=12345;if(i>=4 && i<52){unsigned v=(i-4)/8,c=(i-4)%8;float x=coords[v*2]*(1.0f/32767),y=coords[v*2+1]*(1.0f/32767);
            float expected[8]={x,-y,-1,1,x*.5f+.5f,.5f-y*.5f,x*2+.25f,y*3-.5f};wanted=expected[c];}
        if(fabsf(values[i]-wanted)>1e-6){fprintf(stderr,"original vertex offset=%u actual=%g expected=%g\n",i,values[i],wanted);++mismatch;}}
    assert(!mismatch);glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);glDeleteBuffers(1,&output);glDeleteTextures(2,textures);glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&target);glDeleteProgram(program);glDeleteShader(vs);glDeleteShader(fs);
    puts("actual original Aegir vertex: signed16 dynamic input, position/UV/uniform offsets, transform-feedback guards PASS (host Mesa)");
}
int main(int argc,char **argv)
{
    assert(argc==2);EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));EGLConfig config;EGLint count;
    EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,a,&config,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(d,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    char *source=read_file(argv[1],"narrow_convert.comp");GLuint cs=shader(GL_COMPUTE_SHADER,source),p=link_program(cs,0);free(source);linked(p);glUseProgram(p);GLuint buffers[3];glGenBuffers(3,buffers);
    unsigned char original[3][16384];
    for(unsigned phase=0;phase<2;++phase){memset(original[0],0xa5,16384);memset(original[1],0xb6,16384);memset(original[2],0xc7,16384);narrow_source(original[1]+64,original[2]+64,phase);
        for(unsigned i=0;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,16384,original[i],GL_DYNAMIC_COPY);glBindBufferRange(GL_SHADER_STORAGE_BUFFER,i,buffers[i],64,16320);}
        GLuint info[16]={0};info[3]=64;info[7]=1;info[11]=1;info[8]=64;info[9]=info[10]=1;info[12]=info[13]=info[14]=1;glUniform4uiv(glGetUniformLocation(p,"_qdispatch_info[0]"),4,info);
        glDispatchCompute(1,1,1);glMemoryBarrier(GL_ALL_BARRIER_BITS);glFinish();assert(glGetError()==GL_NO_ERROR);
        for(unsigned i=0;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);void *mapped=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,16384,GL_MAP_READ_BIT);assert(mapped);memcpy(original[i],mapped,16384);glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);}
        unsigned mismatch=narrow_check(original[0],original[1],original[2],phase);fprintf(stderr,"actual narrow compute phase=%u checked=49152 mismatches=%u\n",phase,mismatch);assert(!mismatch);
    }glDeleteBuffers(3,buffers);glDeleteProgram(p);glDeleteShader(cs);original_vertex(argv[1]);puts("actual generated narrow ESSL: signed/unsigned8/16 scalar/vector results, original bytes and full guards PASS (host Mesa)");return 0;
}
