"""Sanitize the actual v727 mapper and direction-selection body before installation."""
import json,subprocess,hashlib,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2];stage=root/'build/surface-readonly-v727';candidate=stage/'candidate'
tree=root/'build/dxb/xnu/iokit/Kernel/arm'
lock=json.loads((root/'reproducibility/source-lock.json').read_bytes())
for name in ('SGSharedMapper.h','SGRootTransaction.h','SGVirtualAddress.h','SGPageTables.h'):
    path=tree/name
    assert hashlib.sha256(path.read_bytes()).hexdigest()==lock['audited_xnu_files']['iokit/Kernel/arm/'+name]
    target=candidate/name
    if target.exists():assert target.read_bytes()==path.read_bytes()
    else:shutil.copy2(path,target)
def run(name,code,sources=()):
    file=stage/(name+'.cpp');file.write_bytes(code.encode());binary=stage/name
    compile_result=subprocess.run(['clang++','-std=c++17','-O1','-g','-fsanitize=address,undefined',
        '-I'+str(candidate),'-I'+str(tree),'-I'+str(root/'build/dxb/xnu/bsd/dev'),str(file),*[str(p) for p in sources],'-o',str(binary)],capture_output=True,text=True)
    (stage/(name+'-compile.log')).write_text(compile_result.stdout+compile_result.stderr)
    assert compile_result.returncode==0,compile_result.stderr
    result=subprocess.run([str(binary)],capture_output=True,text=True)
    (stage/(name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    print(result.stdout)
s=(candidate/'SweetSurface.cpp').read_text()
begin=s.index('static IOReturn sweet_caller_direction(');end=s.index('static void *sweet_caller_allocate',begin)
body=s[begin:end]
run('test_direction',r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "CallerSurfaceBacking.h"
using task_t=void*;using vm_map_t=void*;using IOReturn=int;using IODirection=int;
using vm_map_offset_t=uint64_t;using vm_map_size_t=uint64_t;using natural_t=unsigned;
using vm_address_t=uint64_t;using vm_size_t=uint64_t;using vm_region_recurse_info_64_t=void*;
using mach_msg_type_number_t=unsigned;using kern_return_t=int;
enum {kIOReturnBadArgument=1,kIOReturnNotReadable=2,kIOReturnSuccess=0,KERN_FAILURE=5,
      VM_PROT_READ=1,VM_PROT_WRITE=2,kIODirectionInOut=3,kIODirectionOut=1,VM_REGION_SUBMAP_INFO_COUNT_64=16};
struct vm_region_submap_info_data_64_t {unsigned is_submap=0,protection=0,max_protection=0;};
struct Region {uint64_t start,bytes;unsigned prot,max;};
std::vector<Region> regions;unsigned calls,mode;
vm_map_t get_task_map(task_t t){return t;}
int vm_region_recurse_64(vm_map_t,vm_address_t*a,vm_size_t*b,natural_t*,
 vm_region_recurse_info_64_t raw,mach_msg_type_number_t*c){
 auto*i=(vm_region_submap_info_data_64_t*)raw;
 ++calls;if(mode==1)return 2;if(mode==2){i->is_submap=1;return 0;}
 if(mode==3){*c=0;*b=655360;i->protection=3;i->max_protection=7;return 0;}
 for(auto&r:regions)if(r.start+r.bytes>*a){*a=r.start;*b=r.bytes;i->protection=r.prot;i->max_protection=r.max;return 0;}
 return 1;
}
''' + body + r'''
int main(){sca_range r{};IODirection d=-1;
 assert(sca_range_plan(0x112000020,640000,640000,16384,&r));
 regions={{r.storage_start,r.storage_bytes,1,1}};
 assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionOut);
 regions={{r.storage_start,16384,3,7},{r.storage_start+16384,r.storage_bytes-16384,3,7}};
 assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionInOut);
 regions[1].prot=1;assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionOut);
 regions[1].prot=0;assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 regions[1].prot=3;regions[1].start+=16384;
 assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 assert(sweet_caller_direction(nullptr,&r,&d)==kIOReturnBadArgument);
 for(mode=1;mode<=3;++mode){calls=0;assert(sweet_caller_direction((void*)1,&r,&d)!=0);assert(calls<=16);}
 mode=0;regions.clear();assert(sca_range_plan(0x100000000,65*16384,65*16384,16384,&r));
 for(unsigned n=0;n<65;++n)regions.push_back({r.storage_start+n*16384,16384,3,7});
 calls=0;assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnBadArgument&&calls==64);
 puts("actual direction body: read-only/read-write/mixed/no-read/hole/submap/error/truncation/64-region bounds PASS");
}
''')
run('test_mapper',r'''
#include "SGSharedMapper.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
struct Peer {sg_root_observation observed{};unsigned allocations=0,releases=0;bool locked=false;};
void setup(Peer&p){auto&r=p.observed.hardware;p.observed.current_live=p.observed.worker_finished=1;
 r.expected_root_pa=r.ttbr0=r.ttbr0_after=0xa17f7000;r.id0=0x48016403;r.id1=0x20008003;r.id2=0x7511;
 r.scr0=0x201e36;r.cbar0=0x1f300;r.cba2r0=1;r.sctlr=0x1167;r.tcr=0x80201c;r.tcr2=0x38011;r.mair=0x44;
 r.smr[0]=0x80000000;r.smr[2]=0x80000002;r.s2cr[2]=0x200ff;r.smr[3]=0x80000003;r.s2cr[3]=0x30000005;
 r.cp_result=3;r.sequence=r.completed=1;r.ring_read=r.ring_write=25;r.fsr=0x400;
 for(uint64_t n=2;n<8;++n)p.observed.words[n]=(n<<30)|0x641;}
int lock(void*c){auto&p=*(Peer*)c;assert(!p.locked);p.locked=true;return 1;}
void unlock(void*c){auto&p=*(Peer*)c;assert(p.locked);p.locked=false;}
int observe(void*c,sg_root_observation*out){auto&p=*(Peer*)c;assert(p.locked);*out=p.observed;return 0;}
sg_publication store(void*c,uint64_t value){auto&p=*(Peer*)c;assert(p.locked);p.observed.words[1]=value;return SG_PUBLISHED;}
int good(void*c){assert(((Peer*)c)->locked);return 0;}
int sync(void*c,unsigned time){assert(((Peer*)c)->locked&&time==10);return 0;}
int allocate(void*c,sg_pt_page*out){auto&p=*(Peer*)c;void*w=nullptr;assert(!posix_memalign(&w,16384,16384));
 ++p.allocations;*out={w,(uint64_t*)w,0xa3100000+p.allocations*UINT64_C(0x14000),16384,1};return 0;}
int dispose(void*c,sg_pt_page*p){std::free(p->owner);++((Peer*)c)->releases;return 0;}
void cache(void*,const sg_pt_page*p,unsigned bytes){assert(p->prepared&&p->owner&&bytes==4096);}
int main(){Peer p;setup(p);sg_shared_ops ops={{&p,lock,unlock,observe,store,good,good,good,sync},{&p,allocate,dispose,cache}};
 sg_shared_mapper m{};sg_shared_record records[8]{};sg_va_record va[8]{};assert(sg_shared_init(&m,records,va,8));
 sg_lease rw{},ro{};rw.state=ro.state=SG_PINNED;rw.descriptor=&rw;ro.descriptor=&ro;
 rw.pinned=ro.pinned=1;rw.generation=ro.generation=1;rw.map_bytes=ro.map_bytes=16384;ro.read_only=1;
 uint64_t a=0xb0000000,b=0xc0000000,av=0,ak=0,bv=0,bk=0;
 assert(sg_shared_map(&ops,&m,1,1,&rw,&a,1,&av,&ak)==SG_PUBLISHED);
 assert(sg_shared_map(&ops,&m,2,1,&ro,&b,1,&bv,&bk)==SG_PUBLISHED);
 for(unsigned n=0;n<4;++n){auto ra=m.l3[(av>>21)&511].words[((av+n*4096)>>12)&511];
  auto rb=m.l3[(bv>>21)&511].words[((bv+n*4096)>>12)&511];assert(!(ra&(1<<7))&&(rb&(1<<7)));}
 assert(sg_shared_unmap(&ops,&m,1,1,ak)==SG_PUBLISHED);
 assert(sg_shared_unmap(&ops,&m,2,1,bk)==SG_PUBLISHED&&m.active==0&&!m.root_visible);
 assert(p.allocations==p.releases);
 ro.read_only=2;unsigned before=p.allocations;
 assert(sg_shared_map(&ops,&m,2,1,&ro,&b,1,&bv,&bk)==SG_NOT_PUBLISHED&&p.allocations==before);
 puts("actual shared mapper: mixed permissions/every CPU subpage/paired retirement/invalid permission rejection PASS");
}
''',[candidate/'SGSharedMapper.cpp',tree/'SGVirtualAddress.cpp'])
manifest=json.loads((root/'reproducibility/surface-diag-v727.json').read_bytes())
manifest['actual_direction_and_shared_mapper_asan_ubsan_passed']=True
manifest['host_and_device_validation_pending']=True
(stage/'host-checks.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
