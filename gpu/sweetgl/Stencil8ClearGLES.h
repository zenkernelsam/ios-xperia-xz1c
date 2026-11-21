/* SPDX-License-Identifier: BSD-2-Clause; no CPU template or depth refill. */
#pragma once
#include <GLES3/gl31.h>
#include <errno.h>
// FD5 rejects Z32 fast clear. Keep a real fragment with discard, as in the
// pinned Gallium stencil fallback; do not depend on an empty-fragment clear.
static const char sweet_stencil_clear_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_stencil_clear_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform uint clear_value;uniform int bit_index;void main(){if(bit_index>=0 && ((clear_value>>uint(bit_index))&1u)==0u)discard;}";
static inline int sweet_stencil8_clear_samples(GLuint texture,unsigned width,unsigned height,unsigned value,unsigned samples,GLuint *cache)
{
    if(!texture || !width || !height || value>255 || !cache || (samples!=1 && samples!=2 && samples!=4))return -EINVAL;
    GLuint program=*cache,shaders[2]={0},fbo=0,vao=0;GLint previous=0,oldProgram=0,ok=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previous);glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);
    if(!program){const GLenum types[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[]={sweet_stencil_clear_vertex,sweet_stencil_clear_fragment};
        for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
        program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;*cache=program;}
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    // This is the combined Z32/S8 image backed by the original separate planes.
    // Keep its primary depth attachment explicit; depth test/writes stay off.
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D,texture,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D,texture,0);
    const GLenum no_color=GL_NONE;glDrawBuffers(1,&no_color);glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);glUniform1ui(glGetUniformLocation(program,"clear_value"),value);
    glViewport(0,0,width,height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_MASK);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_STENCIL_TEST);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glStencilFunc(GL_ALWAYS,255,255);glStencilMask(255);
    glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);glUniform1i(glGetUniformLocation(program,"bit_index"),-1);glDrawArrays(GL_TRIANGLES,0,3);
    glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
    for(unsigned bit=0;bit<8;++bit){glStencilMask(1u<<bit);glUniform1i(glGetUniformLocation(program,"bit_index"),(GLint)bit);glDrawArrays(GL_TRIANGLES,0,3);}
    glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previous);glUseProgram((GLuint)oldProgram);if(fbo)glDeleteFramebuffers(1,&fbo);
    for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program && *cache!=program)glDeleteProgram(program);
    return result;
}
static inline int sweet_stencil8_clear(GLuint texture,unsigned width,unsigned height,unsigned value,GLuint *cache)
{return sweet_stencil8_clear_samples(texture,width,height,value,1,cache);}
