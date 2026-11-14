/* Generated ESSL binary16 arithmetic and graphics on actual host Mesa. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/diagnostics/HalfProbeValues.h"
static char *read_file(const char *directory,const char *name)
{char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);assert(n>0 && n<1048576);rewind(f);char *s=calloc(n+1,1);assert(s && fread(s,1,n,f)==n);fclose(f);return s;}
static GLuint shader(const char *directory,const char *name,GLenum kind)
{char *s=read_file(directory,name);GLuint h=glCreateShader(kind);glShaderSource(h,1,(const char **)&s,NULL);glCompileShader(h);free(s);GLint ok;glGetShaderiv(h,GL_COMPILE_STATUS,&ok);
    if(!ok){char e[8192];glGetShaderInfoLog(h,sizeof(e),NULL,e);fprintf(stderr,"half shader %s: %s\n",name,e);}assert(ok);return h;}
static GLuint program(GLuint a,GLuint b)
{GLuint p=glCreateProgram();glAttachShader(p,a);if(b)glAttachShader(p,b);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char e[8192];glGetProgramInfoLog(p,sizeof(e),NULL,e);fprintf(stderr,"half link: %s\n",e);}assert(ok);return p;}
int main(int argc,char **argv)
{
    assert(argc==2);EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));EGLConfig cfg;EGLint count;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,attrs,&cfg,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,version);assert(ctx!=EGL_NO_CONTEXT && eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,ctx));
    GLuint cs=shader(argv[1],"half_arithmetic.comp",GL_COMPUTE_SHADER),p=program(cs,0);glUseProgram(p);GLuint buffers[3];glGenBuffers(3,buffers);unsigned char bytes[3][16384];
    for(unsigned phase=0;phase<2;++phase){memset(bytes[0],0xa5,16384);memset(bytes[1],0xb6,16384);memset(bytes[2],0xc7,16384);half_probe_sources(bytes[1]+64,bytes[2]+64,phase);
        for(unsigned b=0;b<3;++b){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[b]);glBufferData(GL_SHADER_STORAGE_BUFFER,16384,bytes[b],GL_DYNAMIC_COPY);glBindBufferRange(GL_SHADER_STORAGE_BUFFER,b,buffers[b],64,16320);}
        GLuint info[16]={0};info[3]=64;info[7]=info[11]=1;info[8]=64;info[9]=info[10]=info[12]=info[13]=info[14]=1;glUniform4uiv(glGetUniformLocation(p,"_qdispatch_info[0]"),4,info);
        glDispatchCompute(1,1,1);glMemoryBarrier(GL_ALL_BARRIER_BITS);glFinish();assert(glGetError()==GL_NO_ERROR);
        for(unsigned b=0;b<3;++b){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[b]);const void *map=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,16384,GL_MAP_READ_BIT);assert(map);memcpy(bytes[b],map,16384);glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);}
        unsigned mismatch=half_probe_check(bytes[0],bytes[1],bytes[2],phase);fprintf(stderr,"actual half AIR arithmetic phase=%u checked=49152 mismatches=%u\n",phase,mismatch);
        if(mismatch){for(unsigned i=0;i<64;++i){uint16_t expected[12];half_probe_result(i,phase,expected);for(unsigned c=0;c<12;++c){uint16_t actual;memcpy(&actual,bytes[0]+64+i*24+c*2,2);
                    if(actual!=expected[c])fprintf(stderr,"half value i=%u c=%u actual=%04x expected=%04x\n",i,c,actual,expected[c]);}}}assert(!mismatch);
    }
    glDeleteBuffers(3,buffers);glDeleteProgram(p);glDeleteShader(cs);
    GLuint vs=shader(argv[1],"half_position.vert",GL_VERTEX_SHADER),fs=shader(argv[1],"half_colour.frag",GL_FRAGMENT_SHADER);p=program(vs,fs);glUseProgram(p);
    uint16_t positions[12]={0xbc00,0x3c00,0x3800,0x3c00,0x4200,0x3c00,0x3800,0x3c00,0xbc00,0xc200,0x3800,0x3c00};unsigned char page[16384]={0};memcpy(page,positions,24);
    GLuint words;glGenTextures(1,&words);glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,words);glTexImage2D(GL_TEXTURE_2D,0,GL_R32UI,4096,1,0,GL_RED_INTEGER,GL_UNSIGNED_INT,page);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glUniform1i(glGetUniformLocation(p,"_smtl_vertex_buffer0"),8);glUniform2ui(glGetUniformLocation(p,"_smtl_vertex_buffer_info[0]"),0,6);
    GLuint target,fbo,vao;glActiveTexture(GL_TEXTURE0);glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,17,9,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glViewport(0,0,17,9);glDisable(GL_DITHER);glDrawArrays(GL_TRIANGLES,0,3);glFinish();assert(glGetError()==GL_NO_ERROR);
    unsigned char output[17*9*4];glReadPixels(0,0,17,9,GL_RGBA,GL_UNSIGNED_BYTE,output);unsigned mismatch=0;for(unsigned i=0;i<153;++i){unsigned char e[4]={64,128,191,255};for(unsigned c=0;c<4;++c)mismatch+=output[i*4+c]!=e[c];}assert(!mismatch);
    puts("actual half AIR: packed buffer loads/stores, per-result rounding, conversion boundaries and vertex/fragment graphics PASS (host Mesa only)");return 0;
}
