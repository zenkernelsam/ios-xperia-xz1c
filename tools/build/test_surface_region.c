#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "SurfaceRegionObserve.h"
int main(void) {
    struct sro_span p; uint64_t n;
    assert(sro_plan(0x112000020,640000,&p));
    assert(p.start==0x112000000 && p.end-p.start==655360);
    assert(sro_advance(&p,p.start,p.start,16384,&n) && n==p.start+16384);
    assert(sro_advance(&p,n,p.start,655360,&n) && n==p.end);
    assert(!sro_advance(&p,p.start,p.start+16384,655360,&n));
    assert(!sro_advance(&p,p.start,UINT64_MAX-7,16,&n));
    assert(!sro_plan(UINT64_MAX-32768,640000,&p));
    assert(!sro_plan(1,0,&p)); assert(!sro_plan(0,1,&p));
    assert(!sro_plan(1,(UINT64_C(256)<<20)+1,&p));
    assert(sro_plan(0x100000080,4988928,&p));
    assert(p.end-p.start==4997120);
    puts("region coverage: offset, multi-region, hole and overflow PASS");
}
