"""Prepare an isolated v731-based logger overlay; never install, link or boot."""
import argparse,hashlib,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from diagnostic_candidate import configuration,audit,WLED_KERNEL_SHA
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--tree',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 tree=Path(a.tree).resolve();out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists()
 proof=audit(731,WLED_KERNEL_SHA);config=configuration(731)
 old=(tree/'bsd/dev/vermeer_debug.c').read_bytes();header=(tree/'bsd/dev/vermeer_log_ring.h').read_bytes()
 assert sha(old)==config['source_hashes']['bsd/dev/vermeer_debug.c']
 assert sha(header)==config['source_hashes']['bsd/dev/vermeer_log_ring.h']
 text=old.decode();assert '\r' not in text
 old_append='''    if (size<=0) return;
    if (!os_atomic_cmpxchg(&log_lock,0,1,acquire)) {
        os_atomic_add(&dropped,(uint64_t)size,relaxed); return;
    }
    vl_append(&log_ring,data,(unsigned)size);
    os_atomic_store(&log_lock,0,release);'''
 new_append='''    if (size<=0) return;
    unsigned published=lt_append(&log_ring,data,(unsigned)size);
    if(published<(unsigned)size)
        os_atomic_add(&dropped,(uint64_t)((unsigned)size-published),relaxed);'''
 edits={
  '#include "vermeer_log_ring.h"':'#include "SweetLogTickets.h"',
  'static struct vl_ring log_ring;':'static struct sweet_log_tickets log_ring;',
  'static uint32_t _Atomic log_lock;\n':'',
  old_append:new_append,
  'return vl_copy(&log_ring,pos,out,max);':'return lt_copy(&log_ring,pos,out,max);',
  'return atomic_load_explicit(&log_ring.total,memory_order_acquire);':'return lt_reserved_total(&log_ring);',
 }
 for before,after in edits.items():
  assert text.count(before)==1,before;text=text.replace(before,after,1)
 tail='/* The optional GUI sampler';assert old.decode().split(tail,1)[1]==text.split(tail,1)[1]
 assert 'uint64_t vermeer_debug_dropped(void) { return os_atomic_load(&dropped,relaxed); }' in text
 output=out/'overlay/bsd/dev';output.mkdir(parents=True)
 (output/'vermeer_debug.c').write_bytes(text.encode())
 ticket=(ROOT/'xnu/support/SweetLogTickets.h').read_bytes();(output/'SweetLogTickets.h').write_bytes(ticket)
 report={'base_version':731,'base_kernel_sha256':WLED_KERNEL_SHA,'base_actual_audit':proof,
  'before_sources':{'bsd/dev/vermeer_debug.c':sha(old),'bsd/dev/vermeer_log_ring.h':sha(header)},
  'overlay_sources':{'bsd/dev/vermeer_debug.c':sha(text.encode()),'bsd/dev/SweetLogTickets.h':sha(ticket)},
  'helper_source':'xnu/support/SweetLogTickets.h','generator_source_sha256':sha(Path(__file__).read_bytes()),
  'RPC_callback_tail_exact_byte_equal':True,'PON_GPU_allocator_SCM_VM_UFS_sources_not_modified':True,
  'static_cell_storage_bytes':4*(1<<20),'only_logger_source_and_new_header_in_overlay':True,
  'original_tree_not_modified':True,'candidate_not_linked':True,'candidate_not_installed':True,'device_operations':0}
 with (out/'manifest.json').open('x',encoding='utf-8',newline='\n') as f:json.dump(report,f,indent=2);f.write('\n')
 print(json.dumps({'overlay_sources':report['overlay_sources'],'RPC_callbacks_unchanged':True,'original_tree_not_modified':True}))
if __name__=='__main__':main()
