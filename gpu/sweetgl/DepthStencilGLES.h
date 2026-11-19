/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include "MetalDepthStencilRules.h"
#include <GLES3/gl31.h>
static inline GLenum sweet_depth_compare(uint32_t compare)
{
    static const GLenum values[]={GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS};
    return compare<8?values[compare]:0;
}
static inline GLenum sweet_stencil_operation(uint32_t operation)
{
    static const GLenum values[]={GL_KEEP,GL_ZERO,GL_REPLACE,GL_INCR,GL_DECR,GL_INVERT,GL_INCR_WRAP,GL_DECR_WRAP};
    return operation<8?values[operation]:0;
}
static inline void sweet_stencil_face_apply(GLenum side,const SweetStencilFace *face,uint32_t reference)
{
    glStencilFuncSeparate(side,face->enabled?sweet_depth_compare(face->compare):GL_ALWAYS,(GLint)reference,face->enabled?face->read_mask:255);
    glStencilMaskSeparate(side,face->enabled?face->write_mask:0);
    glStencilOpSeparate(side,face->enabled?sweet_stencil_operation(face->fail):GL_KEEP,
        face->enabled?sweet_stencil_operation(face->depth_fail):GL_KEEP,face->enabled?sweet_stencil_operation(face->pass):GL_KEEP);
}
static inline bool sweet_depth_stencil_apply(const SweetDepthStencilState *state,bool depth_attached,bool stencil_attached)
{
    if(!sweet_depth_stencil_valid(state)) return false;
    if(depth_attached && (state->write || state->compare!=7)) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glDepthFunc(sweet_depth_compare(state->compare));glDepthMask(depth_attached && state->write?GL_TRUE:GL_FALSE);
    if(stencil_attached && (state->front.enabled || state->back.enabled)) glEnable(GL_STENCIL_TEST);
    else glDisable(GL_STENCIL_TEST);
    sweet_stencil_face_apply(GL_FRONT,&state->front,state->front_reference);
    sweet_stencil_face_apply(GL_BACK,&state->back,state->back_reference);
    return true;
}
