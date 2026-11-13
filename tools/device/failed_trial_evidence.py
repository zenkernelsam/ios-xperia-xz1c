"""Preserve an actual failed trial without turning a complete return into stage PASS."""
import argparse,json,re
from pathlib import Path
from controller import Artifact
from package_trial import prove_payload,sha
from return_evidence import inspect_return
def verify(directory):
    root=Path(directory);row=json.loads((root/'results.json').read_bytes())[-1];a=row['artifact']
    assert row['simulated'] is False and row['result']=='REVISE' and row['decision']=='REVISE'
    assert row['live'] and row['reset_ack'] and row['fastboot_returned']
    assert row['boot_dispatches']==row['reset_requests']==1 and row['reset_to_fastboot_seconds']>=30
    Artifact(a).validate();payload=prove_payload(a);wire=row['boot_wire']
    assert wire['serial']=='BH905ND69G' and wire['retries']==0
    assert wire['download_command_attempts']==wire['boot_command_attempts']==1
    assert wire['image_sha256']==a['sha256']
    raw=(root/'cycle-01/serial.raw.log').read_bytes()
    assert sha(raw)==row['log_sha256'] and sha(raw[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256']
    returned=inspect_return(raw,a['token'],row['checkpoint_log_bytes'],require_drain=True);assert returned['complete']
    text=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',raw.decode('ascii','replace'))
    start=text.index('SWEET LOOP LIVE proto=1 token='+a['token']);end=text.index('SWEET LOOP ACK FASTBOOT token='+a['token'])
    scope=text[start:end];wrong=re.findall(r'sweetgl: (\w+(?: \w+)*) selftest -> WRONG phase=(\w+)',scope)
    failures=re.findall(r'SWEET (?:AIR (?:COMPUTE|RENDER)|METAL (?:RESOURCE|QUEUE)) END result=([1-9]\d*)',scope)
    assert wrong or failures,'Actual failure must be present in the current LIVE scope'
    return dict(source=str(root.resolve()),version=a['version'],token=a['token'],kernel_sha256=a['kernel_sha256'],
        image_sha256=a['sha256'],log_sha256=sha(raw),original_result=row['result'],original_reason=row['reason'],
        actual_failures=wrong,native_GPU_failure_results=list(map(int,failures)),payload_audit=payload,strict_return=returned,
        return_seconds_with_health=row['reset_to_fastboot_seconds'],failed_stage_not_accepted=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    result=verify(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps(result,indent=2))
