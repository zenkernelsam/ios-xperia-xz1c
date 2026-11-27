"""Relink exact v731 objects, then replace only the console logger object."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(Path(__file__).parent))
from diagnostic_candidate import audit,WLED_KERNEL_SHA,WLED_BUILD
from link_lilac_wled_candidate import uuid_bounds
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--candidate',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 candidate=Path(a.candidate).resolve();out=Path(a.output).resolve();assert candidate.is_relative_to(ROOT/'build') and out.is_relative_to(ROOT/'build') and not out.exists()
 assert (Path.home()/'xnu-autobuild.pause').is_file();before=audit(731,WLED_KERNEL_SHA)
 base=ROOT/WLED_BUILD;proof=json.loads((base/'link-proof.json').read_bytes());args=json.loads((base/'wled-link.argv.json').read_bytes())
 inputs=(base/'wled.link.filelist').read_text().splitlines();assert len(inputs)==len(set(inputs))
 objroot=ROOT/'build/dxb/build/xnu.obj/DEVELOPMENT_ARM64_SWEET';logger=objroot/'bsd/DEVELOPMENT/vermeer_debug.o';assert inputs.count(str(logger))==1
 hashes=proof['original_link_inputs_sha256'].copy();hashes[str(base/'SweetFramebuffer.cpo')]=proof['candidate_framebuffer_object_sha256']
 for name in inputs:assert sha(Path(name))==hashes[name],name
 for name in ('version.o','lastkerneldataconst.o','lastkernelconstructor.o','nonlto.o'):
  path=objroot/name;assert sha(path)==hashes[str(path)],name
 overlay=json.loads((candidate/'overlay/manifest.json').read_bytes());compiled=json.loads((candidate/'logger-arm64-manifest.json').read_bytes());newobj=candidate/'logger.arm64.o'
 assert compiled['object_sha256']==sha(newobj)
 for name,digest in overlay['overlay_sources'].items():assert sha(candidate/'overlay/overlay'/name)==digest
 assert overlay['RPC_callback_tail_exact_byte_equal'] and overlay['PON_GPU_allocator_SCM_VM_UFS_sources_not_modified']
 out.mkdir()
 def run(argv,name):
  with (out/(name+'.argv.json')).open('x') as f:json.dump(argv,f,indent=2);f.write('\n')
  with (out/(name+'.log')).open('xb') as f:subprocess.run(argv,cwd=objroot,stdout=f,stderr=subprocess.STDOUT,timeout=300,check=True)
 def target(argv,path):
  result=argv.copy();result[result.index('-o')+1]=str(path);return result
 replay=out/'baseline-v731.macho';run(target(args,replay),'baseline-link')
 original=(base/'kernel.wled-observer.macho').read_bytes();rebuilt=replay.read_bytes();start,end=uuid_bounds(original)
 assert uuid_bounds(rebuilt)==(start,end) and original[:start]==rebuilt[:start] and original[end:]==rebuilt[end:],'v731 replay changed outside UUID'
 listing=out/'logger.link.filelist';listing.write_text('\n'.join(str(newobj) if name==str(logger) else name for name in inputs)+'\n')
 kernel=out/'kernel.log-tickets.macho';linked=target(args,kernel);linked[linked.index('-filelist')+1]=str(listing);run(linked,'candidate-link')
 for name,digest in hashes.items():assert sha(Path(name))==digest,name
 assert audit(731,WLED_KERNEL_SHA)==before
 report={'base_kernel_sha256':WLED_KERNEL_SHA,'baseline_relink_sha256':sha(replay),'baseline_exact_except_UUID':True,'original_input_count':len(inputs),'original_link_inputs_sha256':hashes,
  'only_replaced_object':str(logger),'old_logger_object_sha256':hashes[str(logger)],'new_logger_object_sha256':sha(newobj),
  'overlay_manifest_sha256':sha(candidate/'overlay/manifest.json'),'candidate_kernel_sha256':sha(kernel),'candidate_kernel_bytes':kernel.stat().st_size,
  'frozen_sources_and_objects_preserved':True,'logging_only':True,'candidate_not_booted':True,'device_operations':0}
 with (out/'manifest.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
 print(json.dumps({k:v for k,v in report.items() if k!='original_link_inputs_sha256'}))
if __name__=='__main__':main()
