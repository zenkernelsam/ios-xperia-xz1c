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
from exception_evidence import verify as verify_exception
from metal_abi_evidence import verify as verify_metal_abi
from metal_resource_evidence import verify as verify_metal_resources
from metal_queue_evidence import verify as verify_metal_queue
from air_compute_evidence import verify as verify_air_compute
from air_render_evidence import verify as verify_air_render
from air_constants_evidence import verify as verify_air_constants
from metal_discovery_evidence import verify as verify_metal_discovery
from air_xr_evidence import verify as verify_air_xr
from metal_surface_evidence import verify as verify_metal_surface
from metal_mip_evidence import verify as verify_metal_mips
from backboard_probe_evidence import verify as verify_backboard_probe
from registry_proof_evidence import verify as verify_registry_proof
from pipeline_observer_evidence import verify as verify_pipeline_observer
from air_narrow_evidence import verify as verify_air_narrow
from air_half_evidence import verify as verify_air_half
from metal_mrt_evidence import verify as verify_metal_mrt
from ramcpu_prefix_evidence import verify as verify_ramcpu_prefix
from air_sampler_evidence import verify as verify_air_sampler
from program_cache_evidence import verify as verify_program_cache
from static_half_evidence import verify as verify_static_half
from program_request_evidence import verify as verify_program_request
from program_cache_scope_evidence import verify as verify_cache_scope
from postdraw_observer_evidence import inspect as inspect_postdraw
from cif10_reference_evidence import inspect as inspect_cif10_reference

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
            library_line='SWEET GLRAM: library read=0 bytes='+str(library['library_bytes'])+' fnv='+library['library_fnv64']+' ro=1'
            ramcpu_prefix=verify_ramcpu_prefix(raw,a['token'],library_line)
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
            if 'exception_observer' in a:
                if a['exception_observer']['target_process']=='backboardd':exceptions={'background_probe':verify_backboard_probe(text,a['token'])}
                elif a['exception_observer'].get('registration_only') and 'SWEET EXCEPTION begin call=' not in scope:
                    assert 'SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1' in scope
                    # An original application succeeding need not throw. Native
                    # discovery, exact prior GPU proof and actual factory return
                    # remain independently required below for this scoped mode.
                    exceptions={'original_exception_events':0,'original_chain_installed':True}
                else:exceptions=verify_exception(text,a['token'],require_renderer='renderer_observations_max' in a['exception_observer'] and not a['exception_observer'].get('original_Metal_registry_probe'))
                if 'runtime_Metal_ABI_methods' in a['exception_observer']:exceptions['runtime_Metal_ABI']=verify_metal_abi(text,a['token'])
                if a['exception_observer'].get('metal_resource_probe'):exceptions['Metal_resources']=verify_metal_resources(text,a['token'])
                if a['exception_observer'].get('metal_queue_probe'):exceptions['Metal_queue']=verify_metal_queue(text,a['token'],a.get('metal_resources',{}).get('blit_transport_evidence_revision'))
                if a['exception_observer'].get('AIR_compute_probe'):exceptions['AIR_compute']=verify_air_compute(text,a['token'],a['exception_observer']['AIR_fixture'])
                if a['exception_observer'].get('AIR_render_probe'):exceptions['AIR_render']=verify_air_render(text,a['token'],a['exception_observer']['AIR_render_fixture'])
                if a['exception_observer'].get('AIR_constants_probe'):exceptions['AIR_constants']=verify_air_constants(text,a['token'],a['exception_observer']['AIR_constants_fixture'])
                if a['exception_observer'].get('original_Metal_registry_probe'):exceptions['Metal_discovery']=verify_metal_discovery(text,a['token'])
                if a['exception_observer'].get('registration_only'):exceptions['native_prior_binding']=verify_registry_proof(text,a['token'],a['exception_observer']['prior_native_hardware_proof'])
                if a['exception_observer'].get('original_pipeline_observer'):exceptions['pipeline_factory']=verify_pipeline_observer(text,a['token'])
                if a['exception_observer'].get('original_postdraw_observer'):exceptions['postdraw_snapshot']=inspect_postdraw(text,a['token'],method_observer=a['exception_observer'].get('snapshot_method_observer',False),png_input_observer=a['exception_observer'].get('postdraw_png_input_metadata',False))
                if a['exception_observer'].get('AIR_XR_probe'):exceptions['AIR_XR']=verify_air_xr(text,a['token'])
                if a['exception_observer'].get('Metal_IOSurface_probe'):exceptions['Metal_IOSurface']=verify_metal_surface(text,a['token'])
                if a['exception_observer'].get('Metal_mipmap_probe'):exceptions['Metal_mipmaps']=verify_metal_mips(text,a['token'])
                if a['exception_observer'].get('AIR_narrow_probe'):exceptions['AIR_narrow']=verify_air_narrow(text,a['token'],a['exception_observer']['AIR_narrow_fixture'])
                if a['exception_observer'].get('AIR_half_probe'):exceptions['AIR_half']=verify_air_half(text,a['token'],a['exception_observer']['AIR_half_fixture'])
                if a['exception_observer'].get('AIR_MRT_probe'):exceptions['Metal_MRT']=verify_metal_mrt(text,a['token'],a['exception_observer']['AIR_MRT_fixture'],require_xr_clear_elision=a.get('metal_resources',{}).get('native_XR_clear_decode_elision',False))
                if a['exception_observer'].get('AIR_sampler_probe'):exceptions['AIR_sampler']=verify_air_sampler(text,a['token'],a['exception_observer']['AIR_sampler_fixture'])
                if a['exception_observer'].get('program_cache_probe'):exceptions['Program_cache']=verify_program_cache(text,a['token'],a['exception_observer']['AIR_sampler_fixture'])
                if a['exception_observer'].get('AIR_static_half_probe'):exceptions['AIR_static_half']=verify_static_half(text,a['token'],a['exception_observer']['AIR_static_half_fixture'])
                if a['exception_observer'].get('program_request_export'):exceptions['Program_request_export']=verify_program_request(text,a['token'],a['exception_observer']['program_request_export'])
                if a['exception_observer'].get('program_cache_scope'):exceptions['Program_cache_scope']=verify_cache_scope(text,a['token'],a['exception_observer']['program_cache_scope'])
                if a['exception_observer'].get('cif10_PNG_reference'):exceptions['CIF10_PNG_reference']=inspect_cif10_reference(text,a['token'])
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
    if mode=='gpu' and 'exception_observer' in a:report['exceptions']=exceptions
    if mode in ('gpu','ramcpu'):report['ram_library_binding']=record_library(a)
    if mode=='ramcpu':
        report['library_loaded_before_current_live']=text.index(library_line)<start
        report['ram_library_prefix']=ramcpu_prefix
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--mode',required=True);p.add_argument('--output')
    a=p.parse_args();r=verify(a.directory,a.mode)
    if a.output:Path(a.output).write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(r,indent=2))
