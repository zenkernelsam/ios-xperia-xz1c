"""Preserve correctly returned queue trial whose original fence records were capped."""
import argparse,json,hashlib,re
from pathlib import Path
from trial_evidence import verify as verify_stage
from ram_library import require_same_library
from controller import Artifact
from package_trial import prove_payload
from return_evidence import inspect_return
from gpu_pixels import verify as verify_gpu
from metal_queue_evidence import verify as verify_queue,expected_hash

parser=argparse.ArgumentParser();parser.add_argument('directory');parser.add_argument('--cpu',required=True);parser.add_argument('--output',required=True)
args=parser.parse_args();root=Path(args.directory);original=(root/'results.json').read_bytes();row=json.loads(original)[-1];a=row['artifact']
assert row['simulated'] is False and row['live'];Artifact(a).validate();payload=prove_payload(a)
cpu=verify_stage(args.cpu,'ramcpu',a['kernel_sha256'],a['version']);require_same_library(a,cpu)
raw=(root/'cycle-01/serial.raw.log').read_bytes();sha=lambda data:hashlib.sha256(data).hexdigest()
assert sha(raw)==row['log_sha256'] and sha(raw[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256']
returned=inspect_return(raw,a['token'],row['checkpoint_log_bytes'],require_drain=True);assert returned['complete']
assert row['reset_to_fastboot_seconds']>=30 and row['boot_dispatches']==row['reset_requests']==1
wire=row['boot_wire'];assert wire['serial']=='BH905ND69G' and wire['retries']==0 and wire['image_sha256']==a['sha256']
assert wire['boot_command_attempts']==wire['download_command_attempts']==1
text=raw.decode('ascii','replace');normalized=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',text)
pixels=verify_gpu(normalized,a['token']);error=None
try:verify_queue(text,a['token'])
except AssertionError as failure:error=str(failure) or 'Actual submission metadata missing'
assert error
completed=re.findall(r'SWEET METAL QUEUE completed first_status=4 second_status=4 first_seq=(\d+) second_seq=(\d+) first_ops=2 second_ops=1 scheduled=2 callbacks=2 checked=33024 mismatches=0 hash='+expected_hash()+r' valid=1',text)
assert completed
report=dict(token=a['token'],kernel_sha256=a['kernel_sha256'],image_sha256=a['sha256'],raw_sha256=sha(raw),
    original_results_sha256=sha(original),original_controller_result=row['result'],return_complete=returned,
    return_seconds_with_health=row['reset_to_fastboot_seconds'],original_full_GLES_pixels_verified=pixels,
    queue_reported_byte_and_callback_results=completed,actual_full_queue_fence_evidence_passed=False,
    failed_verification_reason=error,original_result_not_rewritten=True,system_Metal_and_shader_rendering_not_provided=True)
assert (root/'results.json').read_bytes()==original
Path(args.output).write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps({k:v for k,v in report.items() if k not in ('return_complete','original_full_GLES_pixels_verified','queue_reported_byte_and_callback_results')}))
