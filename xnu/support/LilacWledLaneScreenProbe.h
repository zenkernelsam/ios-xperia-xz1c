/* SPDX-License-Identifier: BSD-2-Clause
 * Called only after the existing completed first-frame WLED sample. */
#pragma once
#include "LilacSpmiLaneSnapshot.h"
static bool lilac_wled_lanes_emitted;
static bool lilac_wled_lane_token_valid(const char *token)
{
    if(!token)return false;
    for(unsigned i=0;i<16;++i)if(!((token[i]>='0' && token[i]<='9') || (token[i]>='a' && token[i]<='f')))return false;
    return token[16]==0;
}
static void lilac_wled_lane_screen_probe(const char *token,uint32_t frame_id)
{
    if(frame_id!=1 || !vermeer_loop_live_ready() || !lilac_wled_lane_token_valid(token) || lilac_wled_lanes_emitted)return;
    lilac_wled_lanes_emitted=true;
    sweet_screen_record("SWEET WLED LANES BEGIN token=%s id=1 revision=1 roots=2 read_only=1",token);
    unsigned failures=0;
    for(unsigned root=0;root<2;++root){
        if(!vermeer_loop_live_ready())return;
        unsigned ppid=root?0x3d9u:0x3d8u;
        struct LilacSpmiLaneSnapshot state={};
        int result=sweet_lilac_wled_lane_snapshot(ppid,&state);
        if(!vermeer_loop_live_ready())return;
        if(!result && (!lilac_wled_lane_snapshot_valid(&state) || state.ppid!=ppid))result=-1;
        if(result)++failures;
        sweet_screen_record("SWEET WLED LANES STATE token=%s id=1 root=%u ppid=%03x result=%d matches=%u selected=%u",
            token,root,ppid,result,result?0u:state.matches,result?(unsigned)LILAC_WLED_NO_APID:state.selected);
        if(!result)for(unsigned i=0;i<state.matches;++i){
            if(!vermeer_loop_live_ready())return;
            const struct LilacSpmiLane *lane=&state.lanes[i];
            sweet_screen_record("SWEET WLED LANES ITEM token=%s id=1 root=%u index=%u apid=%u map=%08x owner=%08x obs=%u wr=%u poison=%u",
                token,root,i,lane->apid,lane->map_word,lane->owner_word,lane->observer_mapped,lane->writer_mapped,lane->poisoned);
        }
    }
    if(vermeer_loop_live_ready())sweet_screen_record(
        "SWEET WLED LANES END token=%s id=1 result=%u roots=2 epoch=0 write_lease=0",token,failures?1u:0u);
}
