"""Replay actual XNU old/new wrapper bodies and the real v728 direction selector."""
import hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2];stage=root/'build/surface-readonly-v728'
tree=root/'build/dxb/xnu';source=tree/'osfmk/vm/vm_user.c'
def extract(text,name):
    start=text.index('\n'+name+'(')+1;brace=text.index('{',start);depth=1;at=brace+1
    while depth:
        if text[at]=='{':depth+=1
        elif text[at]=='}':depth-=1
        at+=1
    return 'kern_return_t\n'+text[start:at]+'\n'
wrappers=extract(source.read_text(),'mach_vm_region_recurse')+extract(source.read_text(),'vm_region_recurse_64')
limits=tree/'osfmk/mach/arm/vm_param.h'
assert '0x00000000F0000000ULL' in limits.read_text()
s=(stage/'candidate/SweetSurface.cpp').read_text()
start=s.index('static IOReturn sweet_caller_direction(');end=s.index('static void *sweet_caller_allocate',start)
body=s[start:end]
old=(root/'build/surface-readonly-v728/before/SweetSurface.cpp').read_text()
v727=json.loads((root/'reproducibility/surface-diag-v727.json').read_bytes())
assert hashlib.sha256(old.encode()).hexdigest()==v727['overrides']['iokit/Kernel/arm/SweetSurface.cpp']
old_start=old.index('static IOReturn sweet_caller_direction(');old_end=old.index('static void *sweet_caller_allocate',old_start)
old_body=old[old_start:old_end].replace('sweet_caller_direction','old_direction')
harness=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "CallerSurfaceBacking.h"
using task_t=void*;using vm_map_t=void*;using IOReturn=int;using IODirection=int;
using vm_map_offset_t=uint64_t;using vm_map_address_t=uint64_t;using vm_map_size_t=uint64_t;
using vm_address_t=uint64_t;using vm_size_t=uint64_t;using mach_vm_address_t=uint64_t;using mach_vm_size_t=uint64_t;
using natural_t=uint32_t;using mach_msg_type_number_t=uint32_t;using kern_return_t=int;
struct vm_region_submap_info_data_64_t {unsigned is_submap=0,protection=0,max_protection=0;};
using vm_region_submap_info_64_t=vm_region_submap_info_data_64_t*;
using vm_region_recurse_info_t=void*;using vm_region_recurse_info_64_t=void*;
enum {kIOReturnBadArgument=1,kIOReturnNotReadable=2,kIOReturnSuccess=0,KERN_FAILURE=5,
      KERN_INVALID_ARGUMENT=4,KERN_INVALID_ADDRESS=1,KERN_SUCCESS=0,
      VM_PROT_READ=1,VM_PROT_WRITE=2,kIODirectionInOut=3,kIODirectionOut=1,VM_REGION_SUBMAP_INFO_COUNT_64=16};
constexpr uint64_t VM_MAX_ADDRESS=0x00000000F0000000ULL;
#define VM_MAP_NULL nullptr
#define CAST_DOWN(type,x) static_cast<type>(x)
struct Region {uint64_t start,bytes;unsigned prot,max;};
std::vector<Region> regions;unsigned calls=0,mode=0;
vm_map_t get_task_map(task_t t){return t;}
int vm_map_region_recurse_64(vm_map_t,vm_map_offset_t*a,vm_map_size_t*b,natural_t*,
 vm_region_submap_info_64_t i,mach_msg_type_number_t*c){
 ++calls;if(mode==1)return 2;if(mode==2){i->is_submap=1;return 0;}
 if(mode==3){*c=0;*b=655360;i->protection=3;i->max_protection=7;return 0;}
 for(auto&r:regions)if(r.start+r.bytes>*a){*a=r.start;*b=r.bytes;i->protection=r.prot;i->max_protection=r.max;return 0;}
 return 1;
}
''' + wrappers + old_body + body + r'''
int main(){sca_range r{};IODirection d=-1;
 assert(sca_range_plan(0x10cf94080,4988928,4988928,16384,&r));
 regions={{r.storage_start,r.storage_bytes,3,7}};
 assert(old_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionInOut);
 assert(sca_range_plan(0x112000020,640000,640000,16384,&r));
 regions={{r.storage_start,r.storage_bytes,1,1}};
 assert(old_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionOut);
 assert(sca_range_plan(0x10000020,640000,640000,16384,&r));
 regions={{r.storage_start,r.storage_bytes,3,7}};
 assert(old_direction((void*)1,&r,&d)==0&&sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionInOut);
 regions={{r.storage_start,16384,3,7},{r.storage_start+16384,r.storage_bytes-16384,1,1}};
 assert(sweet_caller_direction((void*)1,&r,&d)==0&&d==kIODirectionOut);
 regions[1].prot=0;assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 regions[1].prot=3;regions[1].start+=16384;assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnNotReadable);
 for(mode=1;mode<=3;++mode){calls=0;assert(sweet_caller_direction((void*)1,&r,&d)!=0);assert(calls<=16);}
 assert(sweet_caller_direction(nullptr,&r,&d)==kIOReturnBadArgument);
 mode=0;regions.clear();assert(sca_range_plan(0x100000000,65*16384,65*16384,16384,&r));
 for(unsigned n=0;n<65;++n)regions.push_back({r.storage_start+n*16384,16384,3,7});
 calls=0;assert(sweet_caller_direction((void*)1,&r,&d)==kIOReturnBadArgument&&calls==64);
 puts("actual XNU wrappers: v727 high-address failure reproduced; v728 readonly/readwrite/high/low/mixed/error bounds PASS");
}
'''
file=stage/'test_wrappers.cpp';file.write_bytes(harness.encode());binary=stage/'test_wrappers'
compiled=subprocess.run(['clang++','-std=c++17','-O1','-g','-fsanitize=address,undefined',
    '-I'+str(tree/'iokit/Kernel/arm'),str(file),'-o',str(binary)],capture_output=True,text=True)
(stage/'host-compile.log').write_text(compiled.stdout+compiled.stderr)
assert compiled.returncode==0,compiled.stderr
result=subprocess.run([str(binary)],capture_output=True,text=True)
(stage/'host-test.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
manifest=json.loads((root/'reproducibility/surface-diag-v728.json').read_bytes())
proof=dict(patch_sha256=manifest['patch_sha256'],actual_xnu_wrapper_bodies_sha256=hashlib.sha256(wrappers.encode()).hexdigest(),
    vm_user_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),actual_old_failure_reproduced=True,
    actual_new_direction_and_wrapper_asan_ubsan_passed=True,full_goal_not_complete=True)
(stage/'host-checks.json').write_bytes((json.dumps(proof,indent=2)+'\n').encode());print(result.stdout)
