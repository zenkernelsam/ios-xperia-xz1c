/* Authored CPU-only concurrent logger fixture. No device/kernel activation. */
#define SWEET_LOG_HOST_TEST 1
#include "../../xnu/support/SweetLogTickets.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
static struct sweet_log_tickets paused,stale,parallel,overflow;
enum { WRITERS=8, RECORDS=2000, BYTES=16 };
static atomic_uint finished;
static uint32_t read32(const char *p)
{const unsigned char *b=(const unsigned char *)p;return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static void write32(char *p,uint32_t v)
{for(unsigned i=0;i<4;++i)p[i]=(char)(v>>(i*8));}
static void *producer(void *opaque)
{
    unsigned id=(unsigned)(uintptr_t)opaque;char record[BYTES];
    for(unsigned i=0;i<RECORDS;++i) {
        write32(record,0x7469636b);write32(record+4,id);write32(record+8,i);write32(record+12,id^i^0x5a98d381);
        assert(lt_append(&parallel,record,sizeof(record))==sizeof(record));
        if(!(i&31))sched_yield();
    }
    atomic_fetch_add(&finished,1);return NULL;
}
static void *consumer(void *opaque)
{
    (void)opaque;uint64_t cursor=0;char bytes[257],record[BYTES];unsigned used=0,count=0,next[WRITERS]={0};
    while(atomic_load(&finished)<WRITERS || cursor<lt_reserved_total(&parallel)) {
        uint64_t before=cursor;unsigned n=lt_copy(&parallel,&cursor,bytes,sizeof(bytes));
        if(!n){assert(cursor==before);sched_yield();continue;}
        assert(cursor==before+n);
        for(unsigned i=0;i<n;++i) {
            record[used++]=bytes[i];if(used!=BYTES)continue;used=0;
            uint32_t id=read32(record+4),index=read32(record+8);assert(read32(record)==0x7469636b && id<WRITERS);
            assert(index==next[id]++ && read32(record+12)==(id^index^0x5a98d381));++count;
        }
    }
    assert(used==0 && count==WRITERS*RECORDS);
    for(unsigned i=0;i<WRITERS;++i)assert(next[i]==RECORDS);return NULL;
}
int main(void)
{
    char output[2048];uint64_t a,b,cursor=0;
    assert(lt_reserve(&paused,4,&a) && a==0);assert(lt_reserve(&paused,4,&b) && b==4);
    assert(lt_publish(&paused,b,"BBBB",4)==4);
    assert(lt_copy(&paused,&cursor,output,sizeof(output))==0 && cursor==0);
    assert(lt_publish(&paused,a,"AAAA",4)==4);
    assert(lt_copy(&paused,&cursor,output,sizeof(output))==8 && !memcmp(output,"AAAABBBB",8));
    assert(lt_reserve(&stale,1,&a) && a==0);
    static char wrap[SWEET_LOG_TICKET_CAPACITY+1];for(unsigned i=0;i<sizeof(wrap);++i)wrap[i]=(char)(i*19);
    assert(lt_append(&stale,wrap,sizeof(wrap))==SWEET_LOG_TICKET_CAPACITY);
    assert(lt_publish(&stale,a,"X",1)==0);cursor=0;
    unsigned n=lt_copy(&stale,&cursor,output,sizeof(output));assert(n==LT_COPY_BOUND && cursor==2+n);
    assert(!memcmp(output,wrap+1,n));
    uint64_t bad=99;assert(!lt_reserve(&paused,0,&bad) && bad==99);
    assert(!lt_publish(&paused,100,"x",1));
    atomic_store(&overflow.reserved,lt_limit());assert(!lt_reserve(&overflow,1,&bad));
    pthread_t producers[WRITERS],reader;assert(!pthread_create(&reader,NULL,consumer,NULL));
    for(unsigned i=0;i<WRITERS;++i)assert(!pthread_create(&producers[i],NULL,producer,(void *)(uintptr_t)i));
    for(unsigned i=0;i<WRITERS;++i)assert(!pthread_join(producers[i],NULL));assert(!pthread_join(reader,NULL));
    printf("Log tickets PASS: writers=%u records=%u exact_bytes=%u; pending prefix waits, stale writer cannot overwrite next generation, wrap/limit refusal; host only\n",WRITERS,WRITERS*RECORDS,WRITERS*RECORDS*BYTES);
    return 0;
}
