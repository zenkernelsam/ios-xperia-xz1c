"""Validate concurrent logging snapshots before installing the logging-only delta."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);args=parser.parse_args()
out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/'tools/build/test_console_snapshot.c';header=root/'xnu/support/vermeer_log_ring.h'
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    return result.stdout
for sanitizer,name in [('address,undefined','asan-ubsan'),('thread','tsan')]:
    run(['clang','-std=c11','-O1','-g','-fsanitize='+sanitizer,'-pthread',source,'-o',out/name],name+'-compile')
    assert 'PASS' in run([out/name],name+'-test')
run(['xcrun','--sdk','macosx','clang','-arch','arm64','-std=c11','-O2','-c',source,'-o',out/'arm64.o'],'arm64-compile')
report=dict(header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),
    test_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),snapshot_boundaries_and_concurrency_ASan_UBSan_TSan_passed=True,
    ARM64_compile_passed=True,reader_does_not_hold_writer_lock=True,actual_device_acceptance_pending=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
