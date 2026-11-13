/* Writers serialize externally; readers never hold the writer lock. */
#ifndef VERMEER_LOG_RING_H
#define VERMEER_LOG_RING_H
#ifdef VERMEER_HOST_TEST
#include <stdint.h>
#else
#include <sys/types.h>
#endif
#include <stdatomic.h>
#ifndef VL_RING_SIZE
#define VL_RING_SIZE (1u << 20)
#endif
struct vl_ring {
    _Atomic uint64_t total,sequence;
    _Atomic unsigned char bytes[VL_RING_SIZE];
};
static inline void vl_append(struct vl_ring *r,const char *s,unsigned n)
{
    atomic_fetch_add_explicit(&r->sequence,1,memory_order_acq_rel);
    uint64_t at=atomic_load_explicit(&r->total,memory_order_relaxed),total=at+n;
    if(n>VL_RING_SIZE){unsigned skip=n-VL_RING_SIZE;s+=skip;at+=skip;n=VL_RING_SIZE;}
    for(unsigned i=0;i<n;++i)atomic_store_explicit(&r->bytes[(at+i)%VL_RING_SIZE],(unsigned char)s[i],memory_order_relaxed);
    atomic_store_explicit(&r->total,total,memory_order_release);
    atomic_fetch_add_explicit(&r->sequence,1,memory_order_release);
}
static inline unsigned vl_copy(const struct vl_ring *r,uint64_t *pos,char *out,unsigned max)
{
    uint64_t sequence=atomic_load_explicit(&r->sequence,memory_order_acquire);
    if(sequence&1)return 0;
    uint64_t total=atomic_load_explicit(&r->total,memory_order_acquire),cursor=*pos;
    if(cursor>total)cursor=0;
    if(total-cursor>VL_RING_SIZE)cursor=total-VL_RING_SIZE;
    uint64_t left=total-cursor;unsigned n=left<max?(unsigned)left:max;
    for(unsigned i=0;i<n;++i)out[i]=(char)atomic_load_explicit(&r->bytes[(cursor+i)%VL_RING_SIZE],memory_order_relaxed);
    atomic_thread_fence(memory_order_acquire);
    if(sequence!=atomic_load_explicit(&r->sequence,memory_order_relaxed))return 0;
    *pos=cursor+n;return n;
}
#endif
