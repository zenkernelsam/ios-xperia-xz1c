#include "SGSharedMapper.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
struct Peer {sg_root_observation observed{};unsigned allocations=0,releases=0,stores=0,syncs=0,fail_sync=0;bool locked=false;};
static void setup(Peer &p){auto &r=p.observed.hardware;
    p.observed.current_live=p.observed.worker_finished=1;r.expected_root_pa=r.ttbr0=r.ttbr0_after=0xa17f7000;
    r.id0=0x48016403;r.id1=0x20008003;r.id2=0x7511;r.scr0=0x201e36;r.cbar0=0x1f300;r.cba2r0=1;
    r.sctlr=0x1167;r.tcr=0x80201c;r.tcr2=0x38011;r.mair=0x44;r.smr[0]=0x80000000;
    r.smr[2]=0x80000002;r.s2cr[2]=0x200ff;r.smr[3]=0x80000003;r.s2cr[3]=0x30000005;
    r.cp_result=3;r.sequence=r.completed=1;r.ring_read=r.ring_write=25;r.fsr=0x400;
    for(uint64_t i=2;i<8;++i)p.observed.words[i]=(i<<30)|0x641;
}
static int lock(void *c){auto &p=*static_cast<Peer *>(c);assert(!p.locked);p.locked=true;return 1;}
static void unlock(void *c){auto &p=*static_cast<Peer *>(c);assert(p.locked);p.locked=false;}
static int observe(void *c,sg_root_observation *out){auto &p=*static_cast<Peer *>(c);assert(p.locked);*out=p.observed;return 0;}
static sg_publication store(void *c,uint64_t value){auto &p=*static_cast<Peer *>(c);assert(p.locked);++p.stores;p.observed.words[1]=value;return SG_PUBLISHED;}
static int good(void *c){assert(static_cast<Peer *>(c)->locked);return 0;}
static int sync(void *c,unsigned timeout){auto &p=*static_cast<Peer *>(c);assert(p.locked&&timeout==10);++p.syncs;return p.fail_sync==p.syncs?-1:0;}
static int allocate(void *c,sg_pt_page *out){auto &p=*static_cast<Peer *>(c);void *words=nullptr;assert(posix_memalign(&words,16384,16384)==0);++p.allocations;*out={words,static_cast<uint64_t *>(words),0xa3100000+p.allocations*UINT64_C(0x14000),16384,1};return 0;}
static int dispose(void *c,sg_pt_page *page){auto &p=*static_cast<Peer *>(c);std::free(page->owner);++p.releases;return 0;}
static void cache(void *,const sg_pt_page *page,unsigned bytes){assert(page->owner&&page->prepared&&bytes==4096);}
static sg_shared_ops ops(Peer &p){return {{&p,lock,unlock,observe,store,good,good,good,sync},{&p,allocate,dispose,cache}};}
struct Data {sg_lease lease{};std::vector<uint64_t> pages;uint64_t va=0,key=0;explicit Data(size_t count,uint64_t base):pages(count){
    lease.state=SG_PINNED;lease.descriptor=this;lease.pinned=1;lease.generation=1;lease.map_bytes=count*UINT64_C(16384);
    for(size_t i=0;i<count;++i)pages[i]=base+i*65536;
}};
static uint64_t translated(const sg_shared_mapper &m,uint64_t va){
    auto &l3=m.l3[(va>>21)&511];assert(l3.owner);return l3.words[(va>>12)&511]&UINT64_C(0x0000fffffffff000);
}
static void fixture_cleanup(sg_shared_mapper &m){if(m.l2.owner)std::free(m.l2.owner);for(auto &p:m.l3)if(p.owner)std::free(p.owner);}
int main(){
    {
        Peer p;setup(p);auto o=ops(p);sg_shared_mapper m{};sg_shared_record records[16]{};sg_va_record va[16]{};
        assert(sg_shared_init(&m,records,va,16));Data a(320,0xb0000000),b(768,0xc0000000),c(1,0xd0000000);
        assert(sg_shared_map(&o,&m,1,1,&a.lease,a.pages.data(),a.pages.size(),&a.va,&a.key)==SG_PUBLISHED);
        assert(a.va==0x40000000&&a.key==1&&m.active==1&&p.stores==1);
        for(size_t leaf=0;leaf<a.pages.size()*4;++leaf)assert(translated(m,a.va+leaf*4096)==a.pages[leaf/4]+(leaf%4)*4096);
        assert(sg_shared_map(&o,&m,2,1,&b.lease,b.pages.data(),b.pages.size(),&b.va,&b.key)==SG_PUBLISHED);
        assert(b.va==0x40500000&&b.key==2&&m.active==2&&p.stores==1);
        /* Both BOs share the partially filled third L3 at the5MiB boundary. */
        assert(m.used_leaves[2]==512&&sg_shared_tables_valid(&m));
        for(size_t leaf=0;leaf<a.pages.size()*4;++leaf)assert(translated(m,a.va+leaf*4096)==a.pages[leaf/4]+(leaf%4)*4096);
        for(size_t leaf=0;leaf<b.pages.size()*4;++leaf)assert(translated(m,b.va+leaf*4096)==b.pages[leaf/4]+(leaf%4)*4096);
        assert(sg_shared_contains(&m,2,1,b.key,b.va,b.lease.map_bytes));
        assert(!sg_shared_contains(&m,1,1,b.key,b.va,4));
        assert(sg_shared_unmap(&o,&m,2,1,a.key)==SG_PUBLICATION_UNKNOWN&&m.state==SG_SHARED_READY);
        assert(sg_shared_unmap(&o,&m,1,1,a.key)==SG_PUBLISHED&&m.active==1&&m.used_leaves[2]==256&&p.stores==1);
        for(size_t leaf=0;leaf<b.pages.size()*4;++leaf)assert(translated(m,b.va+leaf*4096)==b.pages[leaf/4]+(leaf%4)*4096);
        assert(sg_shared_map(&o,&m,3,1,&c.lease,c.pages.data(),1,&c.va,&c.key)==SG_PUBLISHED&&c.va==a.va&&c.key==3);
        assert(sg_shared_unmap(&o,&m,3,1,c.key)==SG_PUBLISHED);
        assert(sg_shared_unmap(&o,&m,2,1,b.key)==SG_PUBLISHED&&m.active==0&&p.observed.words[1]==0&&p.stores==2);
        assert(p.releases==p.allocations&&sg_shared_tables_valid(&m)&&!p.locked);
        assert(sg_shared_map(&o,&m,1,1,&a.lease,a.pages.data(),a.pages.size(),&a.va,&a.key)==SG_PUBLISHED&&a.key==4);
        assert(sg_shared_unmap(&o,&m,1,1,a.key)==SG_PUBLISHED&&p.releases==p.allocations);
    }
    for(unsigned failure=1;failure<=2;++failure){
        Peer p;setup(p);auto o=ops(p);sg_shared_mapper m{};sg_shared_record records[8]{};sg_va_record va[8]{};assert(sg_shared_init(&m,records,va,8));
        Data a(320,0xb0000000),b(768,0xc0000000);p.fail_sync=failure;
        auto result=sg_shared_map(&o,&m,1,1,&a.lease,a.pages.data(),a.pages.size(),&a.va,&a.key);
        if(failure==1)assert(result==SG_PUBLICATION_UNKNOWN&&m.state==SG_SHARED_QUARANTINED&&a.lease.state==SG_QUARANTINED);
        else {assert(result==SG_PUBLISHED);assert(sg_shared_unmap(&o,&m,1,1,a.key)==SG_PUBLICATION_UNKNOWN&&m.state==SG_SHARED_QUARANTINED);}
        unsigned calls=p.allocations;assert(sg_shared_map(&o,&m,2,1,&b.lease,b.pages.data(),b.pages.size(),&b.va,&b.key)==SG_NOT_PUBLISHED&&p.allocations==calls&&!p.releases);
        fixture_cleanup(m);
    }
    puts("SG shared mapper: full5/12MiB concurrent owners, shared partial L3 preserve/unmap, global VA ABA/nonreuse, every leaf, root lifetime, TLB failure quarantine and no premature descriptor release passed (host only)");
}
