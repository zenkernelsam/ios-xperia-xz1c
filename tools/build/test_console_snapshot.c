/* Snapshot consistency and writer progress while an actual reader overlaps. */
#define VERMEER_HOST_TEST 1
#define VL_RING_SIZE 1024u
#include "../../xnu/support/vermeer_log_ring.h"
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct vl_ring concurrent;
static atomic_bool done;
static void *writer(void *unused)
{
    (void)unused;char block[64];
    for(unsigned index=0;index<200000;++index){memset(block,'A'+index%26,sizeof(block));vl_append(&concurrent,block,sizeof(block));}
    atomic_store_explicit(&done,true,memory_order_release);return NULL;
}
static void boundaries(void)
{
    struct vl_ring ring={0};char output[VL_RING_SIZE+1];uint64_t cursor=0;
    vl_append(&ring,"abc",3);assert(vl_copy(&ring,&cursor,output,10)==3 && !memcmp(output,"abc",3));
    uint64_t saved=cursor;atomic_store(&ring.sequence,3);
    assert(!vl_copy(&ring,&cursor,output,10) && cursor==saved);atomic_store(&ring.sequence,4);
    char input[VL_RING_SIZE+7];for(unsigned i=0;i<sizeof(input);++i)input[i]=(char)i;
    vl_append(&ring,input,sizeof(input));cursor=0;
    assert(vl_copy(&ring,&cursor,output,sizeof(output))==VL_RING_SIZE);
    assert(!memcmp(output,input+7,VL_RING_SIZE));
    assert(cursor==atomic_load(&ring.total));cursor=UINT64_MAX;
    assert(vl_copy(&ring,&cursor,output,17)==17 && cursor==atomic_load(&ring.total)-VL_RING_SIZE+17);
    atomic_store(&ring.sequence,UINT64_MAX-1);vl_append(&ring,"z",1);assert(atomic_load(&ring.sequence)==0);
}
int main(void)
{
    boundaries();pthread_t thread;assert(!pthread_create(&thread,NULL,writer,NULL));
    char output[193];uint64_t cursor=0;unsigned accepted=0;
    while(!atomic_load_explicit(&done,memory_order_acquire)) {
        uint64_t before=cursor;unsigned n=vl_copy(&concurrent,&cursor,output,sizeof(output));
        if(!n){assert(cursor==before);continue;}
        assert(cursor>=before && n<=sizeof(output));uint64_t first=cursor-n;
        for(unsigned i=0;i<n;++i)assert(output[i]=='A'+((first+i)/64)%26);
        ++accepted;
    }
    assert(!pthread_join(thread,NULL));assert(atomic_load(&concurrent.total)==12800000);
    assert(accepted);puts("snapshot consistency, ring wrap and unblocked writer progress PASS");return 0;
}
