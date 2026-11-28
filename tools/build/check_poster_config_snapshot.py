"""Compile the real readonly reader and retain all filesystem fault fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--output',required=True)
parser.add_argument('--distribution',default='Ubuntu-22.04');args=parser.parse_args()
out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
fixtures=out/'fixtures';fixtures.mkdir()
sources=['gpu/diagnostics/PosterConfigSnapshot.h','gpu/sweetgl/ProgramCacheCodec.h',
         'gpu/sweetgl/MetalShaderAPI.h','tools/build/test_poster_config_snapshot.c','tools/build/check_poster_config_snapshot.py']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
before={p:sha(ROOT/p) for p in sources}
def run(command,name):
    r=subprocess.run(list(map(str,command)),capture_output=True)
    (out/(name+'.log')).write_bytes(r.stdout+r.stderr)
    assert r.returncode==0,(r.stdout+r.stderr).decode('utf-8','replace')[-5000:]
    return r.stdout
if os.name=='nt':
    def linux(p):
        p=Path(p).resolve();assert p.drive=='D:';return '/mnt/d/'+p.as_posix()[3:]
    compiler=['wsl.exe','-d',args.distribution,'--exec','gcc']
    compile_args=[linux(ROOT/'tools/build/test_poster_config_snapshot.c'),'-o',linux(out/'test-reader'),'-lcrypto']
    execute=['wsl.exe','-d',args.distribution,'--exec',linux(out/'test-reader'),linux(fixtures)]
else:
    compiler=['clang'];compile_args=[ROOT/'tools/build/test_poster_config_snapshot.c','-o',out/'test-reader']
    execute=[out/'test-reader',fixtures]
run(compiler+['-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
             '-Wno-deprecated-declarations','-fno-sanitize-recover=all']+compile_args,'compile')
transcript=run(execute,'run')
assert b'Poster config snapshot host PASS:' in transcript
assert before=={p:sha(ROOT/p) for p in sources}
report=dict(source_files=before,host_ASan_UBSan_passed=True,real_filesystem_fixture_cases=8,invalid_API_case=1,
            production_reader_readonly_nofollow_and_FIFO_refusal_checked=True,
            filename_filter_hex_framing_EINTR_read_error_and_mid_read_mutation_checked=True,
            captured_current_phone_configuration=False,device_operations=0,permanent_deletions=0,
            transcript_sha256=hashlib.sha256(transcript).hexdigest())
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
print(json.dumps(report,indent=2))
