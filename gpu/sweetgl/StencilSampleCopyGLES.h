/* SPDX-License-Identifier: BSD-2-Clause
 * Actual stencil texture transfers, following Gallium's bit-plane fallback.
 * Inputs/outputs are original integer 2D views; depth stays disabled/unwritten.
 * No CPU stencil expansion or sample repack. */
#pragma once
#include <GLES3/gl31.h>
#include <stdbool.h>
#include <errno.h>

static const char sweet_stencil_copy_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_stencil_load_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2D source_image;uniform int sample_index;uniform int bit_index;void main(){uint v=texelFetch(source_image,ivec2(gl_FragCoord.xy),0)[sample_index];if(((v>>uint(bit_index))&1u)==0u)discard;}";
static const char sweet_stencil_store_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2D source_image;layout(location=0) out uvec4 value;void main(){value=uvec4(texelFetch(source_image,ivec2(gl_FragCoord.xy),0).r,0,0,0);}";
static const char sweet_stencil_ms_store_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2DMS source_image;uniform int sample_count;layout(location=0) out uvec4 value;void main(){ivec2 p=ivec2(gl_FragCoord.xy);value=uvec4(0);for(int i=0;i<4;++i)if(i<sample_count)value[i]=texelFetch(source_image,p,i).r;}";

static inline int sweet_stencil_sample_copy_draw(GLuint linear,GLuint combined,unsigned width,unsigned height,unsigned samples,bool to_original,GLuint *cache)
{
    if(!linear || !combined || linear==combined || !width || !height || (samples!=1 && samples!=2 && samples!=4) || !cache)return -EINVAL;
    GLenum target=samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D;
    GLuint program=*cache,shaders[2]={0},fbo=0,vao=0;GLint oldFBO=0,oldProgram=0,oldUnit=0,good=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&oldFBO);glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_ACTIVE_TEXTURE,&oldUnit);
    if(!program){const GLenum types[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[]={sweet_stencil_copy_vertex,to_original?samples>1?sweet_stencil_ms_store_fragment:sweet_stencil_store_fragment:sweet_stencil_load_fragment};
        for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&good);if(!good)goto done;}
        program=glCreateProgram();glAttachShader(program,shaders[0]);glAttachShader(program,shaders[1]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&good);if(!good)goto done;*cache=program;}
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    if(to_original){glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,linear,0);const GLenum color=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&color);glReadBuffer(color);}
    else{glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,target,combined,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,target,combined,0);const GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);}
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);glActiveTexture(GL_TEXTURE0);glUniform1i(glGetUniformLocation(program,"source_image"),0);
    if(to_original){glBindTexture(target,combined);glTexParameteri(target,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_STENCIL_INDEX);if(samples>1)glUniform1i(glGetUniformLocation(program,"sample_count"),(GLint)samples);}
    else glBindTexture(GL_TEXTURE_2D,linear);
    glViewport(0,0,width,height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_SAMPLE_MASK);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    if(to_original){glDisable(GL_STENCIL_TEST);glStencilMask(0);glDrawArrays(GL_TRIANGLES,0,3);}
    else{
        glStencilMask(255);glClearStencil(0);glClear(GL_STENCIL_BUFFER_BIT);glEnable(GL_STENCIL_TEST);glStencilFunc(GL_ALWAYS,255,255);glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
        if(samples>1)glEnable(GL_SAMPLE_MASK);
        for(unsigned sample=0;sample<samples;++sample){if(samples>1)glSampleMaski(0,1u<<sample);glUniform1i(glGetUniformLocation(program,"sample_index"),(GLint)sample);
            for(unsigned bit=0;bit<8;++bit){glStencilMask(1u<<bit);glUniform1i(glGetUniformLocation(program,"bit_index"),(GLint)bit);glDrawArrays(GL_TRIANGLES,0,3);}}
    }
    glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(to_original){glBindTexture(target,combined);glTexParameteri(target,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_DEPTH_COMPONENT);}
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)oldFBO);glUseProgram((GLuint)oldProgram);glActiveTexture((GLenum)oldUnit);
    if(fbo)glDeleteFramebuffers(1,&fbo);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);if(program && *cache!=program)glDeleteProgram(program);
    return result;
}
