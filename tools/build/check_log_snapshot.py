"""Reproduce the locked old CDC mirror's reader collision and test a new helper."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
def sha(data):return hashlib.sha256(data).hexdigest()
def write(path,text):
    with path.open('x',encoding='utf-8',newline='\n') as stream:stream.write(text)
def added(patch,path):
    prefix=('a/'+path+' b/'+path+'\n').encode();sections=[s for s in patch.split(b'diff --git ') if s.startswith(prefix)]
    assert len(sections)==1 and b'--- /dev/null\n' in sections[0]
    return b''.join(line[1:] for line in sections[0].splitlines(keepends=True) if line.startswith(b'+') and not line.startswith(b'+++'))
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--sanitizer',choices=['address,undefined','thread'],default='address,undefined');a=parser.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    patch=(ROOT/'xnu/lilac-current.patch').read_bytes();lock=json.loads((ROOT/'reproducibility/source-lock.json').read_bytes());assert sha(patch)==lock['xnu']['patch_sha256']
    original=added(patch,'bsd/dev/vermeer_debug.c');header=added(patch,'bsd/dev/vermeer_log_ring.h')
    # The deployed v730/v731 kernel uses the formal v729 increment, rather
    # than the older base-patch logger. Reconstruct and test both explicitly.
    tree=out/'deployed-tree';(tree/'bsd/dev').mkdir(parents=True)
    (tree/'bsd/dev/vermeer_debug.c').write_bytes(original)
    (tree/'bsd/dev/vermeer_log_ring.h').write_bytes(header)
    increment=ROOT/'xnu/lilac-console-snapshot.patch';spec=json.loads((ROOT/'reproducibility/surface-diag-v729.json').read_bytes())
    assert sha(increment.read_bytes())==spec['patch_sha256']
    for flag in ('--check',None):
        args=['git','-c','core.autocrlf=false','-c','core.eol=lf','apply']+([flag] if flag else [])+[str(increment)]
        env=dict(os.environ,GIT_CEILING_DIRECTORIES=str(out))
        result=subprocess.run(args,cwd=tree,env=env,capture_output=True,timeout=20)
        (out/('v729-check.log' if flag else 'v729-apply.log')).write_bytes(result.stdout+result.stderr)
        assert result.returncode==0,result.stderr
    deployed={p:sha((tree/p).read_bytes()) for p in ('bsd/dev/vermeer_debug.c','bsd/dev/vermeer_log_ring.h')}
    assert all(deployed[p]==spec['overrides'][p] for p in deployed)
    current_dir=out/'current-prefix';current_dir.mkdir()
    current=(tree/'bsd/dev/vermeer_debug.c').read_text().split('/* The optional GUI sampler')[0]
    current='#define vermeer_debug_append current_append\n#define vermeer_debug_copy current_copy\n#define vermeer_debug_total current_total\n#define vermeer_debug_dropped current_dropped\n'+current
    write(current_dir/'current.c',current)
    actual=(tree/'bsd/dev/vermeer_log_ring.h').read_text();point='    atomic_fetch_add_explicit(&r->sequence,1,memory_order_acq_rel);';assert actual.count(point)==1
    actual=actual.replace(point,point+'\n    extern void log_test_writer_hook(void);log_test_writer_hook();')
    write(current_dir/'vermeer_log_ring.h',actual)
    marker=b'/* The optional GUI sampler';assert original.count(marker)==1
    core=original.split(marker)[0].decode();core='#define vermeer_debug_append legacy_append\n#define vermeer_debug_copy legacy_copy\n#define vermeer_debug_total legacy_total\n#define vermeer_debug_dropped legacy_dropped\n'+core
    write(out/'legacy.c',core)
    old=header.decode();needle='    uint64_t left=r->total-*pos;';assert old.count(needle)==1
    old=old.replace(needle,'    extern void log_test_copy_hook(void);log_test_copy_hook();\n'+needle)
    write(out/'vermeer_log_ring.h',old)
    (out/'sys').mkdir();(out/'machine').mkdir();write(out/'sys/systm.h','#include <stdio.h>\n')
    write(out/'machine/atomic.h','''#include <stdatomic.h>
#define os_atomic_cmpxchg(p,o,n,order) ({ __typeof__(atomic_load(p)) e=(o); atomic_compare_exchange_strong_explicit((p),&e,(n),memory_order_##order,memory_order_relaxed); })
#define os_atomic_add(p,v,order) (atomic_fetch_add_explicit((p),(v),memory_order_##order)+(v))
#define os_atomic_load(p,order) atomic_load_explicit((p),memory_order_##order)
#define os_atomic_store(p,v,order) atomic_store_explicit((p),(v),memory_order_##order)
''')
    def native(p):
        p=Path(p).resolve()
        return '/mnt/'+p.drive[0].lower()+'/'+p.as_posix()[3:] if os.name=='nt' else str(p)
    prefix=['wsl.exe','-d','Ubuntu-22.04','--'] if os.name=='nt' else []
    common=['clang','-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-DVERMEER_HOST_TEST=1','-pthread','-fsanitize='+a.sanitizer,'-fno-sanitize-recover=all','-I',native(out)]
    # The literal old prefix keeps optional callback globals whose functions
    # are outside this logger test. Suppress only that translation unit's
    # unused variables; candidate/test code retains the complete warning gate.
    legacy=prefix+common+['-Wno-unused-variable','-c',native(out/'legacy.c'),'-o',native(out/'legacy.o')]
    deployed_compile=prefix+common+['-Wno-unused-variable','-c',native(current_dir/'current.c'),'-o',native(out/'current.o')]
    command=prefix+common+[native(ROOT/'tools/build/test_log_snapshot.c'),native(out/'legacy.o'),native(out/'current.o'),'-o',native(out/'test')]
    if os.name=='nt':command.insert(len(prefix)+1,'-no-pie')
    for name,args in [('legacy-compile',legacy),('current-compile',deployed_compile),('compile',command),('test',prefix+[native(out/'test')])]:
        result=subprocess.run(args,capture_output=True,timeout=60);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError(name+' failed: '+result.stderr.decode('utf-8','replace')[-2000:])
    assert b'Console snapshot PASS:' in (out/'test.log').read_bytes()
    paths=['xnu/support/SweetLogSnapshot.h','tools/build/test_log_snapshot.c','tools/build/check_log_snapshot.py']
    report={'source_files':{p:sha((ROOT/p).read_bytes()) for p in paths},'original_console_source_sha256':sha(original),'original_ring_header_sha256':sha(header),'original_formal_patch_sha256':sha(patch),'deployed_v729_source_hashes':deployed,'v729_increment_sha256':spec['patch_sha256'],'host_copy_and_writer_pause_hooks_only_change_timing':True,'base_reader_contention_drop_reproduced':True,'deployed_writer_contention_drop_reproduced':True,'prototype_only_externally_serialized_writers_supported':True,'atomic_cell_snapshot_and_no_cursor_advance_on_retry':True,'threaded_records_checked':10000,'sanitizers':a.sanitizer,'commands':[legacy,deployed_compile,command,prefix+[native(out/'test')]],'kernel_compiled_or_installed':False,'device_operations':0}
    write(out/'manifest.json',json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
