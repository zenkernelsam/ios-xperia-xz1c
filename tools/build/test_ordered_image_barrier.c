/* SPDX-License-Identifier: MIT; actual Mesa emitters over an authored ring. */
#ifndef SWEET_ORDERED_BARRIER_HEADER
#define SWEET_ORDERED_BARRIER_HEADER "../../gpu/sweetgl/OrderedImageBarrier.h"
#endif
#include SWEET_ORDERED_BARRIER_HEADER
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fd_mesa_debug=FD_DBG_SYSMEM;
static unsigned flushes,wfi_calls,size_checks,relocations;
static unsigned grows;
static uint32_t *owned_ring;
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
static void grow(struct fd_ringbuffer *ring,uint32_t bytes)
{
    assert(owned_ring && ring->start==owned_ring+8 && bytes==(uint32_t)ring->size);
    const unsigned capacity=(unsigned)ring->size/4,used=(unsigned)(ring->cur-ring->start);
    for(unsigned i=0;i<8;++i)assert(owned_ring[i]==0xa5a5a5a5 && owned_ring[capacity+8+i]==0xa5a5a5a5);
    uint32_t *next=malloc((capacity*2+16)*sizeof(uint32_t));assert(next);
    memset(next,0xa5,(capacity*2+16)*sizeof(uint32_t));
    memcpy(next+8,ring->start,used*sizeof(uint32_t));free(owned_ring);owned_ring=next;
    ring->start=next+8;ring->cur=ring->start+used;ring->end=ring->start+capacity*2;
    ring->size=(int)(capacity*2*sizeof(uint32_t));++grows;
}
int main(void)
{
    uint32_t stream[64];memset(stream,0xa5,sizeof(stream));
    const struct fd_ringbuffer_funcs funcs={.grow=grow,.emit_reloc=reloc,.assert_attached=attached};
    struct fd_ringbuffer ring={.cur=stream,.end=stream+64,.start=stream,.funcs=&funcs,.size=256};
    struct fd_screen screen={.gen=5};
    struct fd5_context native={0};native.base.base.flush=flush;native.base.screen=&screen;native.blit_mem=&original_bo;
    struct fd_batch batch={.ctx=&native.base,.draw=&ring,.num_draws=1};native.base.batch=&batch;
    static struct gl_context gl;
    struct gl_pipeline_object shader={0};
    struct gl_program fragment={0};
    gl._Shader=&shader;shader.CurrentProgram[MESA_SHADER_FRAGMENT]=&fragment;
    fragment.info.num_images=2;fragment.sh.ImageUnits[0]=0;fragment.sh.ImageUnits[1]=8;
    fragment.sh.image_access[0]=ACCESS_COHERENT|ACCESS_NON_WRITEABLE;
    fragment.sh.image_access[1]=ACCESS_COHERENT|ACCESS_NON_READABLE;
    struct st_context context={.pipe=&native.base.base,.ctx=&gl};
    struct pipe_resource image={.target=PIPE_TEXTURE_2D,.nr_samples=1};
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask=3;
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[0]=(struct pipe_image_view){.resource=&image,.format=PIPE_FORMAT_R32G32B32A32_FLOAT,.access=PIPE_IMAGE_ACCESS_READ};
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[1]=(struct pipe_image_view){.resource=&image,.format=PIPE_FORMAT_R32G32B32A32_FLOAT,.access=PIPE_IMAGE_ACCESS_WRITE};
    uint32_t sequence=0;
#ifdef SWEET_EXPECT_DENSE_REJECTION
    assert(!sweet_ordered_image_barrier(&context,1,&sequence));
    assert(!sequence && ring.cur==stream && !flushes && !size_checks);
    for(unsigned i=0;i<64;++i)assert(stream[i]==0xa5a5a5a5);
    puts("Prior ordered image barrier PASS: actual dense two-slot layout refused by the old sparse mask check; zero emitted words");
    return 0;
#endif
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
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask=3;
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[1].access=PIPE_IMAGE_ACCESS_READ;assert(!sweet_ordered_image_barrier(&context,1,&sequence));
    native.base.shaderimg[PIPE_SHADER_FRAGMENT].si[1].access=PIPE_IMAGE_ACCESS_WRITE;
    assert(!sweet_ordered_image_barrier(&context,0,&sequence) && !sweet_ordered_image_barrier(&context,256,&sequence));
    sequence=UINT32_MAX;assert(!sweet_ordered_image_barrier(&context,1,&sequence) && sequence==UINT32_MAX);
    assert(ring.cur==end && !memcmp(stream,expected,sizeof(expected)) && flushes==0);

    // Match the original star draw's 4212 vertices / 1404 primitives. Rotate
    // authored batches at irregular boundaries to ensure that a new batch
    // cannot reset this program/context's timestamp or reinitialize scratch.
    // This exercises actual emitters, not Mesa's scheduler or GPU execution.
    struct fd_batch batches[2]={{.ctx=&native.base,.draw=&ring,.num_draws=1},
                               {.ctx=&native.base,.draw=&ring,.num_draws=1}};
    owned_ring=malloc(80*sizeof(uint32_t));assert(owned_ring);
    memset(owned_ring,0xa5,80*sizeof(uint32_t));
    ring.start=owned_ring+8;ring.cur=ring.start;ring.end=ring.start+64;ring.size=256;
    sequence=0;
    unsigned words=0,switches=0;
    const unsigned initial_checks=size_checks,initial_relocs=relocations;
    for(unsigned primitive=0;primitive<1404;++primitive) {
        if(primitive && primitive%127==0)++switches;
        struct fd_batch *current=&batches[switches%2];
        current->num_draws=primitive%127+1;
        native.base.batch=current;
        uint32_t wanted[26];
        const unsigned length=primitive?21:26;
        memcpy(wanted,primitive?next_expected:expected,length*sizeof(uint32_t));
        wanted[primitive?5:10]=primitive+1;
        wanted[primitive?10:15]=primitive+1;
        assert(sweet_ordered_image_barrier(&context,1,&sequence));
        assert(sequence==primitive+1 && ring.cur==ring.start+words+length);
        assert(!memcmp(ring.start+words,wanted,length*sizeof(uint32_t)));
        for(unsigned i=0;i<8;++i)assert(owned_ring[i]==0xa5a5a5a5 && ring.end[i]==0xa5a5a5a5);
        words+=length;
    }
    assert(sequence==1404 && words==29489 && switches==11 && grows==9 && flushes==0);
    assert(size_checks-initial_checks==1404 && relocations-initial_relocs==2809);
    assert(!memcmp(ring.start,expected,sizeof(expected)));
    free(owned_ring);owned_ring=NULL;
    ring.start=stream;ring.end=stream+64;ring.size=256;
    // The last representable timestamp still emits; the next call must
    // leave both the ring and sequence unchanged and use the GL fallback.
    memset(stream,0xa5,sizeof(stream));ring.cur=stream;
    sequence=UINT32_MAX-1;
    assert(sweet_ordered_image_barrier(&context,1,&sequence) && sequence==UINT32_MAX);
    assert(stream[5]==UINT32_MAX && stream[10]==UINT32_MAX && ring.cur==stream+21);
    assert(!sweet_ordered_image_barrier(&context,1,&sequence) && ring.cur==stream+21);
    puts("Ordered image barrier PASS: exact pinned A5xx first-use scratch CP initialization, monotonic timestamp/WAIT_REG_MEM/UCHE packets, 1404 primitives/29489 continuous words across 11 authored batch changes and 9 guarded ring growths, existing ring, zero context flush, guards/overflow and unsupported contract rejection; host command stream only");
}
