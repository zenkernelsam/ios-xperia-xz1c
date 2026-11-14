/* Actual readiness consumer with a real worker, failure and deadline replay. */
#include "../../gpu/diagnostics/MetalProbeStartup.h"
#include <stdatomic.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
static _Atomic uint32_t state;
static _Atomic unsigned work;
static int result;
static uint32_t current(void){return atomic_load_explicit(&state,memory_order_acquire);}
static void pause_probe(void){usleep(1000);}
static void *worker(void *unused){(void)unused;result=sweet_probe_wait_for_gles(current,pause_probe,100);
    if(!result)atomic_fetch_add_explicit(&work,1,memory_order_relaxed);return NULL;}
int main(void)
{
    pthread_t thread;atomic_store(&state,1);assert(!pthread_create(&thread,NULL,worker,NULL));usleep(5000);assert(!atomic_load(&work));
    atomic_store_explicit(&state,2,memory_order_release);assert(!pthread_join(thread,NULL));assert(result==0 && atomic_load(&work)==1);
    atomic_store(&state,3);assert(!pthread_create(&thread,NULL,worker,NULL));assert(!pthread_join(thread,NULL));assert(result==-1 && atomic_load(&work)==1);
    atomic_store(&state,0);assert(sweet_probe_wait_for_gles(current,pause_probe,2)==-2 && atomic_load(&work)==1);
    atomic_store(&state,4);assert(sweet_probe_wait_for_gles(current,pause_probe,2)==-1);
    puts("actual readiness consumer: worker returns asynchronously, exact pass gate, failure/unknown/deadline prevent GPU work PASS");return 0;
}
