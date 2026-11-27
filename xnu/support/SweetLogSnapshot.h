/* SPDX-License-Identifier: BSD-2-Clause
 * One externally serialized writer, independent nonblocking readers.
 * Every cell is atomic; a rejected copy never advances the caller cursor.
 * SC operations provide one order for the epoch, cells and committed total.
 * No allocation, storage, transport or control operation occurs here. */
#ifndef SWEET_LOG_SNAPSHOT_H
#define SWEET_LOG_SNAPSHOT_H
#ifdef SWEET_LOG_HOST_TEST
#include <stdint.h>
#include <stdatomic.h>
#define SL_LOAD(p) atomic_load_explicit((p),memory_order_seq_cst)
#define SL_STORE(p,v) atomic_store_explicit((p),(v),memory_order_seq_cst)
#else
#include <sys/types.h>
#include <machine/atomic.h>
#define SL_LOAD(p) os_atomic_load((p),seq_cst)
#define SL_STORE(p,v) os_atomic_store((p),(v),seq_cst)
#endif
#ifndef SWEET_LOG_RING_BYTES
#define SWEET_LOG_RING_BYTES (1u<<20)
#endif
enum { SWEET_LOG_COPY_BOUND=1024 };
struct sweet_log_snapshot {
    uint64_t _Atomic epoch,total;
    unsigned char _Atomic cells[SWEET_LOG_RING_BYTES];
};
static inline int sl_append_serialized(struct sweet_log_snapshot *ring,const char *data,unsigned bytes)
{
    if(!ring || (!data && bytes))return 0;
    if(!bytes)return 1;
    uint64_t total=SL_LOAD(&ring->total),epoch=SL_LOAD(&ring->epoch);
    if((epoch&1) || bytes>~(uint64_t)0-total || epoch>~(uint64_t)0-2)return 0;
    SL_STORE(&ring->epoch,epoch+1);
    unsigned original=bytes;uint64_t at=total;
    if(bytes>SWEET_LOG_RING_BYTES){unsigned skip=bytes-SWEET_LOG_RING_BYTES;data+=skip;at+=skip;bytes=SWEET_LOG_RING_BYTES;}
    for(unsigned i=0;i<bytes;++i)SL_STORE(&ring->cells[(at+i)%SWEET_LOG_RING_BYTES],(unsigned char)data[i]);
    SL_STORE(&ring->total,total+original);
    SL_STORE(&ring->epoch,epoch+2);
    return 1;
}
static inline unsigned sl_copy_snapshot(const struct sweet_log_snapshot *ring,uint64_t *cursor,char *output,unsigned maximum)
{
    if(!ring || !cursor || !output || !maximum)return 0;
    uint64_t epoch=SL_LOAD(&ring->epoch);if(epoch&1)return 0;
    uint64_t total=SL_LOAD(&ring->total),at=*cursor;
    if(at>total)at=0;
    if(total-at>SWEET_LOG_RING_BYTES)at=total-SWEET_LOG_RING_BYTES;
    uint64_t left=total-at;unsigned count=left<maximum?(unsigned)left:maximum;
    if(count>SWEET_LOG_COPY_BOUND)count=SWEET_LOG_COPY_BOUND;
#ifdef SWEET_LOG_TEST_COPY_HOOK
    SWEET_LOG_TEST_COPY_HOOK();
#endif
    for(unsigned i=0;i<count;++i)output[i]=(char)SL_LOAD(&ring->cells[(at+i)%SWEET_LOG_RING_BYTES]);
    if(SL_LOAD(&ring->epoch)!=epoch)return 0;
    *cursor=at+count;return count;
}
static inline uint64_t sl_committed_total(const struct sweet_log_snapshot *ring)
{return ring?SL_LOAD(&ring->total):0;}
#undef SL_LOAD
#undef SL_STORE
#endif
