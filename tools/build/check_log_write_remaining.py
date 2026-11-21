"""Exercise the actual bounded process logger write helper with I/O faults."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from check_depth_stencil_gles import ROOT,linux,bounded_linux_test

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu');a=p.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['gpu/sweetgl/LogWriteAll.h','tools/build/test_log_write_remaining.c','tools/build/check_log_write_remaining.py']
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    cmd=['wsl.exe','-d',a.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/paths[1]),'-o',linux(out/'test')]
    r=subprocess.run(cmd,capture_output=True,timeout=55);(out/'compile.log').write_bytes(r.stdout+r.stderr);assert r.returncode==0,(r.stdout+r.stderr)[-2000:]
    data=bounded_linux_test(a.distribution,out,'test',['UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    assert b'Process log writer PASS cases=10 exact_remaining_bytes=1 EINTR_and_partial=1 bounded_interruption=1 device_operations=0' in data
    assert sources=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    report=dict(source_files=sources,cases=10,UBSan_passed=True,actual_write_remaining_helper_fault_test_passed=True,
                exact_once_bytes_with_EINTR_and_partial_writes=True,bounded_repeated_interruption=True,
                host_only_NOT_live_serial_return_or_GPU_proof=True,device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
