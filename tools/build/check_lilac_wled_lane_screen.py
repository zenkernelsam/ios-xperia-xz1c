"""Verify exact observer-header patch, actual producer and strict decoder."""
import argparse,hashlib,json,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/device'))
from wled_lane_evidence import inspect_scope,inspect


def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--shadow',required=True);a=p.parse_args()
    out=Path(a.output).resolve();shadow=Path(a.shadow).resolve()
    assert out.is_relative_to(ROOT/'build') and not out.exists() and not shadow.exists()
    temporary=[Path(tempfile.gettempdir()).resolve()]
    if os.name!='nt':temporary.append(Path('/tmp').resolve())
    assert any(shadow.is_relative_to(t) for t in temporary)
    out.mkdir(parents=True);target=shadow/'iokit/Kernel/arm/LilacWledScreenProbe.h';target.parent.mkdir(parents=True)
    names=['xnu/support/LilacWledScreenProbe.h','xnu/support/LilacWledLaneScreenProbe.h','xnu/support/LilacSpmiLaneSnapshot.h',
           'xnu/lilac-wled-lane-screen.patch','tools/build/test_lilac_wled_lane_screen.cpp','tools/build/check_lilac_wled_lane_screen.py',
           'tools/device/wled_lane_evidence.py']
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    before={p:sha(ROOT/p) for p in names};original=(ROOT/names[0]).read_bytes();target.write_bytes(original)
    def run(args,name,cwd=ROOT):
        r=subprocess.run(list(map(str,args)),cwd=cwd,capture_output=True,timeout=30)
        (out/(name+'.log')).write_bytes(r.stdout+r.stderr)
        assert r.returncode==0,(r.stdout+r.stderr).decode('utf8','replace')[-4000:]
        return r.stdout
    git=['git','-c','core.autocrlf=false','-c','core.eol=lf','apply']
    delta=ROOT/'xnu/lilac-wled-lane-screen.patch';run(git+['--check',delta],'apply-check',shadow);run(git+[delta],'apply',shadow)
    expected=original.replace(b'#include "LilacWledSnapshot.h"\n',b'#include "LilacWledSnapshot.h"\n#include "LilacWledLaneScreenProbe.h"\n')
    expected=expected.replace(b'(unsigned)snapshot.duty[1]);\n}',b'(unsigned)snapshot.duty[1]);\n    lilac_wled_lane_screen_probe(token, frame_id);\n}')
    assert target.read_bytes()==expected
    if os.name=='nt':
        def linux(p):return '/mnt/'+p.drive[0].lower()+p.as_posix()[2:]
        compiler=['wsl','-d','Ubuntu-22.04','--exec','g++'];source=linux(ROOT/'tools/build/test_lilac_wled_lane_screen.cpp');binary=linux(out/'test-producer');execute=['wsl','-d','Ubuntu-22.04','--exec',binary]
    else:compiler=['clang++'];source=ROOT/'tools/build/test_lilac_wled_lane_screen.cpp';binary=out/'test-producer';execute=[binary]
    run(compiler+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-fno-sanitize-recover=all',source,'-o',binary],'compile')
    raw=run(execute,'run');text=raw.decode();assert 'producer host PASS:' in text
    token='0123456789abcdef';decoded={}
    for name in ('success','getter-failed','invalid-state','wrong-ppid'):
        scope=text.split('HOST WLED LANES '+name+' BEGIN\n',1)[1].split('HOST WLED LANES '+name+' END',1)[0]
        decoded[name]=inspect_scope(scope,token)
        assert decoded[name]['both_queries_succeeded']==(name=='success')
    success=text.split('HOST WLED LANES success BEGIN\n',1)[1].split('HOST WLED LANES success END',1)[0]
    dropped=text.split('HOST WLED LANES live-dropped BEGIN\n',1)[1].split('HOST WLED LANES live-dropped END',1)[0]
    bad=dict(live_dropped=dropped,missing_item=re.sub(r'SWEET WLED LANES ITEM[^\n]*\n','',success,count=1),
             missing_END=re.sub(r'SWEET WLED LANES END[^\n]*','',success),
             wrong_token=success.replace(token,'fedcba9876543210'),wrong_count=success.replace('matches=1','matches=2',1),
             selected_changed=success.replace('selected=11','selected=12',1),claim_write_lease=success.replace('write_lease=0','write_lease=1'),
             unknown_record=success+'SWEET WLED LANES unknown\n')
    rejected={}
    for name,value in bad.items():
        try:inspect_scope(value,token)
        except ValueError as e:rejected[name]=str(e)
        else:raise AssertionError('Accepted bad record '+name)
    live='SWEET LOOP LIVE proto=1 token='+token+'\n';ack='SWEET LOOP ACK FASTBOOT token='+token+'\n'
    frame='SWEET SCREEN END token='+token+' id=1 encoded=10 blocks=1 raw_fnv=0000000000000001 enc_crc=00000001 complete=1\n'
    whole=live+frame+success+ack;assert inspect(whole,token)['frame_end_boundary_checked']
    for name,value in dict(frame_late=live+success+frame+ack,missing_frame=live+success+ack,
                           token_prefix=whole.replace(token,token+'0'),ACK_first=ack+whole).items():
        try:inspect(value,token)
        except ValueError as e:rejected[name]=str(e)
        else:raise AssertionError('Accepted bad boundary '+name)
    assert before=={p:sha(ROOT/p) for p in names}
    report=dict(source_files=before,actual_producer_ASan_UBSan_passed=True,producer_cases=8,
        decoder_completed_cases=4,corruption_and_boundary_rejections=rejected,
        exact_header_delta_checked=True,patched_header_sha256=sha(target),
        token_current_LIVE_first_frame_and_no_repeat_checked=True,
        frame_bytes_not_verified_by_host_marker_cases=True,no_epoch_or_write_lease=True,
        ARM64_build_not_done=True,current_kernel_not_changed=True,device_operations=0,permanent_deletions=0,
        shadow=str(shadow),transcript_sha256=hashlib.sha256(raw).hexdigest())
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
