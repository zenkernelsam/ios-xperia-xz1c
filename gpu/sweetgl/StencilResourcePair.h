/* SPDX-License-Identifier: BSD-2-Clause
 * Pinned Mesa separate-z32s8 ownership: the primary owns one S8 resource. */
#pragma once
#if !defined(SWEET_STENCIL_PAIR_HOST_TEST)
#include "pipe/p_state.h"
#include "freedreno/freedreno_resource.h"
#endif
#include <stdbool.h>
#include <stddef.h>
#include "MultisampleLayout.h"
static inline bool sweet_stencil_pair_take_samples(struct pipe_resource *primary,struct pipe_resource **secondary,unsigned samples)
{
    if(!sweet_multisample_count(samples) || !primary || !secondary || !*secondary)return false;
    struct pipe_resource *stencil=*secondary;
    if(primary==stencil || primary->screen!=stencil->screen || primary->target!=PIPE_TEXTURE_2D || stencil->target!=PIPE_TEXTURE_2D ||
       primary->format!=PIPE_FORMAT_Z32_FLOAT || fd_resource(primary)->internal_format!=PIPE_FORMAT_Z32_FLOAT ||
       stencil->format!=PIPE_FORMAT_S8_UINT || fd_resource(stencil)->internal_format!=PIPE_FORMAT_S8_UINT ||
       fd_resource(primary)->stencil || fd_resource(stencil)->stencil ||
       primary->width0!=stencil->width0 || primary->height0!=stencil->height0 || primary->depth0!=1 || stencil->depth0!=1 ||
       primary->array_size!=1 || stencil->array_size!=1 || primary->last_level || stencil->last_level ||
       (primary->nr_samples?primary->nr_samples:1)!=samples || (stencil->nr_samples?stencil->nr_samples:1)!=samples ||
       (primary->nr_storage_samples?primary->nr_storage_samples:1)!=samples || (stencil->nr_storage_samples?stencil->nr_storage_samples:1)!=samples)return false;
    primary->format=PIPE_FORMAT_Z32_FLOAT_S8X24_UINT;
    fd_resource(primary)->stencil=fd_resource(stencil);
    *secondary=NULL;
    return true;
}
static inline bool sweet_stencil_pair_take(struct pipe_resource *primary,struct pipe_resource **secondary)
{return sweet_stencil_pair_take_samples(primary,secondary,1);}
