/* SPDX-License-Identifier: BSD-2-Clause; no CPU template or depth refill. */
#pragma once
#include <GLES3/gl31.h>
#include <errno.h>
static inline int sweet_stencil8_clear_samples(GLuint texture,unsigned width,unsigned height,unsigned value,unsigned samples,GLuint *cache)
{
    if(!texture || !width || !height || value>255 || !cache || (samples!=1 && samples!=2 && samples!=4))return -EINVAL;
    GLuint fbo=0;GLint previous=0;int result=-EIO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previous);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    // This is the combined Z32/S8 image backed by the original separate planes.
    // Keep its primary depth attachment explicit; depth test/writes stay off.
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D,texture,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,samples>1?GL_TEXTURE_2D_MULTISAMPLE:GL_TEXTURE_2D,texture,0);
    const GLenum no_color=GL_NONE;glDrawBuffers(1,&no_color);glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    glViewport(0,0,width,height);glEnable(GL_SCISSOR_TEST);glScissor(0,0,width,height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_MASK);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_STENCIL_TEST);
    glStencilMask(255);glClearStencil((GLint)value);glClear(GL_STENCIL_BUFFER_BIT);glFinish();result=glGetError()==GL_NO_ERROR?0:-EIO;
done:
    glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)previous);if(fbo)glDeleteFramebuffers(1,&fbo);
    return result;
}
static inline int sweet_stencil8_clear(GLuint texture,unsigned width,unsigned height,unsigned value,GLuint *cache)
{return sweet_stencil8_clear_samples(texture,width,height,value,1,cache);}
