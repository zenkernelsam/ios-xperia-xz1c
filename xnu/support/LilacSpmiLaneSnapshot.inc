/* SPDX-License-Identifier: BSD-2-Clause
 * Included only by an explicitly built SPMI observation candidate. */
#include "LilacSpmiLaneSnapshot.h"

static int
sp_wled_lane_observe(unsigned ppid,struct LilacSpmiLaneSnapshot *out)
{
    struct LilacSpmiLaneSnapshot state={0};
    state.revision=1;state.ppid=ppid;state.selected=LILAC_WLED_NO_APID;
    for(unsigned i=0;i<sp_count;++i){
        uint32_t map=sp_rd(sp_core,0x800u+4u*i);
        if(!map || ((map>>8)&0xfffu)!=ppid)continue;
        if(state.matches==LILAC_WLED_LANES)return -1;
        struct LilacSpmiLane *lane=&state.lanes[state.matches++];
        lane->apid=i;lane->map_word=map;lane->owner_word=sp_rd(sp_cfg,0x700u+4u*i);
        lane->observer_mapped=sp_obs_ch[i]!=0;lane->writer_mapped=sp_wr_ch[i]!=0;
        lane->poisoned=sp_poisoned[i]?1u:0u;
    }
    state.selected=sp_find_apid(ppid);
    if(!lilac_wled_lane_snapshot_valid(&state))return -1;
    *out=state;return 0;
}

int
sweet_lilac_wled_lane_snapshot(unsigned ppid,struct LilacSpmiLaneSnapshot *out)
{
    if(!out || (ppid!=0x3d8u && ppid!=0x3d9u) || !sp_ready || !sp_lock ||
       !sp_core || !sp_cfg || !sp_count || sp_count>512u)return -1;
    struct LilacSpmiLaneSnapshot first,second;
    IOLockLock(sp_lock);
    int result=sp_wled_lane_observe(ppid,&first);
    if(!result)result=sp_wled_lane_observe(ppid,&second);
    if(!result && !lilac_wled_lane_snapshot_equal(&first,&second))result=-1;
    IOLockUnlock(sp_lock);
    if(result)return -1;
    *out=first;return 0;
}
