/* SPDX-License-Identifier: MIT; actual Mesa emitters over an authored ring. */
#include "../../gpu/sweetgl/OrderedImageBarrier.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int fd_mesa_debug=FD_DBG_SYSMEM;
static unsigned flushes,wfi_calls,size_checks,relocations;
static struct fd_bo original_bo={.iova=UINT64_C(0x41234000)};
uint32_t fd_bo_size(struct fd_bo *bo){assert(bo==&original_bo);return 4096;}
struct pipe_context *threaded_context_unwrap_sync(struct pipe_context *pipe){return pipe;}
void fd_batch_check_size(struct fd_batch *batch){assert(batch);++size_checks;}
void fd_wfi(struct fd_batch *batch,struct fd_ringbuffer *ring)
{assert(batch && batch->needs_wfi);++wfi_calls;OUT_WFI5(ring);batch->needs_wfi=false;}
static void attached(struct fd_ringbuffer *ring,struct fd_bo *bo){assert(ring && bo==&original_bo);}
static void reloc(struct fd_ringbuffer *ring,const struct fd_reloc *value)
{assert(value->iova==UINT64_C(0x41234040));OUT_RING(ring,(uint32_t)value->iova);OUT_RING(ring,(uint32_t)(value->iova>>32));++relocations;}
static void flush(struct pipe_context *pipe,struct pipe_fence_handle **fence,unsigned flags)
{(void)pipe;(void)fence;(void)flags;++flushes;}
int main(void)
{
    uint32_t stream[64];memset(stream,0xa5,sizeof(stream));
    const struct fd_ringbuffer_funcs funcs={.emit_reloc=reloc,.assert_attached=attached};
    struct fd_ringbuffer ring={.cur=stream,.end=stream+64,.start=stream,.funcs=&funcs,.size=256};
    struct fd_screen screen={.gen=5};
    struct fd5_context native={0};native.base.base.flush=flush;native.base.screen=&screen;native.blit_mem=&original_bo;
    struct fd_batch batch={.ctx=&native.base,.draw=&ring,.num_draws=1};native.base.batch=&batch;
    static struct gl_context gl;
    struct st_context context={.pipe=&native.base.base,.ctx=&gl};
    struct pipe_resource image={.nr_samples=1};
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask=0x101;
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[0]=(struct pipe_image_view){.resource=&image,.format=PIPE_FORMAT_R32G32B32A32_FLOAT,.access=PIPE_IMAGE_ACCESS_READ};
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[8]=(struct pipe_image_view){.resource=&image,.format=PIPE_FORMAT_R32G32B32A32_FLOAT,.access=PIPE_IMAGE_ACCESS_WRITE};
    uint32_t sequence=0;
    assert(sweet_ordered_image_barrier(&context,1,&sequence));
    const uint32_t expected[]={0x70268000,0x703d8003,0x41234040,0,0,0x70928000,
        0x70460004,4,0x41234040,0,1,
        0x70bc8006,0x13,0x41234040,0,1,UINT32_MAX,16,0x70928000,
        0x480e9185,0,0,0,0,0x12,0x70268000};
    assert(ring.cur-stream==26 && !memcmp(stream,expected,sizeof(expected)) && sequence==1);
    assert(flushes==0 && size_checks==1 && wfi_calls==1);
    for(unsigned i=26;i<64;++i)assert(stream[i]==0xa5a5a5a5);
    assert(sweet_ordered_image_barrier(&context,1,&sequence) && sequence==2);
    assert(ring.cur-stream==47 && stream[26+5]==2 && stream[26+10]==2);
    const uint32_t next_expected[]={0x70268000,0x70460004,4,0x41234040,0,2,
        0x70bc8006,0x13,0x41234040,0,2,UINT32_MAX,16,0x70928000,
        0x480e9185,0,0,0,0,0x12,0x70268000};
    assert(!memcmp(stream+26,next_expected,sizeof(next_expected)));
    for(unsigned i=47;i<64;++i)assert(stream[i]==0xa5a5a5a5);
    uint32_t *end=ring.cur;
    gl.GLThread.enabled=true;assert(!sweet_ordered_image_barrier(&context,1,&sequence));gl.GLThread.enabled=false;
    screen.gen=6;assert(!sweet_ordered_image_barrier(&context,1,&sequence));screen.gen=5;
    fd_mesa_debug=0;assert(!sweet_ordered_image_barrier(&context,1,&sequence));fd_mesa_debug=FD_DBG_SYSMEM;
    batch.flushed=true;assert(!sweet_ordered_image_barrier(&context,1,&sequence));batch.flushed=false;
    batch.nondraw=true;assert(!sweet_ordered_image_barrier(&context,1,&sequence));batch.nondraw=false;
    batch.framebuffer.samples=2;assert(!sweet_ordered_image_barrier(&context,1,&sequence));batch.framebuffer.samples=1;
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask=1;assert(!sweet_ordered_image_barrier(&context,1,&sequence));
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask=0x101;
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[8].access=PIPE_IMAGE_ACCESS_READ;assert(!sweet_ordered_image_barrier(&context,1,&sequence));
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[8].access=PIPE_IMAGE_ACCESS_WRITE;
    assert(!sweet_ordered_image_barrier(&context,0,&sequence) && !sweet_ordered_image_barrier(&context,256,&sequence));
    sequence=UINT32_MAX;assert(!sweet_ordered_image_barrier(&context,1,&sequence) && sequence==UINT32_MAX);
    assert(ring.cur==end && !memcmp(stream,expected,sizeof(expected)) && flushes==0);
    puts("Ordered image barrier PASS: exact pinned A5xx first-use scratch CP initialization, monotonic timestamp/WAIT_REG_MEM/UCHE packets, existing ring, zero context flush, guards/overflow and unsupported contract rejection; host command stream only");
}
