/* SPDX-License-Identifier: BSD-2-Clause
 * Measure actual host GLES compile/link CPU wall time; no draw/device proof. */
#define _POSIX_C_SOURCE 200809L
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double now(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec+t.tv_nsec/1e9;}
static char *read_source(const char *path)
{FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long bytes=ftell(f);assert(bytes>0 && bytes<2*1024*1024);rewind(f);
    char *source=calloc(bytes+1,1);assert(source && fread(source,1,bytes,f)==(size_t)bytes);fclose(f);return source;}
static GLuint compile(const char *path,GLenum type)
{char *source=read_source(path);GLuint shader=glCreateShader(type);assert(shader);glShaderSource(shader,1,(const char **)&source,NULL);
    fprintf(stderr,"compile begin path=%s type=%x\n",path,type);fflush(stderr);double start=now();glCompileShader(shader);GLint valid=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&valid);
    char error[8192]={0};glGetShaderInfoLog(shader,sizeof(error),NULL,error);fprintf(stderr,"compile end path=%s seconds=%.6f valid=%d error=%s\n",path,now()-start,valid,error);fflush(stderr);free(source);assert(valid);return shader;}
int main(int argc,char **argv)
{assert(argc==3);EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));
    EGLConfig config;EGLint count=0;EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(display,attributes,&config,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    fprintf(stderr,"host GL=%s renderer=%s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
    GLuint vertex=compile(argv[1],GL_VERTEX_SHADER),fragment=compile(argv[2],GL_FRAGMENT_SHADER),program=glCreateProgram();glAttachShader(program,vertex);glAttachShader(program,fragment);
    fprintf(stderr,"link begin\n");fflush(stderr);double start=now();glLinkProgram(program);GLint valid=0;glGetProgramiv(program,GL_LINK_STATUS,&valid);char error[8192]={0};glGetProgramInfoLog(program,sizeof(error),NULL,error);
    fprintf(stderr,"link end seconds=%.6f valid=%d error=%s no_draw=1 host_only=1\n",now()-start,valid,error);assert(valid && glGetError()==GL_NO_ERROR);return 0;}
