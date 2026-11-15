/* SPDX-License-Identifier: BSD-2-Clause */
#define _GNU_SOURCE 1
#include "../../gpu/sweetgl/ProgramCacheStore.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"cache check line=%d failed: %s GL=%#x\n",__LINE__,#x,glGetError());exit(1);}}while(0)
static GLuint shader(GLenum type,const char *text)
{GLuint s=glCreateShader(type);glShaderSource(s,1,&text,NULL);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);CHECK(ok);return s;}
static void pixels(GLuint program)
{
    glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
    GLuint source=0,target=0,fbo=0,vao=0;const unsigned char data[16]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};
    glActiveTexture(GL_TEXTURE0);glGenTextures(1,&source);glBindTexture(GL_TEXTURE_2D,source);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,2,2);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,data);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,8,8);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glBindTexture(GL_TEXTURE_2D,source);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_BLEND);glDisable(GL_DITHER);glViewport(0,0,8,8);glDrawArrays(GL_TRIANGLES,0,3);glFinish();
    unsigned char raw[288];memset(raw,0xa5,sizeof(raw));glReadPixels(0,0,8,8,GL_RGBA,GL_UNSIGNED_BYTE,raw+16);CHECK(glGetError()==GL_NO_ERROR);
    for(unsigned i=0;i<sizeof(raw);++i){unsigned char expected=0xa5;if(i>=16 && i<272){unsigned n=i-16,x=(n/4)%8,y=(n/4)/8;expected=data[((y%2)*2+x%2)*4+n%4];}CHECK(raw[i]==expected);}
    glDeleteVertexArrays(1,&vao);glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&target);glDeleteTextures(1,&source);
}
int main(int argc,char **argv)
{
    CHECK(argc==2);EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(eglInitialize(d,NULL,NULL));CHECK(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig cfg;EGLint n=0;CHECK(eglChooseConfig(d,attr,&cfg,1,&n) && n==1);
    EGLint size[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE},version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLSurface surface=eglCreatePbufferSurface(d,cfg,size);
    EGLContext context=eglCreateContext(d,cfg,EGL_NO_CONTEXT,version);CHECK(eglMakeCurrent(d,surface,surface,context));
    const char *vs="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
    const char *fs="#version 310 es\nprecision highp float;uniform highp sampler2D image;uniform vec4 tint;layout(location=0) out vec4 colour;void main(){colour=texelFetch(image,ivec2(gl_FragCoord.xy)%ivec2(2),0)*tint;}";
    GLuint vert=shader(GL_VERTEX_SHADER,vs),frag=shader(GL_FRAGMENT_SHADER,fs),program=glCreateProgram();glProgramParameteri(program,GL_PROGRAM_BINARY_RETRIEVABLE_HINT,GL_TRUE);
    glAttachShader(program,vert);glAttachShader(program,frag);glLinkProgram(program);GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked);pixels(program);
    GLint formats=0,bytes=0;glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&formats);glGetProgramiv(program,GL_PROGRAM_BINARY_LENGTH,&bytes);CHECK(formats>0 && bytes>0 && bytes<(GLint)SWEET_PROGRAM_CACHE_MAX);
    unsigned char *binary=malloc((size_t)bytes),identity[32],key[32];memset(identity,0x37,sizeof(identity));SweetRenderPipelineConfig config={0};config.format=80;config.write_mask=15;
    sweet_cache_key(identity,&config,vs,strlen(vs),fs,strlen(fs),key);GLsizei actual=0;GLenum format=0;glGetProgramBinary(program,bytes,&actual,&format,binary);CHECK(actual>0 && actual<=bytes && glGetError()==GL_NO_ERROR);
    size_t count=SWEET_PROGRAM_CACHE_HEADER+(size_t)actual;unsigned char *record=malloc(count);CHECK(sweet_cache_header(record,SWEET_PROGRAM_CACHE_BINARY,format,identity,key,binary,(size_t)actual,strlen(vs),strlen(fs)));
    memcpy(record+SWEET_PROGRAM_CACHE_HEADER,binary,(size_t)actual);CHECK(sweet_cache_validate(record,count,SWEET_PROGRAM_CACHE_BINARY,identity,key));
    char path[1024];CHECK(sweet_cache_path(argv[1],identity,key,SWEET_PROGRAM_CACHE_BINARY,path)==0);CHECK(sweet_cache_write_file(argv[1],path,identity,record,count)==0);
    CHECK(sweet_cache_write_file(argv[1],path,identity,record,count)==-EEXIST);
    glDeleteProgram(program);glDeleteShader(vert);glDeleteShader(frag);CHECK(eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(d,context);
    context=eglCreateContext(d,cfg,EGL_NO_CONTEXT,version);CHECK(eglMakeCurrent(d,surface,surface,context));unsigned char *loaded=NULL;size_t length=0;
    CHECK(sweet_cache_read_file(path,&loaded,&length)==0 && length==count && sweet_cache_validate(loaded,length,SWEET_PROGRAM_CACHE_BINARY,identity,key));
    program=glCreateProgram();glProgramBinary(program,format,loaded+SWEET_PROGRAM_CACHE_HEADER,(GLsizei)sweet_cache_get32(loaded+20));glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked && glGetError()==GL_NO_ERROR);pixels(program);
    loaded[length-1]^=1;CHECK(!sweet_cache_validate(loaded,length,SWEET_PROGRAM_CACHE_BINARY,identity,key));loaded[length-1]^=1;
    identity[0]^=1;CHECK(!sweet_cache_validate(loaded,length,SWEET_PROGRAM_CACHE_BINARY,identity,key));identity[0]^=1;
    key[0]^=1;CHECK(!sweet_cache_validate(loaded,length,SWEET_PROGRAM_CACHE_BINARY,identity,key));key[0]^=1;
    CHECK(!sweet_cache_validate(loaded,length-1,SWEET_PROGRAM_CACHE_BINARY,identity,key));sweet_cache_put32(loaded+20,SWEET_PROGRAM_CACHE_MAX+1);CHECK(!sweet_cache_validate(loaded,length,SWEET_PROGRAM_CACHE_BINARY,identity,key));
    free(loaded);free(record);free(binary);glDeleteProgram(program);eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(d,context);eglDestroySurface(d,surface);eglTerminate(d);
    printf("program cache host PASS original_and_fresh_context_pixels=128 guarded_bytes=576 binary_roundtrip=1 build_source_corruption_truncation_rejected=1 immutable_publication=1 no_files_deleted=1\n");return 0;
}
