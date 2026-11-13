"""Verify actual diagnostic trial bytes, current-token metadata, stage and complete return."""
import argparse,hashlib,json,re
from pathlib import Path
from controller import Artifact
from package_trial import ROOT,baseline,sha,prove_payload
from return_evidence import inspect_return
from owner_metadata import verify as verify_owner
from gpu_pixels import verify as verify_gpu_pixels
from diagnostic_candidate import verify_record
from ram_library import record_library
from readonly_pixels import verify as verify_readonly

def verify(directory,mode,kernel_sha=None,version=None):
    root=Path(directory)
    row=json.loads((root/'results.json').read_text(encoding='utf-8'))[-1]
    a=row['artifact']
    assert row['simulated'] is False and row['result']=='PASS' and row['live']
    assert a['trial_mode']==mode
    if version is not None:assert a['version']==version, 'Prerequisite version differs'
    verify_record(a)
    if kernel_sha:assert a['kernel_sha256']==kernel_sha
    Artifact(a).validate()
    payload_proof=prove_payload(a)
    wire=row['boot_wire']
    assert wire['serial']=='BH905ND69G' and wire['retries']==0
    assert wire['download_command_attempts']==wire['boot_command_attempts']==1
    assert wire['image_sha256']==a['sha256']
    assert row['boot_dispatches']==row['reset_requests']==1
    assert row['reset_ack'] and row['fastboot_returned'] and row['reset_to_fastboot_seconds']>=30
    raw=(root/'cycle-01/serial.raw.log').read_bytes()
    assert sha(raw)==row['log_sha256']
    assert sha(raw[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256']
    returned=inspect_return(raw,a['token'],row['checkpoint_log_bytes'],require_drain=True)
    assert returned['complete']
    text=raw.decode('ascii','replace')
    assert text.count('SWEET LOOP LIVE proto=1 token='+a['token'])==1
    start=text.index('SWEET LOOP LIVE proto=1 token='+a['token'])
    ack=text.index('SWEET LOOP ACK FASTBOOT token='+a['token'])
    scope=text[start:ack]
    # Some console records wrap mid-word; normalize them only for metadata checks.
    normalized=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', scope)
    assert 'SWEET RMI4: F12 page=' in scope
    if mode in ('private','ramcpu'):
        vm=re.findall(r'SWEET VM RO END snap=([12]) valid=1 hash=([0-9a-f]+) writes=0 complete=1',scope)
        assert len(vm)==2 and [x[0] for x in vm]==['1','2'] and vm[0][1]==vm[1][1]
        for snap in (1,2):
            assert re.search(r'SWEET SMEM STATE END snap='+str(snap)+r' mode=2 attempted=9 .*writes=0 unowned_reads=0 complete=1',normalized)
        assert 'SWEET SCATTER READY BEGIN' not in text
        if mode=='ramcpu':
            library=record_library(a)
            ready=text.index('SWEET LOOP READY proto=1 token='+a['token'])
            assert ready<start<ack
            library_line='SWEET GLRAM: library read=0 bytes='+str(library['library_bytes'])+' fnv='+library['library_fnv64']+' ro=1'
            assert text[ready:ack].count(library_line)==1
    elif mode=='owner':
        snapshots=verify_owner(normalized)
        assert 'SWEET GPU: rpm gpu clocks -> 0' in scope
        assert re.search(r'SWEET GPU: CP consumed=1 trustoff=1 .*fence=0x1 CP LIVE',scope)
        assert 'SWEET GPU: service on ' in scope
    else:
        assert 'SWEET SCATTER READY END result=0 context=1 identity=1 idle=1 input_bits=36 output_bits=36 native_probe=0' in scope
        assert 'SWEET SCATTER SERVICE registered selectors=10-16 allocator=scatter' in scope
        assert 'SWEET GPU: rpm gpu clocks -> 0' in scope
        if mode=='gpu':
            for item in ('full selftest','full surface selftest','caller selftest','copy refresh selftest','copy context selftest','bridge state selftest','program life selftest'):
                assert 'sweetgl: '+item+' -> PASS' in scope,item
            assert 'SWEET SURF CALLER DIAG phase=' in scope
            pixels=verify_gpu_pixels(re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', text),a['token'])
            if a['version']>=727:
                readonly=verify_readonly(re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', text),a['token'])
    boundary='serialized=1' if mode in ('ready','gpu') else 'serialized=0'
    assert re.search(r'SWEET LOOP MAP_BOUNDARY token='+a['token']+' '+boundary+r' claimed=1',scope)
    report={'version':a['version'],'mode':mode,'source':str(root.resolve()),'token':a['token'],'kernel_sha256':a['kernel_sha256'],
            'image_sha256':a['sha256'],'log_sha256':sha(raw),'return_complete':returned,
            'return_seconds_with_health':row['reset_to_fastboot_seconds'],
            'single_download_boot_verified':True,'actual_stage_verified':True,
            'complete_panel_and_peripheral_goal_not_achieved':True}
    report['payload_audit']=payload_proof
    if mode=='owner':report['snapshots']=snapshots
    if mode=='gpu':report['pixels_and_retirement']=pixels
    if mode=='gpu' and a['version']>=727:report['readonly_pixels']=readonly
    if mode in ('gpu','ramcpu'):report['ram_library_binding']=record_library(a)
    if mode=='ramcpu':report['library_loaded_before_current_live']=text.index(library_line)<start
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--mode',required=True);p.add_argument('--output')
    a=p.parse_args();r=verify(a.directory,a.mode)
    if a.output:Path(a.output).write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(r,indent=2))
