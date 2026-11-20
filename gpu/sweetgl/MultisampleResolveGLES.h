/* SPDX-License-Identifier: BSD-2-Clause
 * Resolve original samples in a real GLES draw. No CPU averaging/repack. */
#pragma once
#include <GLES3/gl31.h>
#include <errno.h>
#include "MultisampleLayout.h"
static const char sweet_ms_resolve_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_ms_color_resolve_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2DMS source_image;uniform int samples;layout(location=0) out vec4 color;void main(){ivec2 p=ivec2(gl_FragCoord.xy);vec4 value=vec4(0);for(int i=0;i<4;++i)if(i<samples)value+=texelFetch(source_image,p,i);color=value/float(samples);}";
static const char sweet_ms_depth_resolve_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2DMS source_image;uniform int samples;uniform int mode;void main(){ivec2 p=ivec2(gl_FragCoord.xy);float value=texelFetch(source_image,p,0).r;for(int i=1;i<4;++i)if(i<samples && mode!=0){float v=texelFetch(source_image,p,i).r;value=mode==1?min(value,v):max(value,v);}gl_FragDepth=value;}";
static inline int sweet_multisample_resolve_draw(GLuint source,GLuint target,uint32_t width,uint32_t height,
    uint32_t samples,uint32_t aspect,uint32_t filter)
{
    if(!source || !target || source==target || !width || !height || (samples!=2 && samples!=4) ||
        aspect>1 || (aspect==0 && filter) || filter>2)return -EINVAL;
    GLuint shaders[2]={0},program=0,fbo=0,vao=0;GLint valid=0,oldFBO=0,oldProgram=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&oldFBO);glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);
    const GLenum kinds[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
    const char *sources[2]={sweet_ms_resolve_vertex,aspect?sweet_ms_depth_resolve_fragment:sweet_ms_color_resolve_fragment};
    for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(kinds[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&valid);if(!valid)goto done;}
    program=glCreateProgram();glAttachShader(program,shaders[0]);glAttachShader(program,shaders[1]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&valid);if(!valid)goto done;
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,aspect?GL_DEPTH_ATTACHMENT:GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
    if(aspect){const GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);}else{const GLenum color=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&color);glReadBuffer(color);}
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,source);
    glUniform1i(glGetUniformLocation(program,"source_image"),0);glUniform1i(glGetUniformLocation(program,"samples"),(GLint)samples);
    if(aspect)glUniform1i(glGetUniformLocation(program,"mode"),(GLint)filter);
    glViewport(0,0,width,height);glDepthRangef(0,1);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glDisable(GL_SAMPLE_MASK);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DITHER);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    if(aspect){glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);}else{glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);}
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)oldFBO);glUseProgram((GLuint)oldProgram);
    if(fbo)glDeleteFramebuffers(1,&fbo);if(program)glDeleteProgram(program);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
    return result;
}
