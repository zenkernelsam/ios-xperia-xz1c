/* SPDX-License-Identifier: BSD-2-Clause
 * Multi-producer byte stream. Cell generation and byte publish together;
 * readers never accept a cell belonging to another absolute byte ticket.
 * Storage is static/caller-retained; no allocation, IRQ or transport IO.
 * Reservations are bounded; cell publication is lock-free and monotonic.
 * Incomplete producer prefixes wait at the reader, rather than being repaired.
 */
#ifndef SWEET_LOG_TICKETS_H
#define SWEET_LOG_TICKETS_H
#ifdef SWEET_LOG_HOST_TEST
#include <stdint.h>
#include <stdatomic.h>
#define LT_LOAD(p) atomic_load_explicit((p),memory_order_seq_cst)
#define LT_CAS(p,old,value) ({ __typeof__(atomic_load(p)) expected=(old);atomic_compare_exchange_strong_explicit((p),&expected,(value),memory_order_seq_cst,memory_order_seq_cst); })
#else
#include <sys/types.h>
#include <machine/atomic.h>
#define LT_LOAD(p) os_atomic_load((p),seq_cst)
#define LT_CAS(p,old,value) os_atomic_cmpxchg((p),(old),(value),seq_cst)
#endif
#ifndef SWEET_LOG_TICKET_CAPACITY
#define SWEET_LOG_TICKET_CAPACITY (1u<<20)
#endif
enum { LT_RESERVE_ATTEMPTS=128, LT_COPY_BOUND=1024, LT_LAST_GENERATION=0xffffffu };
struct sweet_log_tickets {
    uint64_t _Atomic reserved;
    uint32_t _Atomic cells[SWEET_LOG_TICKET_CAPACITY];
};
static inline uint64_t lt_limit(void)
{return (uint64_t)LT_LAST_GENERATION*SWEET_LOG_TICKET_CAPACITY;}
static inline int lt_reserve(struct sweet_log_tickets *r,unsigned bytes,uint64_t *ticket)
{
    if(!r || !ticket || !bytes)return 0;
    for(unsigned attempt=0;attempt<LT_RESERVE_ATTEMPTS;++attempt) {
        uint64_t at=LT_LOAD(&r->reserved);
        if(at>lt_limit() || bytes>lt_limit()-at)return 0;
        if(LT_CAS(&r->reserved,at,at+bytes)){*ticket=at;return 1;}
    }
    return 0;
}
/* Returns bytes published into the retained window. Oversized prefixes and
 * producers overtaken by a whole ring are counted as unretained, never copied
 * into newer cells. Caller supplies a unique range from lt_reserve. */
static inline unsigned lt_publish(struct sweet_log_tickets *r,uint64_t ticket,const char *data,unsigned bytes)
{
    if(!r || (!data && bytes) || ticket>lt_limit() || bytes>lt_limit()-ticket)return 0;
    if(ticket+bytes>LT_LOAD(&r->reserved))return 0;
    if(bytes>SWEET_LOG_TICKET_CAPACITY){unsigned skip=bytes-SWEET_LOG_TICKET_CAPACITY;ticket+=skip;data+=skip;bytes=SWEET_LOG_TICKET_CAPACITY;}
    unsigned published=0;
    for(unsigned i=0;i<bytes;++i) {
        uint64_t at=ticket+i;uint32_t generation=(uint32_t)(at/SWEET_LOG_TICKET_CAPACITY)+1;
        uint32_t value=(generation<<8)|(unsigned char)data[i];
        uint32_t _Atomic *cell=&r->cells[at%SWEET_LOG_TICKET_CAPACITY];
        for(;;) {
            uint64_t head=LT_LOAD(&r->reserved);
            if(head-at>SWEET_LOG_TICKET_CAPACITY)break;
            uint32_t old=LT_LOAD(cell);
            if((old>>8)>=generation)break;
            if(LT_CAS(cell,old,value)){++published;break;}
        }
    }
    return published;
}
static inline unsigned lt_append(struct sweet_log_tickets *r,const char *data,unsigned bytes)
{
    uint64_t ticket=0;
    if(!data || !bytes || !lt_reserve(r,bytes,&ticket))return 0;
    return lt_publish(r,ticket,data,bytes);
}
static inline unsigned lt_copy(const struct sweet_log_tickets *r,uint64_t *cursor,char *output,unsigned maximum)
{
    if(!r || !cursor || !output || !maximum)return 0;
    uint64_t head=LT_LOAD(&r->reserved),at=*cursor;
    if(at>head)at=0;
    if(head-at>SWEET_LOG_TICKET_CAPACITY)at=head-SWEET_LOG_TICKET_CAPACITY;
    uint64_t left=head-at;unsigned limit=left<maximum?(unsigned)left:maximum;
    if(limit>LT_COPY_BOUND)limit=LT_COPY_BOUND;
    unsigned count=0;
    for(;count<limit;++count) {
        uint64_t position=at+count;uint32_t wanted=(uint32_t)(position/SWEET_LOG_TICKET_CAPACITY)+1;
        uint32_t value=LT_LOAD(&r->cells[position%SWEET_LOG_TICKET_CAPACITY]);
        if((value>>8)>wanted)return 0; // Concurrent overrun; leave cursor untouched.
        if((value>>8)<wanted)break;    // Earlier producer has not published yet.
        output[count]=(char)(value&255);
    }
    if(count)*cursor=at+count;
    return count;
}
static inline uint64_t lt_reserved_total(const struct sweet_log_tickets *r)
{return r?LT_LOAD(&r->reserved):0;}
#undef LT_LOAD
#undef LT_CAS
#endif
