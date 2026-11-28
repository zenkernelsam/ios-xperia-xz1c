/* Exercise the actual include and pinned selector against readonly MMIO views. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uintptr_t vm_offset_t;
typedef struct {unsigned busy,locks,unlocks;} IOLock;
static IOLock lock;
static IOLock *sp_lock=&lock;
static vm_offset_t sp_core=0x1000,sp_cfg=0x2000,sp_obs_ch[512],sp_wr_ch[512];
static uint32_t maps[512],owners[512];
static bool sp_ready=true,sp_poisoned[512];
static unsigned sp_count=512,reads,attack;
#define SP_NO_APID 0xffffu
static void IOLockLock(IOLock *p){assert(p==&lock && !p->busy);p->busy=1;++p->locks;}
static void IOLockUnlock(IOLock *p){assert(p==&lock && p->busy);p->busy=0;++p->unlocks;}
static uint32_t sp_rd(vm_offset_t base,unsigned offset)
{
    assert(lock.busy);++reads;
    if(attack && reads==1027){
        if(attack==1)owners[11]^=1;
        if(attack==2)maps[11]=0;
        if(attack==3)sp_poisoned[11]=true;
    }
    if(base==sp_core){assert(offset>=0x800 && offset<0x1000 && !(offset&3));return maps[(offset-0x800)/4];}
    assert(base==sp_cfg && offset>=0x700 && offset<0xf00 && !(offset&3));return owners[(offset-0x700)/4];
}
#include "PinnedSpmiFind.h"
#include "../../xnu/support/LilacSpmiLaneSnapshot.inc"
static void clear(void)
{
    memset(maps,0,sizeof(maps));memset(owners,0,sizeof(owners));memset(sp_obs_ch,0,sizeof(sp_obs_ch));
    memset(sp_wr_ch,0,sizeof(sp_wr_ch));memset(sp_poisoned,0,sizeof(sp_poisoned));
    lock=(IOLock){0};reads=attack=0;sp_count=512;sp_ready=true;sp_core=0x1000;sp_cfg=0x2000;
}
static void map(unsigned apid,unsigned owner)
{maps[apid]=(0x3d8u<<8)|0x42;owners[apid]=0xa0|owner;sp_obs_ch[apid]=0x400000+apid*0x8000;sp_wr_ch[apid]=0x800000+apid*0x8000;}
static struct LilacSpmiLaneSnapshot success(void)
{
    uint32_t before_maps[512],before_owners[512];vm_offset_t before_obs[512],before_wr[512];bool before_poison[512];
    memcpy(before_maps,maps,sizeof(maps));memcpy(before_owners,owners,sizeof(owners));
    memcpy(before_obs,sp_obs_ch,sizeof(sp_obs_ch));memcpy(before_wr,sp_wr_ch,sizeof(sp_wr_ch));memcpy(before_poison,sp_poisoned,sizeof(sp_poisoned));
    struct LilacSpmiLaneSnapshot s;memset(&s,0xa5,sizeof(s));
    assert(sweet_lilac_wled_lane_snapshot(0x3d8,&s)==0 && lilac_wled_lane_snapshot_valid(&s));
    assert(!memcmp(before_maps,maps,sizeof(maps)) && !memcmp(before_owners,owners,sizeof(owners)) &&
           !memcmp(before_obs,sp_obs_ch,sizeof(sp_obs_ch)) && !memcmp(before_wr,sp_wr_ch,sizeof(sp_wr_ch)) && !memcmp(before_poison,sp_poisoned,sizeof(sp_poisoned)));
    assert(!lock.busy && lock.locks==1 && lock.unlocks==1);return s;
}
static void failure(unsigned ppid)
{
    struct LilacSpmiLaneSnapshot before,out;memset(&before,0xa5,sizeof(before));out=before;
    assert(sweet_lilac_wled_lane_snapshot(ppid,&out)==-1 && !memcmp(&before,&out,sizeof(out)) && !lock.busy);
    assert(lock.locks==lock.unlocks);
}
int main(void)
{
    _Static_assert(sizeof(struct LilacSpmiLaneSnapshot)==112,"fixed C ABI");
    clear();sp_ready=false;failure(0x3d8);assert(!reads && !lock.locks);
    clear();struct LilacSpmiLaneSnapshot s=success();assert(!s.matches && s.selected==LILAC_WLED_NO_APID);
    clear();map(11,0);s=success();assert(s.matches==1 && s.selected==11 && (s.lanes[0].owner_word&7)==0 && s.lanes[0].writer_mapped);
    clear();map(11,1);s=success();assert((s.lanes[0].owner_word&7)==1); /* Observation does not grant a write lease. */
    clear();map(11,0);map(23,1);s=success();assert(s.matches==2 && s.selected==11);
    clear();map(11,0);map(23,0);s=success();assert(s.matches==2 && s.selected==23); /* Actual selector uses last EE0. */
    clear();map(11,0);sp_poisoned[11]=true;sp_wr_ch[11]=0;s=success();assert(s.lanes[0].poisoned && !s.lanes[0].writer_mapped);
    for(unsigned mode=1;mode<=3;++mode){clear();map(11,0);attack=mode;failure(0x3d8);assert(reads>=1027 && lock.locks==1);}
    clear();for(unsigned i=0;i<5;++i)map(i,0);failure(0x3d8);
    clear();sp_count=513;failure(0x3d8);assert(!reads);
    clear();failure(0x008);assert(!reads && !lock.locks);
    clear();assert(sweet_lilac_wled_lane_snapshot(0x3d8,NULL)==-1 && !reads);
    clear();map(11,0);s=success();struct LilacSpmiLaneSnapshot bad=s;
    bad.revision=2;assert(!lilac_wled_lane_snapshot_valid(&bad));bad=s;bad.ppid=0x008;assert(!lilac_wled_lane_snapshot_valid(&bad));
    bad=s;bad.matches=5;assert(!lilac_wled_lane_snapshot_valid(&bad));bad=s;bad.selected=12;assert(!lilac_wled_lane_snapshot_valid(&bad));
    bad=s;bad.lanes[0].poisoned=2;assert(!lilac_wled_lane_snapshot_valid(&bad));bad=s;bad.lanes[0].map_word=1;assert(!lilac_wled_lane_snapshot_valid(&bad));
    bad=s;bad.matches=2;bad.lanes[1]=bad.lanes[0];assert(!lilac_wled_lane_snapshot_valid(&bad));
    puts("WLED lane snapshot host PASS: 14 state/API cases, 7 malformed snapshots, no bus mapping or writes");
    return 0;
}
