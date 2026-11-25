/* SPDX-License-Identifier: MIT
 * A5xx image visibility within an existing sysmem batch. The native emitters
 * use pinned Mesa fd5_cache_flush (Rob Clark, 2016) and the matching A5xx
 * CACHE_FLUSH_TS/timestamp wait from ios_sagit AdrenoPipeline.h.
 * This changes submission frequency only; every primitive still rounds XR,
 * waits for prior shader writes, invalidates UCHE and retains final fences. */
#pragma once
#include "freedreno/a5xx/fd5_emit.h"
#include "freedreno/freedreno_context.h"
#include "freedreno/freedreno_batch.h"
#include "util/u_threaded_context.h"
#include "state_tracker/st_context.h"
#include "main/mtypes.h"

static bool sweet_ordered_image_barrier(struct st_context *context,unsigned image_mask,uint32_t *sequence)
{
    if(!context || !context->ctx || !context->pipe || !image_mask || (image_mask&~255u) ||
        !sequence || *sequence==UINT32_MAX)return false;
    // The barrier follows an executed draw, never a merely enqueued GLThread
    // command. The original GL barrier remains responsible for async GL.
    if(context->ctx->GLThread.enabled)return false;
    struct pipe_context *pipe=threaded_context_unwrap_sync(context->pipe);
    if(!pipe)return false;
    struct fd_context *native=fd_context(pipe);
    // The optimized path is specific to the exact sysmem A5xx image contract.
    // Other renderers/layouts retain the original GL memory barrier path.
    if(!native->screen || native->screen->gen!=5 || !FD_DBG(SYSMEM))return false;
    struct fd_bo *scratch=fd5_context(native)->blit_mem;
    // Native Mesa event helpers use offset 0. Use a separate DWORD in that
    // same private context's existing 4KiB scratch BO, without allocating.
    const uint32_t scratch_offset=64;
    if(!scratch || fd_bo_size(scratch)<scratch_offset+sizeof(uint32_t))return false;
    struct fd_batch *batch=native->batch;
    if(!batch || batch->flushed || batch->nondraw || !batch->draw || !batch->num_draws ||
        batch->framebuffer.zsbuf || batch->framebuffer.samples>1)return false;
    struct fd_shaderimg_stateobj *images=&native->shaderimg[PIPE_SHADER_FRAGMENT];
    if(images->enabled_mask!=(image_mask|(image_mask<<8)))return false;
    for(unsigned slot=0;slot<8;++slot)if(image_mask&(1u<<slot)) {
        struct pipe_image_view *read=&images->si[slot],*write=&images->si[8+slot];
        if(!read->resource || read->resource!=write->resource ||
            read->format!=PIPE_FORMAT_R32G32B32A32_FLOAT || write->format!=read->format ||
            read->access!=PIPE_IMAGE_ACCESS_READ || write->access!=PIPE_IMAGE_ACCESS_WRITE ||
            read->u.tex.level || write->u.tex.level || read->u.tex.first_layer ||
            read->u.tex.last_layer || write->u.tex.first_layer || write->u.tex.last_layer ||
            read->resource->nr_samples>1)return false;
    }
    // Publish prior shader writes through a real event timestamp and wait for
    // that exact value before invalidating UCHE. The monotonic sequence is
    // owned by this program/context and never repeats across batches.
    uint32_t value=++*sequence;
    OUT_WFI5(batch->draw);
    OUT_PKT7(batch->draw,CP_EVENT_WRITE,4);
    OUT_RING(batch->draw,CACHE_FLUSH_TS);
    OUT_RELOC(batch->draw,scratch,scratch_offset,0,0);
    OUT_RING(batch->draw,value);
    OUT_PKT7(batch->draw,CP_WAIT_REG_MEM,6);
    OUT_RING(batch->draw,0x13); /* memory / equal */
    OUT_RELOC(batch->draw,scratch,scratch_offset,0,0);
    OUT_RING(batch->draw,value);
    OUT_RING(batch->draw,UINT32_MAX);
    OUT_RING(batch->draw,16); /* polling interval */
    OUT_PKT7(batch->draw,CP_WAIT_MEM_WRITES,0);
    fd5_cache_flush(batch,batch->draw);
    fd_batch_check_size(batch);
    return true;
}
