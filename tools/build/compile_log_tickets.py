"""Compile the isolated logger with the exact retained BSD ARM64 recipe."""
import argparse,hashlib,json,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from diagnostic_candidate import audit,WLED_KERNEL_SHA
LOG_SHA='deab7db0f6253ed1ea1c584b6cd6314446d91b93d8214c620c8423d630b5bb59'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--candidate',required=True);a=p.parse_args();root=Path(a.candidate).resolve()
 assert root.is_relative_to(ROOT/'build');target=root/'logger.arm64.o';report_path=root/'logger-arm64-manifest.json';assert not target.exists() and not report_path.exists()
 before=audit(731,WLED_KERNEL_SHA);overlay=json.loads((root/'overlay/manifest.json').read_bytes())
 for name,h in overlay['overlay_sources'].items():assert sha(root/'overlay/overlay'/name)==h
 log=ROOT/'build/surface-diag-v729/build.log';assert sha(log)==LOG_SHA
 lines=[line for line in log.read_text().splitlines() if line.startswith('/Applications/') and 'vermeer_debug.c' in line];assert len(lines)==1
 command=shlex.split(lines[0]);source=root/'overlay/overlay/bsd/dev/vermeer_debug.c';command[-1]=str(source)
 command[command.index('-MF')+1]=str(root/'logger.d');command+=['-o',str(target)]
 cwd=ROOT/'build/dxb/build/xnu.obj/DEVELOPMENT_ARM64_SWEET/bsd/DEVELOPMENT'
 run=subprocess.run(command,cwd=cwd,capture_output=True,timeout=60);(root/'logger-compile.stdout').write_bytes(run.stdout);(root/'logger-compile.stderr').write_bytes(run.stderr);assert run.returncode==0,run.stderr.decode()
 description=subprocess.check_output(['file',str(target)]).decode();symbols=subprocess.check_output(['nm','-u',str(target)]).decode();assert 'Mach-O 64-bit object arm64' in description
 assert all(line.split()[-1]=='_printf' for line in symbols.splitlines() if line.strip()),'Unexpected allocator/atomic/VM/runtime import'
 assert audit(731,WLED_KERNEL_SHA)==before
 record={'source_sha256':sha(source),'header_sha256':sha(source.parent/'SweetLogTickets.h'),'object_sha256':sha(target),'compile_command':command,'actual_kernel_headers':True,'undefined_symbols':symbols.splitlines(),'candidate_kernel_not_linked':True,'device_operations':0}
 with report_path.open('x',encoding='utf-8',newline='\n') as f:json.dump(record,f,indent=2);f.write('\n')
 print(json.dumps({k:v for k,v in record.items() if k!='compile_command'}))
if __name__=='__main__':main()
