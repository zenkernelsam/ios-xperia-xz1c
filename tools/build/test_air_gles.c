/* Actual generated graphics ESSL through host Mesa GLES; hardware proof separate. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *source(const char *directory,const char *name)
{char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE *file=fopen(path,"rb");assert(file);fseek(file,0,SEEK_END);long size=ftell(file);assert(size>0 && size<1048576);rewind(file);char *data=calloc(size+1,1);assert(data && fread(data,1,size,file)==(size_t)size);fclose(file);return data;}
static GLuint program(const char *directory,const char *vertex,const char *fragment)
{
    GLuint result=glCreateProgram(),shader[2];const char *names[2]={vertex,fragment};GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
    for(unsigned i=0;i<2;++i){char *code=source(directory,names[i]);shader[i]=glCreateShader(kinds[i]);glShaderSource(shader[i],1,(const char **)&code,NULL);glCompileShader(shader[i]);free(code);
        GLint ok=0;glGetShaderiv(shader[i],GL_COMPILE_STATUS,&ok);if(!ok){char error[8192];glGetShaderInfoLog(shader[i],sizeof(error),NULL,error);fprintf(stderr,"%s: %s\n",names[i],error);}assert(ok);glAttachShader(result,shader[i]);}
    glLinkProgram(result);GLint ok=0;glGetProgramiv(result,GL_LINK_STATUS,&ok);if(!ok){char error[8192];glGetProgramInfoLog(result,sizeof(error),NULL,error);fprintf(stderr,"link: %s\n",error);}assert(ok);
    for(unsigned i=0;i<2;++i)glDeleteShader(shader[i]);GLint active=0;glGetProgramiv(result,GL_ACTIVE_ATTRIBUTES,&active);
    for(GLint i=0;i<active;++i){char name[256];GLint size;GLenum type;glGetActiveAttrib(result,i,sizeof(name),NULL,&size,&type,name);fprintf(stderr,"actual attribute %s location=%d\n",name,glGetAttribLocation(result,name));
        assert(!strcmp(name,"gl_VertexID") || !strcmp(name,"gl_InstanceID"));}
    return result;
}
static void vertexBuffer(GLuint program,const float *vertices,unsigned bytes)
{
    void *data=calloc(1,16384);assert(data);memcpy(data,vertices,bytes);GLuint texture;glGenTextures(1,&texture);glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32UI,4096,1,0,GL_RED_INTEGER,GL_UNSIGNED_INT,data);free(data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glUniform1i(glGetUniformLocation(program,"_smtl_vertex_buffer0"),8);glUniform2ui(glGetUniformLocation(program,"_smtl_vertex_buffer_info[0]"),0,bytes/4);
}
static void pixel(unsigned x,unsigned y,unsigned phase,unsigned char color[4])
{color[0]=(unsigned char)(x*29+y*7+phase*13);color[1]=(unsigned char)(x*5+y*31+phase*17);color[2]=(unsigned char)(x*19+y*11+phase*23);color[3]=255;}
static void sampleTexture(GLuint program,unsigned phase)
{
    unsigned char data[140];for(unsigned y=0;y<5;++y)for(unsigned x=0;x<7;++x)pixel(x,y,phase,data+y*28+x*4);
    GLuint texture;glGenTextures(1,&texture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,7,5,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glUniform1i(glGetUniformLocation(program,"_qsample0_3"),0);GLuint info[32]={7,5,1,1|(1u<<8)},flags[8]={0};
    glUniform4uiv(glGetUniformLocation(program,"_qtexture2d_info[0]"),8,info);GLint at=glGetUniformLocation(program,"_qsampler_info[0]");if(at>=0)glUniform1uiv(at,8,flags);
}
int main(int argc,char **argv)
{
    assert(argc==2);EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(display!=EGL_NO_DISPLAY);assert(eglInitialize(display,NULL,NULL));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig config;EGLint configs;
    assert(eglChooseConfig(display,attributes,&config,1,&configs) && configs);EGLint ctxAttributes[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxAttributes);assert(context!=EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));fprintf(stderr,"host renderer %s; version %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    GLuint solid=program(argv[1],"position.vert","color.frag"),textured=program(argv[1],"texture_position.vert","textured.frag");
    GLuint target,fbo,vao;glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,33,19,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_DITHER);glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    const float positions[12]={-1,1,0.5f,1,3,1,0.5f,1,-1,-3,0.5f,1};
    const float vertices[24]={-1,1,0.5f,1,0,0,0,0,3,1,0.5f,1,14,0,0,0,-1,-3,0.5f,1,0,10,0,0};
    for(unsigned phase=0;phase<4;++phase){
        if(phase){GLuint shader=phase==1?solid:textured;glUseProgram(shader);vertexBuffer(shader,phase==1?positions:vertices,phase==1?sizeof(positions):sizeof(vertices));
            glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);glViewport(phase==1?3:0,phase==1?2:0,phase==1?27:33,phase==1?15:19);
            glEnable(GL_SCISSOR_TEST);glScissor(phase==1?5:phase==3?3:0,phase==1 || phase==3?4:0,phase==1 || phase==3?23:33,phase==1 || phase==3?11:19);
            if(phase>=2)sampleTexture(shader,phase==3);glDrawArrays(GL_TRIANGLES,0,3);glFinish();}
        GLenum error=glGetError();if(error)fprintf(stderr,"phase %u GL error=%x\n",phase,error);assert(error==GL_NO_ERROR);
        unsigned char output[33*19*4];glReadPixels(0,0,33,19,GL_RGBA,GL_UNSIGNED_BYTE,output);unsigned mismatches=0;
        for(unsigned y=0;y<19;++y)for(unsigned x=0;x<33;++x){unsigned char expected[4]={255,0,0,255};
            if(phase==1 && x>=5 && x<28 && y>=4 && y<15){expected[0]=64;expected[1]=128;expected[2]=191;}
            if(phase>=2)pixel(((2*x+1)*7)/66,((2*y+1)*5)/38,phase==3 && x>=3 && x<26 && y>=4 && y<15,expected);
            for(unsigned c=0;c<4;++c)mismatches+=output[(y*33+x)*4+c]!=expected[c];}
        fprintf(stderr,"actual ESSL phase=%u checked=2508 mismatches=%u\n",phase,mismatches);assert(!mismatches);
    }
    puts("actual AIR->ESSL Mesa host graphics: builtin reflection, guarded vertex fetch, BGRA contract, viewport/scissor/cull, sampling, dynamic refresh PASS (not A540 hardware proof)");
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);eglTerminate(display);return 0;
}
