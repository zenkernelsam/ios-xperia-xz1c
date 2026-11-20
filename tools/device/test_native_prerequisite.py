"""Fault injection for proof identity, exact libraries/sources and required replay."""
import copy,unittest,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from types import SimpleNamespace
from unittest.mock import patch
from native_proof import NATIVE_TESTS,validate,digest,header
from registry_proof_evidence import verify as verify_registry
from run_trial import prerequisites
class NativePrerequisite(unittest.TestCase):
    def setUp(self):
        self.resources=dict(gles_library_sha256='2'*64,library_sha256='3'*64,source_files={'gpu/metal/MetalResources.mm':'9'*64})
        self.proof=dict(revision=1,version=729,kernel_sha256='1'*64,gles_library_sha256='2'*64,metal_library_sha256='3'*64,
            metal_source_files_sha256=digest(self.resources['source_files']),native_run_token='a'*16,native_run_image_sha256='4'*64,
            native_run_log_sha256='5'*64,native_run_process='backboardd',native_tests=list(NATIVE_TESTS))
    def test_schema_and_exact_library_binding(self):
        validate(self.proof,self.resources);self.assertIn(('"'+digest(self.proof)+'"').encode(),header(self.proof))
        for field,value in [('version',728),('native_run_process','AegirPoster'),('native_tests',list(NATIVE_TESTS[:-1])),('native_run_token','BAD'),
            ('metal_library_sha256','6'*64),('gles_library_sha256','7'*64),('metal_source_files_sha256','8'*64)]:
            wrong=copy.deepcopy(self.proof);wrong[field]=value
            with self.assertRaises(AssertionError):validate(wrong,self.resources)
        half=copy.deepcopy(self.resources);half['AIR_frontend']={'local_narrow_conversion_adaptation':True,'local_half_storage_and_rounding_adaptation':True}
        with self.assertRaises(AssertionError):validate(self.proof,half)
        complete=copy.deepcopy(self.proof);complete['native_tests']+=['AIR_narrow','AIR_half'];validate(complete,half)
        half['native_MRT_and_XR_blend']=True
        with self.assertRaises(AssertionError):validate(complete,half)
        complete['native_tests'].append('Metal_MRT');validate(complete,half)
    def test_short_registration_requires_replay(self):
        record=dict(version=729,trial_mode='gpu',kernel_sha256='1'*64,metal_resources=self.resources,
            exception_observer={'registration_only':True,'prior_native_hardware_proof':self.proof})
        args=SimpleNamespace(private_evidence='p',owner_evidence='o',ready_evidence='r',cpu_evidence='c',rpm_evidence='rpm',gles_evidence='g',native_evidence=None)
        with patch('run_trial.verify',return_value={'ram_library_binding':{'library_sha256':'2'*64}}),patch('run_trial.verify_clock',return_value={}),patch('run_trial.require_same_library'):
            with self.assertRaisesRegex(AssertionError,'complete native hardware'):prerequisites(record,args)
            args.native_evidence='native'
            with patch('run_trial.verify_native_prerequisite',return_value=self.proof) as replay:
                self.assertEqual(prerequisites(record,args)['native'],self.proof);replay.assert_called_once_with('native',self.resources,'1'*64,729)
            wrong=copy.deepcopy(self.proof);wrong['native_run_log_sha256']='0'*64
            with patch('run_trial.verify_native_prerequisite',return_value=wrong),self.assertRaisesRegex(AssertionError,'Embedded native proof'):prerequisites(record,args)
    def test_new_depth_backend_rejects_old_full_native_proof(self):
        resources=copy.deepcopy(self.resources);resources['native_depth32_attachment']=True
        proof=copy.deepcopy(self.proof);proof['native_tests']+=['AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half','Clear_cache','IR3_cache']
        with self.assertRaisesRegex(AssertionError,'Depth32'):validate(proof,resources)
        proof['native_tests'].append('Metal_depth32');validate(proof,resources)
        resources['native_stencil8_attachment']=True
        with self.assertRaisesRegex(AssertionError,'Stencil8'):validate(proof,resources)
        proof['native_tests'].append('Metal_stencil8');validate(proof,resources)
        resources['native_memoryless_attachments']=True
        with self.assertRaisesRegex(AssertionError,'Memoryless'):validate(proof,resources)
        proof['native_tests'].append('Metal_memoryless');validate(proof,resources)
        resources['native_multisample_attachments']=True
        with self.assertRaisesRegex(AssertionError,'Multisample'):validate(proof,resources)
        proof['native_tests'].append('Metal_multisample');validate(proof,resources)
    def test_current_log_identity(self):
        p=self.proof;token='f'*16
        scope='\n'.join(['SWEET LOOP LIVE proto=1 token='+token,'SWEET ENV: AegirPoster += DYLD_INSERT_LIBRARIES=/dev/sglram/e',
            'SWEET METAL REGISTRY proof token='+p['native_run_token']+' log_sha='+p['native_run_log_sha256']+' external_hardware_stage=1',
            'SWEET METAL REGISTRY kernel_sha='+p['kernel_sha256'],
            'SWEET METAL REGISTRY gles_sha='+p['gles_library_sha256']+' metal_sha='+p['metal_library_sha256'],
            'SWEET METAL REGISTRY proof_sha='+digest(p)+' long_probes_in_constructor=0','SWEET LOOP ACK FASTBOOT token='+token])
        self.assertTrue(verify_registry(scope,token,p)['exact_prior_native_hardware_proof_logged'])
        for wrong in (scope.replace(p['native_run_log_sha256'],'6'*64),scope.replace(p['metal_library_sha256'],'7'*64),scope.replace('long_probes_in_constructor=0','long_probes_in_constructor=1')):
            with self.assertRaises(AssertionError):verify_registry(wrong,token,p)
if __name__=='__main__':unittest.main()
