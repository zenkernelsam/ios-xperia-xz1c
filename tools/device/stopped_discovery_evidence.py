"""Keep a stopped original-app trial and separately verify device registration."""
import argparse,json,re
from pathlib import Path
from controller import Artifact
from package_trial import prove_payload,sha
from return_evidence import inspect_return
from metal_discovery_evidence import verify as registration
from exception_evidence import inspect_scope
from air_constants_evidence import verify as constants
from air_render_evidence import verify as render
from gpu_pixels import verify as gles
def verify(directory):
    root=Path(directory);row=json.loads((root/'results.json').read_bytes())[-1];a=row['artifact']
    assert not row['simulated'] and row['result']=='STOP' and row['reason']=='OBSERVATION_INCOMPLETE' and row['live']
    assert row['boot_dispatches']==row['reset_requests']==1 and row['reset_ack'] and row['fastboot_returned'] and row['reset_to_fastboot_seconds']>=30
    Artifact(a).validate();payload=prove_payload(a);wire=row['boot_wire']
    assert wire['serial']=='BH905ND69G' and wire['retries']==0 and wire['download_command_attempts']==wire['boot_command_attempts']==1
    assert wire['image_sha256']==a['sha256'];raw=(root/'cycle-01/serial.raw.log').read_bytes();assert sha(raw)==row['log_sha256']
    assert sha(raw[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256'];returned=inspect_return(raw,a['token'],row['checkpoint_log_bytes'],require_drain=True);assert returned['complete']
    text=raw.decode('ascii','replace');start=text.index('SWEET LOOP LIVE proto=1 token='+a['token']);end=text.index('SWEET LOOP ACK FASTBOOT token='+a['token'])
    errors=inspect_scope(text[start:end]);assert all(item['name']=='CAMetalLayerInvalid' and item['reason']=='unsupported extended range format' for item in errors['records'])
    normalized=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',text)
    observer=a['exception_observer']
    return dict(source=str(root.resolve()),token=a['token'],original_controller_result=row['result'],original_reason=row['reason'],
        original_result_not_promoted=True,image_sha256=a['sha256'],kernel_sha256=a['kernel_sha256'],raw_sha256=sha(raw),raw_bytes=len(raw),
        complete_return=returned,return_seconds_with_health=row['reset_to_fastboot_seconds'],payload_audit=payload,
        original_Metal_registration=registration(text,a['token']),AIR_render=render(text,a['token'],observer['AIR_render_fixture']),
        AIR_constants=constants(text,a['token'],observer['AIR_constants_fixture']),original_GLES=gles(normalized,a['token']),
        original_app_exceptions=errors,complete_UI_and_other_peripherals_pending=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args();result=verify(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps({'original_result':result['original_controller_result'],'registered_samples':result['original_Metal_registration']['complete_samples'],'render_samples':result['AIR_render']['complete_samples'],'constants_samples':result['AIR_constants']['complete_samples'],'return':result['return_seconds_with_health'],'UI_error':'unsupported extended range format'}))
