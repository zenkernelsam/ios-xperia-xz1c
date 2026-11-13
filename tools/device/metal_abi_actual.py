"""Verify a metadata trial separately when unrelated GPU log evidence is incomplete."""
import argparse,hashlib,json,re
from pathlib import Path
from controller import Artifact
from package_trial import prove_payload
from return_evidence import inspect_return
from metal_abi_evidence import verify as verify_abi
from gpu_pixels import verify as verify_gpu
from trial_evidence import verify as verify_stage
from ram_library import require_same_library

def verify(directory,cpu_directory):
    root=Path(directory);original=(root/'results.json').read_bytes();row=json.loads(original)[-1];a=row['artifact']
    assert row['simulated'] is False and row['live'] and a['trial_mode']=='gpu'
    Artifact(a).validate();payload=prove_payload(a)
    cpu=verify_stage(cpu_directory,'ramcpu',a['kernel_sha256'],a['version']);require_same_library(a,cpu)
    wire=row['boot_wire'];assert wire['serial']=='BH905ND69G' and wire['retries']==0
    assert wire['download_command_attempts']==wire['boot_command_attempts']==1 and wire['image_sha256']==a['sha256']
    assert row['boot_dispatches']==row['reset_requests']==1 and row['reset_ack'] and row['fastboot_returned']
    assert row['reset_to_fastboot_seconds']>=30
    data=(root/'cycle-01/serial.raw.log').read_bytes();sha=lambda data:hashlib.sha256(data).hexdigest()
    assert sha(data)==row['log_sha256'] and sha(data[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256']
    returned=inspect_return(data,a['token'],row['checkpoint_log_bytes'],require_drain=True);assert returned['complete']
    text=data.decode('ascii','replace');abi=verify_abi(text,a['token']);reason=None;pixels=None
    try:pixels=verify_gpu(re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',text),a['token'])
    except AssertionError as error:reason=str(error)
    assert (root/'results.json').read_bytes()==original
    return dict(token=a['token'],kernel_sha256=a['kernel_sha256'],image_sha256=a['sha256'],payload_audit=payload,
        raw_sha256=sha(data),original_results_sha256=sha(original),original_controller_result=row['result'],
        runtime_Metal_ABI=abi,return_complete=returned,return_seconds_with_health=row['reset_to_fastboot_seconds'],
        full_GPU_pixel_verification_passed=reason is None,GPU_verification_error=reason,GPU_pixels=pixels,
        original_result_unchanged=True,Metal_device_and_rendering_not_yet_provided=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory');parser.add_argument('--cpu',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();report=verify(args.directory,args.cpu);Path(args.output).write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:v for k,v in report.items() if k not in ('runtime_Metal_ABI','payload_audit','return_complete','GPU_pixels')}))
