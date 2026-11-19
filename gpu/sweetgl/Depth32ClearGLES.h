/* SPDX-License-Identifier: BSD-2-Clause
 * Draw-based clear respects the original depth plane's pixel rectangle. */
#pragma once
#include <GLES3/gl31.h>
#include <errno.h>
#include <math.h>
static const char sweet_depth_clear_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_depth_clear_fragment[]="#version 310 es\nprecision highp float;uniform float depth_value;void main(){gl_FragDepth=depth_value;}";
static inline int sweet_depth32_clear(GLuint texture,unsigned width,unsigned height,double value,GLuint *cache)
{
    if(!texture || !width || !height || !cache || !isfinite(value) || value<0 || value>1)return -EINVAL;
    GLuint program=*cache,shaders[2]={0},fbo=0,vao=0;GLint previous=0,ok=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previous);
    if(!program){
        const GLenum kinds[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[]={sweet_depth_clear_vertex,sweet_depth_clear_fragment};
        for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
            glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
        program=glCreateProgram();for(unsigned i=0;i<2;++i)glAttachShader(program,shaders[i]);glLinkProgram(program);
        glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;*cache=program;
    }
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,texture,0);
    const GLenum no_color=GL_NONE;glDrawBuffers(1,&no_color);glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);glUniform1f(glGetUniformLocation(program,"depth_value"),(GLfloat)value);
    glViewport(0,0,width,height);glDepthRangef(0,1);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);
    glDisable(GL_BLEND);glDisable(GL_STENCIL_TEST);glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previous);if(fbo)glDeleteFramebuffers(1,&fbo);
    for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
    if(program && *cache!=program)glDeleteProgram(program);
    return result;
}
