/* SPDX-License-Identifier: BSD-2-Clause
 * Transfer actual depth samples through GLES, preserving original float bits.
 * Original pages are integer 2D views; working pages use real sampler2DMS.
 * No CPU conversion or sample averaging. */
#pragma once
#include <GLES3/gl31.h>
#include <errno.h>
#include <stdbool.h>

static const char sweet_depth_ms_copy_vertex[]="#version 310 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):gl_VertexID==1?vec2(3,-1):vec2(-1,3);gl_Position=vec4(p,0,1);}";
static const char sweet_depth_ms_load_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp usampler2D source_image;uniform int sample_index;void main(){uvec4 v=texelFetch(source_image,ivec2(gl_FragCoord.xy),0);gl_FragDepth=uintBitsToFloat(v[sample_index]);}";
static const char sweet_depth_ms_store_fragment[]="#version 310 es\nprecision highp float;precision highp int;uniform highp sampler2DMS source_image;uniform int sample_count;layout(location=0) out uvec4 value;void main(){ivec2 p=ivec2(gl_FragCoord.xy);value=uvec4(0);for(int i=0;i<4;++i)if(i<sample_count)value[i]=floatBitsToUint(texelFetch(source_image,p,i).r);}";

static inline int sweet_depth_multisample_copy_draw(GLuint linear,GLuint depth,unsigned width,unsigned height,
    unsigned samples,bool to_original,bool separate_stencil,GLuint *cached_program)
{
    if(!linear || !depth || linear==depth || !width || !height || (samples!=2 && samples!=4) || !cached_program)return -EINVAL;
    GLuint program=*cached_program,shaders[2]={0},framebuffer=0,vao=0;GLint oldFBO=0,oldProgram=0,oldUnit=0,ok=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&oldFBO);glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_ACTIVE_TEXTURE,&oldUnit);
    if(!program){const GLenum types[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};const char *sources[]={sweet_depth_ms_copy_vertex,to_original?sweet_depth_ms_store_fragment:sweet_depth_ms_load_fragment};
        for(unsigned i=0;i<2;++i){shaders[i]=glCreateShader(types[i]);glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok)goto done;}
        program=glCreateProgram();glAttachShader(program,shaders[0]);glAttachShader(program,shaders[1]);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)goto done;*cached_program=program;}
    glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    if(to_original){glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,linear,0);
        const GLenum color=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&color);glReadBuffer(color);}
    else{glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D_MULTISAMPLE,depth,0);
        if(separate_stencil)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,GL_TEXTURE_2D_MULTISAMPLE,depth,0);
        const GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);}
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glUseProgram(program);glActiveTexture(GL_TEXTURE0);
    if(to_original){glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,depth);
        if(separate_stencil)glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,GL_DEPTH_STENCIL_TEXTURE_MODE,GL_DEPTH_COMPONENT);
        glUniform1i(glGetUniformLocation(program,"sample_count"),(GLint)samples);}
    else glBindTexture(GL_TEXTURE_2D,linear);
    glUniform1i(glGetUniformLocation(program,"source_image"),0);glViewport(0,0,width,height);glDepthRangef(0,1);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glStencilMask(0);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DITHER);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(to_original?GL_FALSE:GL_TRUE);
    if(to_original){glDisable(GL_DEPTH_TEST);glDisable(GL_SAMPLE_MASK);}
    else{glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glEnable(GL_SAMPLE_MASK);}
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    if(to_original)glDrawArrays(GL_TRIANGLES,0,3);
    else for(unsigned sample=0;sample<samples;++sample){glSampleMaski(0,1u<<sample);glUniform1i(glGetUniformLocation(program,"sample_index"),(GLint)sample);glDrawArrays(GL_TRIANGLES,0,3);}
    glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    if(vao)glDeleteVertexArrays(1,&vao);glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)oldFBO);glUseProgram((GLuint)oldProgram);glActiveTexture((GLenum)oldUnit);
    if(framebuffer)glDeleteFramebuffers(1,&framebuffer);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
    if(program && *cached_program!=program)glDeleteProgram(program);
    return result;
}
