"""Fault replay for prerequisites and live diagnostic failures; no device access."""
import hashlib
import copy
import json
import os
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from clock_evidence import verify as verify_clock
from owner_metadata import verify as verify_owner
from gpu_pixels import verify as verify_gpu
from run_trial import DiagnosticPolicy, prerequisites
from package_trial import validate_wled_gpu_ram,validate_wled_gpu_record
from diagnostic_candidate import WLED_KERNEL_SHA, WLED_MARKER, WLED_VERSION, configuration, verify_record, verify_wled_kernel
from ram_library import require_same_library,load as load_ram
from readonly_pixels import verify as verify_readonly,expected_hash as readonly_hash
from exception_evidence import inspect_scope,verify_renderer


class Gates(unittest.TestCase):
    def test_extension_checkin_and_mip_failure_route_return(self):
        for line,reason in [('CRASH AegirPoster reason ns 7 code 0x3: extension check-in timeout','EXTENSION_CHECKIN_TIMEOUT'),
            ('SWEET METAL MIPS END result=1 formats=0','METAL_MIPMAP_PROBE_FAILED'),
            ('SWEET PCINV END pid=200 phase=B result=-22 files=0','PROGRAM_CACHE_INVENTORY_FAILED'),
            ('SWEET PCINV COMPLETE pid=200 result=-5 verified_before_after=0','PROGRAM_CACHE_INVENTORY_FAILED')]:
            policy=DiagnosticPolicy('0123456789abcdef',[],0);policy.feed('SWEET LOOP LIVE proto=1 token=0123456789abcdef',0);policy.feed(line,1)
            self.assertEqual(policy.recoverable,reason)
    def test_original_native_missing_selector_routes_return(self):
        policy=DiagnosticPolicy('0123456789abcdef',[],0)
        policy.feed('SWEET LOOP LIVE proto=1 token=0123456789abcdef',0)
        policy.feed('SWEET EXCEPTION detail call=1 reason="-[SweetMTLBlitEncoder generateMipmapsForTexture:]: unrecognized selector sent to instance 0x1" observational=1',1)
        self.assertEqual(policy.recoverable,'METAL_API_UNIMPLEMENTED')
        other=DiagnosticPolicy('0123456789abcdef',[],0)
        other.feed('SWEET LOOP LIVE proto=1 token=0123456789abcdef',0)
        other.feed('SWEET EXCEPTION detail call=1 reason="an unrelated application error" observational=1',1)
        self.assertNotEqual(other.recoverable,'METAL_API_UNIMPLEMENTED')
    def test_actual_renderer_metadata_requires_valid_abi_and_nil_identity(self):
        text=('SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=Q device_object=1 original_forwarded=1\n'
              'SWEET METAL uniform call=1 index=0 device_nil=1 device_class=nil new_buffer_method=0 metadata_only=1 original_called=1\n')
        self.assertTrue(verify_renderer(text)['all_observed_devices_nil'])
        for changed in (text.replace('index_type=Q','index_type=i'),text.replace('device_class=nil','device_class=MTLDevice'),
                        text.replace('new_buffer_method=0','new_buffer_method=1'),text.replace('index=0','index=3'),
                        text.replace('original_called=1','original_called=0'),text.replace('call=1','call=33')):
            with self.assertRaises(AssertionError):verify_renderer(changed)
    def test_exception_records_preserve_actual_metadata_and_complete_stack(self):
        text=('SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1\n'
          'SWEET EXCEPTION begin call=1 class=NSException name="NSInvalidArgumentException" reason="missing method" observational=1\n'
          'SWEET EXCEPTION frame call=1 index=0 pc=0x100010000 base=0x100000000 image=e symbol=observe_exception\n'
          'SWEET EXCEPTION frame call=1 index=1 pc=0x180010000 base=0x180000000 image=Foundation symbol=raise\n'
          'SWEET EXCEPTION end call=1 frames=2 original_chain=1\n')
        proof=inspect_scope(text);self.assertEqual(proof['records'][0]['name'],'NSInvalidArgumentException')
        for changed in (text.replace('frames=2','frames=3'),text.replace('original_chain=1','original_chain=0'),
                        text.replace('index=1','index=3'),text.replace('call=1','call=17')):
            with self.assertRaises(AssertionError):inspect_scope(changed)
        wrapped=text.replace('missing method','missing me\nthod').replace('symbol=raise','symbol=ra\nise')
        self.assertEqual(inspect_scope(wrapped)['records'],proof['records'])
        separate=text.replace(' name="NSInvalidArgumentException" reason="missing method" observational=1',
            ' name="NSInvalidArgumentException" observational=1\nSWEET EXCEPTION detail call=1 reason="missing method" observational=1')
        self.assertEqual(inspect_scope(separate)['records'],proof['records'])
        truncated=separate.replace('SWEET EXCEPTION detail call=1 reason="missing method" observational=1\n','')
        with self.assertRaises(AssertionError):inspect_scope(truncated)
        missing=text.replace('SWEET EXCEPTION frame call=1 index=1 pc=0x180010000 base=0x180000000 image=Foundation symbol=raise\n','')
        combined=inspect_scope(missing+text)
        self.assertEqual(len(combined['records']),1)
        self.assertEqual(combined['incomplete_records'][0]['captured_frame_indices'],[0])

    def args(self, **changes):
        values = dict(private_evidence='private', owner_evidence='owner', ready_evidence='ready',
                      cpu_evidence='cpu', rpm_evidence='clock')
        values.update(changes)
        return SimpleNamespace(**values)

    def test_clock_required_for_all_gpu_starting_stages(self):
        for mode in ('owner', 'ready', 'gpu'):
            with self.subTest(mode=mode), patch('run_trial.verify'):
                with self.assertRaises(AssertionError):
                    prerequisites({'trial_mode': mode, 'kernel_sha256': 'kernel'}, self.args(rpm_evidence=None))

    def test_gpu_requires_owner_ready_and_cpu_for_same_kernel(self):
        with patch('run_trial.verify', return_value={}) as stages, patch('run_trial.verify_clock', return_value={}) as clock:
            prerequisites({'version':726,'trial_mode': 'gpu', 'kernel_sha256': 'kernel'}, self.args())
            self.assertEqual(stages.call_args_list, [unittest.mock.call('private', 'private', 'kernel',726),
                unittest.mock.call('owner', 'owner', 'kernel',726), unittest.mock.call('ready', 'ready', 'kernel',726),
                unittest.mock.call('cpu', 'ramcpu', 'kernel',726)])
            clock.assert_called_once_with('clock')

    def test_invalid_clock_blocks_stage(self):
        with patch('run_trial.verify'), patch('run_trial.verify_clock', side_effect=AssertionError('bad clock')):
            with self.assertRaises(AssertionError):
                prerequisites({'trial_mode': 'owner', 'kernel_sha256': 'kernel'}, self.args())

    def test_native_resources_require_complete_matching_GLES_evidence(self):
        record={'version':728,'trial_mode':'gpu','kernel_sha256':'kernel','metal_resources':{'gles_library_sha256':'new'}}
        with patch('run_trial.verify',return_value={}),patch('run_trial.verify_clock'),patch('run_trial.require_same_library'):
            with self.assertRaisesRegex(AssertionError,'full GLES stage'):prerequisites(record,self.args())
        def stage(path,mode,*args):return {'ram_library_binding':{'library_sha256':'old'}} if path=='gles' else {}
        with patch('run_trial.verify',side_effect=stage),patch('run_trial.verify_clock'),patch('run_trial.require_same_library'):
            with self.assertRaises(AssertionError):prerequisites(record,self.args(gles_evidence='gles'))

    def test_positive_and_negative_nonzero_failures_return_after_live(self):
        for line in ('SWEET SG OWN END snap=1 result=1', 'SWEET SG OWN END snap=2 result=-2',
                     'SWEET SCATTER READY END result=3', 'sweetgl: caller selftest -> WRONG phase=map',
                     'SWEET DEPTH32 END result=1', 'SWEET STENCIL8 END result=1', 'SWEET MEMORYLESS END result=1', 'SWEET MULTISAMPLE END result=1', 'SWEET PRESENT PROBE END result=1'):
            with self.subTest(line=line):
                policy = DiagnosticPolicy('0123456789abcdef', [], 30)
                policy.feed(line, 0)
                self.assertEqual(policy.decision(1).kind, 'WAIT')
                policy.feed('SWEET LOOP LIVE proto=1 token=0123456789abcdef', 2)
                policy.feed(line, 3)
                self.assertEqual(policy.decision(3).kind, 'REVISE')

    def test_gpu_minimum_window(self):
        policy = DiagnosticPolicy('0123456789abcdef', ['marker'], 30); policy.minimum = 180
        policy.feed('SWEET LOOP LIVE proto=1 token=0123456789abcdef', 1)
        policy.feed('marker', 2)
        self.assertEqual(policy.decision(50).kind, 'WAIT')
        self.assertEqual(policy.decision(180).kind, 'PASS')

    def test_new_version_cannot_omit_or_change_source_binding(self):
        with self.assertRaises((AssertionError,KeyError)):verify_record({'version':726})
        binding=configuration(726)['binding']
        verify_record({'version':726,'diagnostic_binding':binding})
        changed=json.loads(json.dumps(binding));changed['manifest_sha256']='0'*64
        with self.assertRaises(AssertionError):verify_record({'version':726,'diagnostic_binding':changed})
        screen=configuration(730)
        self.assertEqual(screen['manifest']['base_version'],729)
        self.assertEqual(len(screen['source_hashes']),83)
        self.assertEqual(screen['source_hashes']['iokit/Kernel/arm/SweetGPU.cpp'],configuration(729)['source_hashes']['iokit/Kernel/arm/SweetGPU.cpp'])
        with self.assertRaises(AssertionError):configuration(732)

    def test_wled_has_its_own_kernel_and_scanout_source_identity(self):
        baseline = configuration(730)
        candidate = configuration(WLED_VERSION)
        record = dict(version=WLED_VERSION, diagnostic_binding=candidate['binding'], kernel_sha256=WLED_KERNEL_SHA,
                      source_revision=candidate['binding']['manifest_sha256'])
        self.assertEqual(verify_record(record)['kernel_sha256'], WLED_KERNEL_SHA)
        self.assertEqual(len(candidate['source_hashes']), len(baseline['source_hashes']) + 2)
        self.assertEqual({name for name, digest in candidate['source_hashes'].items()
                          if baseline['source_hashes'].get(name) != digest}, set(candidate['changed_sources']))
        with self.assertRaises(AssertionError):
            verify_record(dict(record, diagnostic_binding=baseline['binding']))
        with self.assertRaises(AssertionError):
            verify_record(dict(record, kernel_sha256='0'*64))
        with self.assertRaises(AssertionError):
            verify_record(dict(record, source_revision=baseline['binding']['manifest_sha256']))
        with self.assertRaisesRegex(ValueError, 'own diagnostic version'):
            verify_wled_kernel(730, WLED_MARKER)

    def test_wled_gpu_admits_only_pinned_gles9_ram(self):
        package = {'frontend_build_binding': {'CA_matrix_revision': 1, 'GLES_suite_tests': 9}}
        binding = {
            'library_sha256': '0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607',
            'blob_sha256': '4277931a612742a2eb2e02f67af8c2083116a13686c7594e7d0a3b5e87f0cf27',
            'manifest_sha256': '7c5da6c1025885e8e2c4c988475c138ec2a43d135987b88a316020f26c27cf46',
        }
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'gles')
        new_binding = dict(
            library_sha256='6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325',
            blob_sha256='25a417018c2febc28f1c72fc14cb0d0a9ab0c4a3ec28536cbcb13406f01b6543',
            manifest_sha256='60680d669f037146a85b0a50e3353c0a2f991ff8be9ff9eedf4f734ef4191281')
        self.assertEqual(validate_wled_gpu_ram(package, new_binding), 'gles')
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(package, dict(new_binding, library_sha256=binding['library_sha256']))
        for bad in (dict(package, exception_observer={}), dict(package, metal_resources={}),
                    {'frontend_build_binding': {'CA_matrix_revision': 1, 'GLES_suite_tests': 8}}):
            with self.subTest(package=bad), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(bad, binding)
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(package, dict(binding, blob_sha256='0'*64))

    def test_wled_metal_requires_pinned_background_suite(self):
        binding = {
            'blob_sha256': 'cb1960218b50b7d908eda964fad198a9394941c0db5b1af9cee7e84471837b0a',
            'manifest_sha256': '27c39aa515bbfd247012b887a163558421f21bbe38ffce54ce2182be422ad459',
        }
        observer = dict(target_process='backboardd', background_full_suite=True, presentation_probe=True,
                        cif10_reference_cases=6, cif10_reference_shared_encoder=True)
        resources = {'native_presentation_required_tests': 21}
        package = dict(exception_observer=observer, metal_resources=resources)
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'native')
        for bad in (dict(observer, registration_only=True), dict(observer, target_process='AegirPoster'),
                    dict(observer, cif10_reference_cases=5)):
            with self.subTest(observer=bad), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(dict(package, exception_observer=bad), binding)
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(package, dict(binding, manifest_sha256='0'*64))

    def test_wled_poster_requires_its_own_native_proof(self):
        binding = {
            'blob_sha256': 'ab1990dfdaef042c06588d21c78fea999195efeff62b62916d95060879e78f3d',
            'manifest_sha256': '1e346d62937f85b21ae807f0164aad7e97c9bb2d94cb79f7e8b05cfc3cd6a4a1',
        }
        observer = dict(target_process='AegirPoster', registration_only=True,
                        prior_native_hardware_proof={'version': WLED_VERSION},
                        png_filter_encoder=True, snapshot_method_observer=True)
        self.assertEqual(validate_wled_gpu_ram({'exception_observer': observer}, binding), 'poster')
        for bad in (dict(observer, prior_native_hardware_proof={'version': 730}),
                    dict(observer, registration_only=False), dict(observer, png_filter_encoder=False)):
            with self.subTest(observer=bad), self.assertRaises(AssertionError):
                validate_wled_gpu_ram({'exception_observer': bad}, binding)

    def test_cache_budget_native_rejects_old_or_mixed_payloads(self):
        manifest = Path(__file__).resolve().parents[2] / 'build/poster-cache-budget-metal-20261124/reference-ram/manifest.json'
        if not manifest.is_file():
            self.skipTest('Local signed cache-budget native RAM is unavailable')
        package = json.loads(manifest.read_bytes())
        binding = dict(library_sha256=package['library_sha256'], blob_sha256=package['blob_sha256'],
                       manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest())
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'native')
        for field in ('library_sha256', 'blob_sha256', 'manifest_sha256'):
            with self.subTest(binding=field), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(package, dict(binding, **{field: '0'*64}))
        mutations = [
            ('metal_resources', 'gles_library_sha256', '0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607'),
            ('metal_resources', 'library_sha256', '0'*64),
            ('metal_resources', 'source_files', {}),
            ('metal_resources', 'immutable_program_cache_identity', '0'*64),
            ('exception_observer', 'library_sha256', '0'*64),
            ('exception_observer', 'registration_only', True),
            ('exception_observer', 'cif10_reference_cases', 5),
            ('exception_observer', 'program_cache_inventory', dict(package['exception_observer']['program_cache_inventory'], conditional_prime=True)),
        ]
        for owner, field, value in mutations:
            changed = copy.deepcopy(package)
            changed[owner][field] = value
            with self.subTest(owner=owner, field=field), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(changed, binding)
        changed = copy.deepcopy(package)
        changed['frontend_build_binding']['program_cache_build_identity']['build_identity'] = '0'*64
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(changed, binding)

    def test_cache_budget_primer_is_bound_to_distinct_reviewed_ram(self):
        manifest = Path(__file__).resolve().parents[2] / 'build/poster-cache-budget-metal-prime-20261124/reference-ram/manifest.json'
        if not manifest.is_file():
            self.skipTest('Local signed cache-budget conditional primer RAM is unavailable')
        package = json.loads(manifest.read_bytes())
        binding = dict(library_sha256=package['library_sha256'], blob_sha256=package['blob_sha256'],
                       manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest())
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'native')
        changed = copy.deepcopy(package)
        changed['exception_observer']['program_cache_inventory']['conditional_prime'] = False
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(changed, binding)
        changed = copy.deepcopy(package)
        changed['exception_observer']['program_cache_prime'] = False
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(changed, binding)
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(package, dict(binding, blob_sha256='a8821cead75c078861eed21733c6366b594c8de4a631c3848b8bfd55d3cb66bf'))

    def test_new_poster_refuses_old_gles_native_and_png_proofs(self):
        manifest = Path(__file__).resolve().parents[2] / 'build/poster-cache-budget-wallpaper-20261124/ram/manifest.json'
        if not manifest.is_file():
            self.skipTest('Local signed cache-budget original poster RAM is unavailable')
        package = json.loads(manifest.read_bytes())
        binding = dict(library_sha256=package['library_sha256'], blob_sha256=package['blob_sha256'],
                       manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest())
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'poster')
        mutations = [
            ('prior_native_hardware_proof', 'gles_library_sha256', '0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607'),
            ('prior_native_hardware_proof', 'native_run_token', '97fa008727dbc047'),
            ('prior_native_hardware_proof', 'native_tests', package['exception_observer']['prior_native_hardware_proof']['native_tests'][:20]),
            ('png_encoder_prior_proof', 'GLES_sha256', '0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607'),
            ('png_encoder_prior_proof', 'reference_token', '97fa008727dbc047'),
        ]
        for proof, field, value in mutations:
            changed = copy.deepcopy(package)
            changed['exception_observer'][proof][field] = value
            with self.subTest(proof=proof, field=field), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(changed, binding)

    def test_minimal_poster_keeps_full_proofs_without_extra_observers(self):
        manifest = Path(__file__).resolve().parents[2] / 'build/poster-cache-budget-minimal-20261125/ram/manifest.json'
        if not manifest.is_file():
            self.skipTest('Local signed minimal original poster RAM is unavailable')
        package = json.loads(manifest.read_bytes())
        binding = dict(library_sha256=package['library_sha256'], blob_sha256=package['blob_sha256'],
                       manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest())
        self.assertEqual(validate_wled_gpu_ram(package, binding), 'poster')
        for key in ('original_request_observer','quad_route_observer','poster_state_observer_revision'):
            changed = copy.deepcopy(package)
            changed['exception_observer'][key] = True
            with self.subTest(key=key), self.assertRaises(AssertionError):
                validate_wled_gpu_ram(changed, binding)
        changed = copy.deepcopy(package)
        changed['exception_observer']['prior_native_hardware_proof']['native_tests'] = changed['exception_observer']['prior_native_hardware_proof']['native_tests'][:20]
        with self.assertRaises(AssertionError):
            validate_wled_gpu_ram(changed, binding)

    def test_poster_scanout_rejects_mixed_timing_sources_and_control(self):
        root = Path(__file__).resolve().parents[2]
        manifest = root / 'build/poster-cache-budget-minimal-20261125/ram/manifest.json'
        artifact = root / 'build/poster-cache-budget-minimal-20261125/gpu/artifacts.json'
        if not manifest.is_file() or not artifact.is_file():
            self.skipTest('Local exact original poster scanout package is unavailable')
        package = json.loads(manifest.read_bytes())
        record = json.loads(artifact.read_bytes())['artifacts'][0]
        binding = record['ram_library_binding']
        validate_wled_gpu_record(record, package, binding)
        mutations = [
            ('screen_capture_after_seconds', 45),
            ('poster_startup_serial_only', True),
            ('swipe_observation', {}),
            ('thread_sample_observation', {}),
            ('screen_snapshot_binding', dict(record['screen_snapshot_binding'], width=750)),
            ('process_log_binding', dict(record['process_log_binding'], processes=['backboardd'])),
        ]
        for field, value in mutations:
            changed = copy.deepcopy(record)
            changed[field] = value
            with self.subTest(field=field), self.assertRaises((AssertionError,ValueError)):
                validate_wled_gpu_record(changed, package, binding)
        with self.assertRaises(AssertionError):
            validate_wled_gpu_record(record, package, dict(binding, blob_sha256='0'*64))

    def test_audit_includes_upstream_memory_descriptor(self):
        first=configuration(725);next_=configuration(726)
        self.assertEqual(len(first['source_hashes']),79)
        self.assertEqual(len(next_['source_hashes']),79)
        path='iokit/Kernel/IOMemoryDescriptor.cpp'
        self.assertNotEqual(first['source_hashes'][path],next_['source_hashes'][path])
        readonly=configuration(727)
        self.assertEqual(len(readonly['source_hashes']),79)
        self.assertEqual(readonly['source_hashes'][path],next_['source_hashes'][path])
        self.assertNotEqual(readonly['source_hashes']['iokit/Kernel/arm/SweetSurface.cpp'],next_['source_hashes']['iokit/Kernel/arm/SweetSurface.cpp'])
        region64=configuration(728)
        self.assertNotEqual(region64['source_hashes']['iokit/Kernel/arm/SweetSurface.cpp'],readonly['source_hashes']['iokit/Kernel/arm/SweetSurface.cpp'])
        self.assertEqual(region64['source_hashes']['iokit/Kernel/arm/SGSharedMapper.cpp'],readonly['source_hashes']['iokit/Kernel/arm/SGSharedMapper.cpp'])

    def test_readonly_pixels_require_permissions_hashes_and_vm_lifetime(self):
        token='0123456789abcdef';pattern=readonly_hash();bgra=readonly_hash(True)
        text=('SWEET LOOP LIVE proto=1 token='+token+'\n'
          'SWEET SURF READONLY id=9 width=400 height=400 bytes=640000 source=0x112000020 direction=out cpu_write=0 gpu_write=0\n'
          'readonly view id=9 width=400 height=400 source=0x112000020 returned=0x113000020 region=0 prot=1 max=1 write_lock=e00002cf read_lock=0 checked=160000 mismatches=0 hash='+bgra+'\n'
          'readonly sampler id=9 handle=2 phase=0 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+pattern+' source_vm_released=0\n'
          'readonly sampler id=9 handle=2 phase=1 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+pattern+' source_vm_released=1\n'
          'readonly selftest -> PASS phase=held_sampler\nSWEET LOOP ACK FASTBOOT token='+token)
        self.assertEqual(verify_readonly(text,token)['pixels_per_gpu_check'],160000)
        for mutation in (text.replace('prot=1 max=1','prot=3 max=3'),text.replace('write_lock=e00002cf','write_lock=0'),
                         text.replace('source_vm_released=1','source_vm_released=0'),text.replace(pattern,'0'*16)):
            with self.assertRaises(AssertionError):verify_readonly(mutation,token)

    def test_new_library_requires_its_actual_cpu_evidence(self):
        record={'ram_library_binding':{'library_sha256':'new','blob_sha256':'new-blob'}}
        with self.assertRaisesRegex(AssertionError,'RAM CPU prerequisite library differs'):
            require_same_library(record,{})
        with self.assertRaises(AssertionError):
            require_same_library(record,{'ram_library_binding':{'library_sha256':'old','blob_sha256':'old-blob'}})
        require_same_library(record,{'ram_library_binding':record['ram_library_binding']})

    def test_production_readonly_gate_checks_all_paths_offsets_and_lifetimes(self):
        token='0123456789abcdef';rows=['SWEET LOOP LIVE proto=1 token='+token]
        for index,offset in enumerate((32,64,128),9):
            source=0x112000000+offset;returned=0x113000000+offset
            rows+=['SWEET SURF READONLY id=0 width=400 height=400 bytes=640000 source='+hex(source)+' direction=out cpu_write=0 gpu_write=0',
                'readonly view id='+str(index)+' width=400 height=400 source='+hex(source)+' returned='+hex(returned)+' region=0 prot=1 max=1 write_lock=e00002cf read_lock=0 checked=160000 mismatches=0 hash='+readonly_hash(True)]
            bind='production sampler id='+str(index)+' path='+('copy' if offset==32 else 'shared')+' offset='+str(offset)+' pitch=1600 result=1'
            rows.extend([bind,bind])
            rows.append('readonly sampler id='+str(index)+' offset='+str(offset)+' phase=0 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+readonly_hash()+' source_vm_released=0')
            rows.append(bind)
            rows.append('readonly sampler id='+str(index)+' offset='+str(offset)+' phase=1 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+readonly_hash()+' source_vm_released=1')
            rows.append('readonly case offset='+str(offset)+' -> PASS phase=held_sampler')
        rows+=['readonly selftest -> PASS phase=held_sampler','SWEET LOOP ACK FASTBOOT token='+token]
        text='\n'.join(rows)
        self.assertEqual(verify_readonly(text,token)['total_gpu_pixels'],960000)
        for changed in (text.replace('path=copy','path=shared'),text.replace('offset=64 phase=1','offset=64 phase=2'),
                        text.replace('source_vm_released=1','source_vm_released=0'),text.replace('prot=1 max=1','prot=1 max=3')):
            with self.assertRaises(AssertionError):verify_readonly(changed,token)


class ActualFaultReplay(unittest.TestCase):
    def test_new_ram_package_actual_bytes(self):
        path=Path(__file__).resolve().parents[2]/'build/surface-region-r1/ramfs'
        if not path.exists():self.skipTest('New RAM package not installed')
        blob,binding,_=load_ram(path)
        self.assertGreater(len(blob),0)
        changed=dict(binding);changed['library_sha256']='0'*64
        with self.assertRaisesRegex(AssertionError,'RAM library binding differs'):load_ram(path,changed)

    def test_embedded_kernel_and_blob_binding(self):
        path=Path(__file__).resolve().parents[2]/'build/surface-diag-v726/gpu/artifacts.json'
        if not path.is_file():self.skipTest('Packaged v726 input not installed')
        from package_trial import prove_payload
        original=json.loads(path.read_bytes())['artifacts'][0]
        self.assertTrue(prove_payload(original)['complete_payload_reconstructed_from_compiled_kernel'])
        changed=json.loads(json.dumps(original));changed['kernel_sha256']='0'*64
        with self.assertRaisesRegex(AssertionError,'Compiled kernel differs'):prove_payload(changed)
        changed=json.loads(json.dumps(original));changed['blobs']['GPU-PM4']['sha256']='0'*64
        with self.assertRaisesRegex(AssertionError,'Trial firmware/RAM inputs differ'):prove_payload(changed)

    def test_previous_version_private_cannot_admit_v726(self):
        source=Path(os.environ['TEMP'])/'lilac-live-7zbjhric'
        if not source.is_dir():self.skipTest('Preserved v725 private evidence not installed')
        from trial_evidence import verify
        with self.assertRaisesRegex(AssertionError,'Prerequisite version differs'):
            verify(source,'private',version=726)

    def test_clock_bytes_and_protocol(self):
        source = Path(os.environ.get('LILAC_CLOCK_TEST_EVIDENCE', str(Path(os.environ['TEMP']) / 'lilac-v676-clock-20261107-2354')))
        if not source.is_dir(): self.skipTest('Preserved v676 raw evidence not installed')
        verify_clock(source)
        rows = json.loads((source / 'results.json').read_text())
        original = (source / 'cycle-01/serial.raw.log').read_bytes()
        for kind in ('hash', 'duplicate_reply', 'missing_ack', 'wrong_token', 'health', 'simulated'):
            # Keep one-use fixtures under TEMP for the native recycle workflow.
            folder = Path(tempfile.mkdtemp(prefix='lilac-clock-fault-', dir=os.environ['TEMP']))
            (folder / 'cycle-01').mkdir()
            row = json.loads(json.dumps(rows))[0]; raw = original
            if kind == 'hash': raw += b'changed'
            elif kind == 'duplicate_reply': raw = raw.replace(b'reply 0x52503033', b'reply 0x52503032')
            elif kind == 'missing_ack': raw = raw.replace(b'SWEET LOOP ACK FASTBOOT', b'SWEET LOOP BAD FASTBOOT')
            elif kind == 'wrong_token': row['artifact']['token'] = '0' * 16
            elif kind == 'health': row['reset_to_fastboot_seconds'] = 5
            elif kind == 'simulated': row['simulated'] = True
            if kind != 'hash':
                row['log_sha256'] = hashlib.sha256(raw).hexdigest()
                row['checkpoint_log_sha256'] = hashlib.sha256(raw[:row['checkpoint_log_bytes']]).hexdigest()
            (folder / 'cycle-01/serial.raw.log').write_bytes(raw)
            (folder / 'results.json').write_text(json.dumps([row]))
            with self.subTest(kind=kind), self.assertRaises((AssertionError, ValueError)):
                verify_clock(folder)

    def test_owner_root_word_and_state_rejected(self):
        source = Path(os.environ.get('LILAC_OWNER_TEST_EVIDENCE', str(Path(os.environ['TEMP']) / 'lilac-v724-owner-20261111-135928')))
        if not source.is_dir(): self.skipTest('Preserved v724 owner evidence not installed')
        text = (source / 'cycle-01/serial.raw.log').read_text(errors='replace')
        import re
        text = re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', text)
        snapshots = verify_owner(text)
        self.assertEqual(len(snapshots), 2)
        for changed in (text.replace('index=2 value=0x80000641', 'index=2 value=0x80000601'),
                        text.replace('root_reads=512', 'root_reads=511'),
                        text.replace('context=1 identity=1 idle=1', 'context=1 identity=1 idle=0')):
            self.assertNotEqual(changed, text)
            with self.assertRaises(AssertionError): verify_owner(changed)

    def test_gpu_pixels_and_retirement_faults(self):
        evidence = os.environ.get('LILAC_GPU_TEST_LOG')
        if not evidence: self.skipTest('Set LILAC_GPU_TEST_LOG to preserved raw GPU evidence')
        import re
        text = Path(evidence).read_text(errors='replace')
        text = re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', text)
        token = re.search(r'SWEET LOOP LIVE proto=1 token=([0-9a-f]{16})', text)[1]
        report = verify_gpu(text, token)
        self.assertEqual(report['lifecycle_pixels'], 20480)
        for changed in (text.replace('hash=ed689397267e7cf5', 'hash=ed689397267e7cf4'),
                        text.replace('same_names=1', 'same_names=0'),
                        text.replace('source_vm_released=1', 'source_vm_released=0'),
                        text.replace('orphaned=0 active=0 root=0', 'orphaned=1 active=0 root=0')):
            self.assertNotEqual(changed, text)
            with self.assertRaises(AssertionError): verify_gpu(changed, token)


if __name__ == '__main__': unittest.main(verbosity=2)
