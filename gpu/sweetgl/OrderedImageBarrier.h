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

static bool sweet_ordered_image_contract(const struct gl_context *gl,
    const struct fd_shaderimg_stateobj *images,unsigned image_mask)
{
    if(!gl || !gl->_Shader || !images || !image_mask || (image_mask&~255u))return false;
    const struct gl_program *program=gl->_Shader->CurrentProgram[MESA_SHADER_FRAGMENT];
    const unsigned count=2u*(unsigned)__builtin_popcount(image_mask);
    // st_bind_images uses dense shader indices, not GL binding numbers:
    // images[i] comes from ImageUnits[program->sh.ImageUnits[i]].
    if(!program || program->info.num_images!=count ||
        program->info.num_abos || program->sh.ShaderStorageBlocksWriteAccess ||
        images->enabled_mask!=((1u<<count)-1))return false;
    for(unsigned stage=0;stage<MESA_SHADER_STAGES;++stage)if(stage!=MESA_SHADER_FRAGMENT) {
        const struct gl_program *other=gl->_Shader->CurrentProgram[stage];
        if(other && (other->info.num_images || other->info.num_abos ||
            other->sh.ShaderStorageBlocksWriteAccess))return false;
    }
    unsigned reads[8],writes[8];for(unsigned i=0;i<8;++i)reads[i]=writes[i]=~0u;
    for(unsigned dense=0;dense<count;++dense) {
        const unsigned binding=program->sh.ImageUnits[dense];
        if(binding>=16 || !(image_mask&(1u<<(binding&7))))return false;
        const bool write=binding>=8;
        unsigned *slot=write?&writes[binding&7]:&reads[binding&7];
        if(*slot!=~0u)return false;
        const unsigned access=program->sh.image_access[dense];
        const unsigned expected=ACCESS_COHERENT|(write?ACCESS_NON_READABLE:ACCESS_NON_WRITEABLE);
        if(access!=expected)return false;
        *slot=dense;
    }
    for(unsigned slot=0;slot<8;++slot)if(image_mask&(1u<<slot)) {
        if(reads[slot]==~0u || writes[slot]==~0u)return false;
        const struct pipe_image_view *read=&images->si[reads[slot]],*write=&images->si[writes[slot]];
        if(!read->resource || read->resource!=write->resource ||
            read->resource->target!=PIPE_TEXTURE_2D ||
            read->format!=PIPE_FORMAT_R32G32B32A32_FLOAT || write->format!=read->format ||
            read->access!=PIPE_IMAGE_ACCESS_READ || write->access!=PIPE_IMAGE_ACCESS_WRITE ||
            read->u.tex.level || write->u.tex.level || read->u.tex.first_layer ||
            read->u.tex.last_layer || write->u.tex.first_layer || write->u.tex.last_layer ||
            read->resource->nr_samples>1 || read->resource->nr_storage_samples>1)return false;
    }
    return true;
}

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
    if(!sweet_ordered_image_contract(context->ctx,images,image_mask))return false;
    // Publish prior shader writes through a real event timestamp and wait for
    // that exact value before invalidating UCHE. The monotonic sequence is
    // owned by this program/context and never repeats across batches.
    uint32_t value=++*sequence;
    OUT_WFI5(batch->draw);
    if(value==1) {
        // A recycled private BO can contain any earlier value. Publish zero
        // through CP first so the first nonzero timestamp cannot match stale
        // data before CACHE_FLUSH_TS has actually completed.
        OUT_PKT7(batch->draw,CP_MEM_WRITE,3);
        OUT_RELOC(batch->draw,scratch,scratch_offset,0,0);
        OUT_RING(batch->draw,0);
        OUT_PKT7(batch->draw,CP_WAIT_MEM_WRITES,0);
    }
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
