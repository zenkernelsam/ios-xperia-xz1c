"""Preserve an actual failed trial without turning a complete return into stage PASS."""
import argparse,json,re
from pathlib import Path
from controller import Artifact
from package_trial import prove_payload,sha
from return_evidence import inspect_return

def unfinished_prime(scope,observer):
    if not observer.get('program_cache_prime'):return []
    begins=list(re.finditer(r'SWEET PROGRAM CACHE prime_begin vertex_bytes=(\d+) fragment_bytes=(\d+) source_verified=1',scope))
    if len(begins)!=1:return []
    block=scope[begins[0].end():]
    if any(marker in block for marker in ('SWEET PROGRAM CACHE prime_entry','SWEET PROGRAM CACHE PRIME END','SWEET METAL PROBES END')):return []
    compiled=re.findall(r'SWEET AIR RENDER compile_end stage=(vertex|fragment) wall_us=(\d+) cpu_us=(\d+) cpu_clock=1 status=1',block)
    link=re.findall(r'SWEET AIR RENDER compile_begin stage=link vertex_bytes=(\d+) fragment_bytes=(\d+)',block)
    if len(compiled)!=2 or [row[0] for row in compiled]!=['vertex','fragment'] or len(link)!=1 or 'SWEET AIR RENDER compile_end stage=link' in block:return []
    if 'SWEET AIR STATIC HALF END result=0' not in scope[:begins[0].start()]:return []
    return [dict(request_vertex_bytes=int(begins[0].group(1)),request_fragment_bytes=int(begins[0].group(2)),
                 wrapped_vertex_bytes=int(link[0][0]),wrapped_fragment_bytes=int(link[0][1]),
                 compile_times=[dict(stage=s,wall_us=int(w),cpu_us=int(c)) for s,w,c in compiled],
                 link_return_not_observed=True,reason='BACKGROUND_ORIGINAL_REQUEST_LINK_INCOMPLETE')]
def verify(directory):
    root=Path(directory);row=json.loads((root/'results.json').read_bytes())[-1];a=row['artifact']
    assert row['simulated'] is False
    assert row['result']==row['decision'] and row['result'] in ('REVISE','STOP')
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
    render_errors=[int(value) for value in re.findall(r'SWEET AIR RENDER draw [^\r\n]* result=(-?\d+)',scope) if int(value)]
    capability_keys=('targets','fp32_rt','fp32_blend','fp32_image','independent_enable','independent_func','float_blend','fbfetch','no_attachment','fragment_images','image_units')
    capability_pattern=r'SWEET METAL MRT caps '+r' '.join(key+r'=(\d+)' for key in capability_keys)
    capabilities=[dict(zip(capability_keys,map(int,record))) for record in dict.fromkeys(re.findall(capability_pattern,scope))]
    gl_errors=[dict(stage=stage,target=int(target),error=error) for stage,target,error in re.findall(r'SWEET METAL MRT GL_error stage=(\S+) target=(-?\d+) error=(0x[0-9a-f]+)',scope)]
    failures=re.findall(r'SWEET (?:AIR (?:COMPUTE|RENDER|CONSTANTS|XR|NARROW|HALF|SAMPLER)|METAL (?:RESOURCE|QUEUE|DISCOVERY|SURFACE|MIPS|MRT)|CLEAR CACHE|IR3 CACHE PROBE) END result=([1-9]\d*)',scope)
    cache_failures=[dict(phase=int(phase),first_state=int(first),restored_state=int(restored)) for phase,first,restored in
        re.findall(r'SWEET PROGRAM CACHE roundtrip phase=(\d+) first_state=(-?\d+) restored_state=(-?\d+) fresh_pipeline=1 valid=0',scope)]
    registry_assertions=re.findall(r'failed assertion ([^\r\n]+) at line (\d+) in MTLAddDevice',scope)
    missing_API=re.findall(r'SWEET METAL missing class=(\S+) selector=(\S+) original_exception=1',scope)
    # Some older native classes did not yet emit the supplemental sentinel.
    # Preserve the original exception's exact class/selector as independent evidence.
    original_missing_API=re.findall(r'SWEET EXCEPTION detail call=\d+ reason="-\[(SweetMTL\w+) ([^ ]+)\]: unrecognized selector',scope)
    missing_API+=original_missing_API
    extension_timeout=re.findall(r'CRASH AegirPoster reason ns 7 code 0x3: extension check-in timeout',scope)
    prime_incomplete=unfinished_prime(scope,a.get('exception_observer',{}))
    scope_errors=[int(code) for code in re.findall(r'SWEET PROGRAM CACHE SCOPE result=(-?\d+)',scope) if int(code)]
    if row['result']=='STOP':assert row['reason']=='OBSERVATION_INCOMPLETE' and (scope_errors or prime_incomplete or ((registry_assertions or missing_API or extension_timeout) and 'CRASH AegirPoster' in scope))
    assert wrong or failures or cache_failures or registry_assertions or missing_API or extension_timeout or prime_incomplete or scope_errors,'Actual failure must be present in the current LIVE scope'
    return dict(source=str(root.resolve()),version=a['version'],token=a['token'],kernel_sha256=a['kernel_sha256'],
        image_sha256=a['sha256'],log_sha256=sha(raw),original_result=row['result'],original_reason=row['reason'],
        actual_failures=wrong,native_GPU_failure_results=list(map(int,failures)),actual_program_cache_failures=cache_failures,registry_assertions=registry_assertions,
        actual_AIR_render_failure_results=render_errors,actual_MRT_context_capability_records=capabilities,actual_MRT_GL_errors=gl_errors,
        missing_device_API=missing_API,original_exception_missing_API=original_missing_API,original_extension_checkin_timeout_count=len(extension_timeout),actual_program_prime_incomplete=prime_incomplete,actual_program_cache_scope_errors=scope_errors,payload_audit=payload,strict_return=returned,
        return_seconds_with_health=row['reset_to_fastboot_seconds'],failed_stage_not_accepted=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    result=verify(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps(result,indent=2))
