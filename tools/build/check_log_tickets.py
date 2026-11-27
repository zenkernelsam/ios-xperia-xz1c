"""Verify the multi-producer ticket prototype before any kernel integration."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--sanitizer',choices=['address,undefined','thread'],default='address,undefined');a=p.parse_args()
 out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
 def native(path):
  path=Path(path).resolve();return '/mnt/'+path.drive[0].lower()+'/'+path.as_posix()[3:] if os.name=='nt' else str(path)
 prefix=['wsl.exe','-d','Ubuntu-22.04','--'] if os.name=='nt' else []
 command=prefix+['clang','-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-pthread','-fsanitize='+a.sanitizer,'-fno-sanitize-recover=all',native(ROOT/'tools/build/test_log_tickets.c'),'-o',native(out/'test')]
 if os.name=='nt':command.insert(len(prefix)+1,'-no-pie')
 commands=[command,prefix+[native(out/'test')]]
 for name,argv in zip(('compile','test'),commands):
  r=subprocess.run(argv,capture_output=True,timeout=60);(out/(name+'.log')).write_bytes(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(name+' failed: '+r.stderr.decode('utf-8','replace')[-2000:])
 assert b'Log tickets PASS:' in (out/'test.log').read_bytes()
 files=['xnu/support/SweetLogTickets.h','tools/build/test_log_tickets.c','tools/build/check_log_tickets.py']
 report={'sources':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files},'sanitizers':a.sanitizer,'writers':8,'records':16000,'exact_bytes':256000,'paused_and_overtaken_writer_checked':True,'queue_integrated_into_kernel':False,'device_operations':0,'commands':commands}
 with (out/'manifest.json').open('x',encoding='utf-8',newline='\n') as f:json.dump(report,f,indent=2);f.write('\n')
 print(json.dumps(report))
if __name__=='__main__':main()
