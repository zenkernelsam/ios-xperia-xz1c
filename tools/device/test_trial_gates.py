"""Fault replay for prerequisites and live diagnostic failures; no device access."""
import hashlib
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
from diagnostic_candidate import configuration, verify_record
from ram_library import require_same_library,load as load_ram
from readonly_pixels import verify as verify_readonly,expected_hash as readonly_hash
from exception_evidence import inspect_scope,verify_renderer


class Gates(unittest.TestCase):
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
                     'SWEET SCATTER READY END result=3', 'sweetgl: caller selftest -> WRONG phase=map'):
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
        with self.assertRaises(AssertionError):configuration(730)

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
