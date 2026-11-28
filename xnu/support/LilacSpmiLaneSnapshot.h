/* SPDX-License-Identifier: BSD-2-Clause
 * WLED lane observations only. No epoch, lease or write authorization. */
#pragma once
#include <stddef.h>
#include <stdint.h>
enum { LILAC_WLED_LANES=4, LILAC_WLED_NO_APID=0xffff };
struct LilacSpmiLane {
    uint32_t apid,map_word,owner_word,observer_mapped,writer_mapped,poisoned;
};
struct LilacSpmiLaneSnapshot {
    uint32_t revision,ppid,matches,selected;
    struct LilacSpmiLane lanes[LILAC_WLED_LANES];
};
static inline int lilac_wled_lane_snapshot_valid(const struct LilacSpmiLaneSnapshot *s)
{
    if(!s || s->revision!=1 || (s->ppid!=0x3d8 && s->ppid!=0x3d9) || s->matches>LILAC_WLED_LANES)return 0;
    if(!s->matches)return s->selected==LILAC_WLED_NO_APID;
    unsigned selected=0;
    for(unsigned i=0;i<s->matches;++i){
        const struct LilacSpmiLane *p=&s->lanes[i];
        if(p->apid>=512 || !p->map_word || ((p->map_word>>8)&0xfff)!=s->ppid ||
           p->observer_mapped>1 || p->writer_mapped>1 || p->poisoned>1)return 0;
        selected+=p->apid==s->selected;
        for(unsigned j=0;j<i;++j)if(s->lanes[j].apid==p->apid)return 0;
    }
    return selected==1;
}
static inline int lilac_wled_lane_snapshot_equal(const struct LilacSpmiLaneSnapshot *a,const struct LilacSpmiLaneSnapshot *b)
{
    if(a->revision!=b->revision || a->ppid!=b->ppid || a->matches!=b->matches || a->selected!=b->selected)return 0;
    for(unsigned i=0;i<LILAC_WLED_LANES;++i){
        const struct LilacSpmiLane *x=&a->lanes[i],*y=&b->lanes[i];
        if(x->apid!=y->apid || x->map_word!=y->map_word || x->owner_word!=y->owner_word ||
           x->observer_mapped!=y->observer_mapped || x->writer_mapped!=y->writer_mapped || x->poisoned!=y->poisoned)return 0;
    }
    return 1;
}
#ifdef __cplusplus
extern "C" {
#endif
/* Caller provides ordinary writable kernel memory. Failure leaves it untouched.
 * Success describes two equal observations under the existing lock. Firmware
 * ownership can still change afterwards; this is not a retained IO lease. */
int sweet_lilac_wled_lane_snapshot(unsigned ppid,struct LilacSpmiLaneSnapshot *out);
#ifdef __cplusplus
}
#endif
