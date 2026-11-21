/* SPDX-License-Identifier: BSD-2-Clause
 * Mesa compact pipe_box fields are x,width,y,height,z,depth. */
#pragma once
#include "pipe/p_state.h"

static inline struct pipe_box sweet_depth_tiled_copy_box(unsigned width,unsigned height)
{
    return (struct pipe_box){.x=0,.y=0,.z=0,.width=(int)width,.height=(int)height,.depth=1};
}
