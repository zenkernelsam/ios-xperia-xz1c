/* Exercise the real old console mirror plus the candidate snapshot helper.
 * Blocking hooks belong solely to this deterministic host fixture. */
#define SWEET_LOG_HOST_TEST 1
void log_test_copy_hook(void);
#define SWEET_LOG_TEST_COPY_HOOK log_test_copy_hook
#include "../../xnu/support/SweetLogSnapshot.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
void legacy_append(const char *,int);
unsigned legacy_copy(uint64_t *,char *,unsigned);
uint64_t legacy_total(void),legacy_dropped(void);
void current_append(const char *,int);
unsigned current_copy(uint64_t *,char *,unsigned);
uint64_t current_total(void),current_dropped(void);
static pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static atomic_int hook_enabled;
static int entered,released;
void log_test_copy_hook(void)
{
    if(!atomic_load(&hook_enabled))return;
    pthread_mutex_lock(&gate);entered=1;pthread_cond_broadcast(&condition);
    while(!released)pthread_cond_wait(&condition,&gate);
    pthread_mutex_unlock(&gate);
}
void log_test_writer_hook(void){log_test_copy_hook();}
static void wait_reader(void)
{pthread_mutex_lock(&gate);while(!entered)pthread_cond_wait(&condition,&gate);pthread_mutex_unlock(&gate);}
static void release_reader(void)
{pthread_mutex_lock(&gate);released=1;pthread_cond_broadcast(&condition);pthread_mutex_unlock(&gate);}
static struct sweet_log_snapshot sample,stress,wrap;
static uint64_t cursor;static unsigned copied;static char output[2048];
static void *old_reader(void *unused)
{(void)unused;copied=legacy_copy(&cursor,output,sizeof(output));return NULL;}
static void *new_reader(void *unused)
{(void)unused;copied=sl_copy_snapshot(&sample,&cursor,output,sizeof(output));return NULL;}
static void *current_writer(void *input)
{const char *text=input;current_append(text,(int)strlen(text));return NULL;}
enum { RECORDS=10000, RECORD_BYTES=8 };
static atomic_int writer_done;
static char expected_byte(uint64_t offset)
{return (char)((offset/RECORD_BYTES+3*(offset%RECORD_BYTES))&255);}
static void *writer(void *unused)
{
    (void)unused;char record[RECORD_BYTES];
    for(unsigned i=0;i<RECORDS;++i){for(unsigned j=0;j<RECORD_BYTES;++j)record[j]=expected_byte((uint64_t)i*RECORD_BYTES+j);assert(sl_append_serialized(&stress,record,sizeof(record)));if(!(i&15))sched_yield();}
    atomic_store(&writer_done,1);return NULL;
}
static void *reader(void *unused)
{
    (void)unused;uint64_t at=0;char bytes[257];
    while(!atomic_load(&writer_done) || at<sl_committed_total(&stress)) {
        uint64_t before=at;unsigned n=sl_copy_snapshot(&stress,&at,bytes,sizeof(bytes));
        if(!n){assert(at==before);sched_yield();continue;}
        assert(at==before+n);for(unsigned i=0;i<n;++i)assert(bytes[i]==expected_byte(before+i));
    }
    assert(at==RECORDS*RECORD_BYTES);return NULL;
}
int main(void)
{
    const char initial[]="before\n",next[]="missing line\n";pthread_t thread;
    legacy_append(initial,sizeof(initial)-1);atomic_store(&hook_enabled,1);
    assert(!pthread_create(&thread,NULL,old_reader,NULL));wait_reader();
    legacy_append(next,sizeof(next)-1);assert(legacy_dropped()==sizeof(next)-1);
    release_reader();assert(!pthread_join(thread,NULL));assert(copied==sizeof(initial)-1 && legacy_total()==sizeof(initial)-1);
    assert(!memcmp(output,initial,copied));
    entered=released=0;cursor=0;
    assert(!pthread_create(&thread,NULL,current_writer,(void *)initial));wait_reader();
    assert(current_copy(&cursor,output,sizeof(output))==0 && cursor==0);
    current_append(next,sizeof(next)-1);assert(current_dropped()==sizeof(next)-1);
    release_reader();assert(!pthread_join(thread,NULL));
    copied=current_copy(&cursor,output,sizeof(output));assert(copied==sizeof(initial)-1 && current_total()==sizeof(initial)-1);
    assert(!memcmp(output,initial,copied));
    entered=released=0;cursor=0;assert(sl_append_serialized(&sample,initial,sizeof(initial)-1));
    assert(!pthread_create(&thread,NULL,new_reader,NULL));wait_reader();
    assert(sl_append_serialized(&sample,next,sizeof(next)-1));release_reader();assert(!pthread_join(thread,NULL));
    assert(copied==0 && cursor==0);atomic_store(&hook_enabled,0);
    copied=sl_copy_snapshot(&sample,&cursor,output,sizeof(output));assert(copied==sizeof(initial)+sizeof(next)-2);
    assert(!memcmp(output,initial,sizeof(initial)-1) && !memcmp(output+sizeof(initial)-1,next,sizeof(next)-1));
    pthread_t a,b;assert(!pthread_create(&a,NULL,writer,NULL));assert(!pthread_create(&b,NULL,reader,NULL));
    assert(!pthread_join(a,NULL) && !pthread_join(b,NULL));
    static char large[SWEET_LOG_RING_BYTES+23];for(unsigned i=0;i<sizeof(large);++i)large[i]=expected_byte(i);
    assert(sl_append_serialized(&wrap,large,sizeof(large)));cursor=0;
    copied=sl_copy_snapshot(&wrap,&cursor,output,sizeof(output));assert(copied==SWEET_LOG_COPY_BOUND);
    assert(cursor==23+copied && !memcmp(output,large+23,copied));
    cursor=0;atomic_store(&wrap.epoch,3);assert(!sl_copy_snapshot(&wrap,&cursor,output,sizeof(output)) && cursor==0);
    puts("Console snapshot PASS: pinned base reader-lock loss and deployed v729 concurrent-writer loss reproduced; serialized-writer prototype invalidates concurrent copies without cursor loss, 10000 records exact; prototype not installed");
    return 0;
}
