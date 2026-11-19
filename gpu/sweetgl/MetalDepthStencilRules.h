/* SPDX-License-Identifier: BSD-2-Clause
 * Metal compare/stencil values mapped to the real GLES depth/stencil state. */
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct SweetStencilFace {
    uint32_t enabled, compare, fail, depth_fail, pass, read_mask, write_mask;
} SweetStencilFace;
typedef struct SweetDepthStencilState {
    uint32_t compare, write, front_reference, back_reference;
    SweetStencilFace front, back;
} SweetDepthStencilState;

static inline SweetDepthStencilState sweet_depth_stencil_default(void)
{
    SweetDepthStencilState state={0}; state.compare=7; return state;
}
static inline bool sweet_depth_stencil_valid(const SweetDepthStencilState *state)
{
    if(!state || state->compare>7 || state->write>1 || state->front_reference>255 || state->back_reference>255) return false;
    const SweetStencilFace *faces[]={&state->front,&state->back};
    for(unsigned i=0;i<2;++i) {
        const SweetStencilFace *face=faces[i];
        if(face->enabled>1 || face->compare>7 || face->fail>7 || face->depth_fail>7 || face->pass>7 ||
           face->read_mask>255 || face->write_mask>255) return false;
    }
    return true;
}
