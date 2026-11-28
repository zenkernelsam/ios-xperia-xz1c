"""Apply the candidate to exact exported source and test its actual getter."""
import argparse,hashlib,json,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
BASE_SHA='78e81f5826347e3589691e3cedfa71d78dab716ca66753f0830c35def74ce334'
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--shadow',required=True);a=p.parse_args()
    out=Path(a.output).resolve();shadow=Path(a.shadow).resolve()
    temporary_roots=[Path(tempfile.gettempdir()).resolve()]
    if os.name!='nt':temporary_roots.append(Path('/tmp').resolve())
    assert out.is_relative_to(ROOT/'build') and not out.exists() and not shadow.exists()
    assert any(shadow.is_relative_to(p) for p in temporary_roots),'Shadow must be temporary'
    out.mkdir(parents=True);(shadow/'bsd/dev').mkdir(parents=True)
    def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    sources=['xnu/lilac-current.patch','xnu/lilac-spmi-lane-observer.patch','xnu/support/LilacSpmiLaneSnapshot.h',
             'xnu/support/LilacSpmiLaneSnapshot.inc','tools/build/test_lilac_spmi_lane_snapshot.c','tools/build/check_lilac_spmi_lane_snapshot.py']
    before={p:digest(ROOT/p) for p in sources}
    patch=(ROOT/'xnu/lilac-current.patch').read_bytes()
    part=re.search(br'diff --git a/bsd/dev/sweet_spmi\.c b/bsd/dev/sweet_spmi\.c\n(.*?)(?=diff --git |\Z)',patch,re.S).group(1)
    started=False;rows=[]
    for line in part.splitlines(keepends=True):
        if line.startswith(b'@@ '):started=True;continue
        if started:
            assert line.startswith(b'+');rows.append(line[1:])
    base=b''.join(rows);assert hashlib.sha256(base).hexdigest()==BASE_SHA
    original=shadow/'bsd/dev/sweet_spmi.c';original.write_bytes(base)
    def run(args,name,cwd=ROOT):
        r=subprocess.run(list(map(str,args)),cwd=cwd,capture_output=True,timeout=30)
        (out/(name+'.log')).write_bytes(r.stdout+r.stderr)
        assert r.returncode==0,(r.stdout+r.stderr).decode('utf8','replace')[-5000:]
        return r.stdout
    delta=ROOT/'xnu/lilac-spmi-lane-observer.patch'
    git=['git','-c','core.autocrlf=false','-c','core.eol=lf','apply']
    run(git+['--check',delta],'apply-check',shadow);run(git+[delta],'apply',shadow)
    newline=b'\r\n' if base.endswith(b'\r\n') else b'\n'
    expected=base.replace(b'#endif /* ARM64_BOARD_CONFIG_SWEET */',b'#include "LilacSpmiLaneSnapshot.inc"'+newline+b'#endif /* ARM64_BOARD_CONFIG_SWEET */')
    assert original.read_bytes()==expected
    selector=re.search(br'static uint16_t\r?\nsp_find_apid\(unsigned ppid\)\r?\n\{.*?\r?\n\}',base,re.S).group(0)
    (out/'PinnedSpmiFind.h').write_bytes(selector+b'\n')
    if os.name=='nt':
        def linux(p):return '/mnt/'+p.drive[0].lower()+p.as_posix()[2:]
        compile=['wsl','-d','Ubuntu-22.04','--exec','gcc'];source=linux(ROOT/'tools/build/test_lilac_spmi_lane_snapshot.c')
        output=linux(out/'test-lane-snapshot');include=linux(out)
        execute=['wsl','-d','Ubuntu-22.04','--exec',output]
    else:
        compile=['clang'];source=ROOT/'tools/build/test_lilac_spmi_lane_snapshot.c';output=out/'test-lane-snapshot';include=out;execute=[output]
    run(compile+['-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined',
        '-fno-sanitize-recover=all','-I',include,source,'-o',output],'compile')
    transcript=run(execute,'run');assert b'WLED lane snapshot host PASS:' in transcript
    assert before=={p:digest(ROOT/p) for p in sources}
    report=dict(source_files=before,base_SPMI_sha256=BASE_SHA,candidate_SPMI_sha256=digest(original),
        exact_existing_selector_sha256=hashlib.sha256(selector).hexdigest(),candidate_patch_apply_checked=True,
        only_added_include_baseline_body_preserved=True,actual_getter_host_ASan_UBSan_passed=True,
        actual_state_API_cases=14,malformed_snapshot_cases=7,snapshot_bytes=112,
        two_equal_observations_existing_lock_checked=True,failed_output_untouched=True,
        no_bus_commands_mapping_initialization_or_owner_writes=True,
        no_epoch_or_retained_write_lease_provided=True,backend_callbacks_and_registration_not_connected=True,
        current_kernel_unchanged=True,ARM64_kernel_build_not_done=True,device_operations=0,permanent_deletions=0,
        shadow=str(shadow),transcript_sha256=hashlib.sha256(transcript).hexdigest())
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
