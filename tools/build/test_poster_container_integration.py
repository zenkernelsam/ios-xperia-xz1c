"""Actual old-package rejection, CLI guards and C-transcript boundary checks."""
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
import uuid

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/device')]
from poster_container_contract import FIELDS,SOURCES,validate
from poster_container_evidence import inspect
from package_trial import validate_wled_gpu_ram


class ContainerIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path=ROOT/'build/poster-config-r1-20261128/ram/manifest.json'
        raw=path.read_bytes();cls.package=json.loads(raw)
        cls.binding=dict(library_sha256=cls.package['library_sha256'],blob_sha256=cls.package['blob_sha256'],
                         manifest_sha256=hashlib.sha256(raw).hexdigest())

    def contract(self):
        observer=copy.deepcopy(self.package['exception_observer']);observer.pop('poster_config_snapshot_revision',None)
        observer.update(FIELDS)
        for p in SOURCES:observer['source_files'][p]=hashlib.sha256((ROOT/p).read_bytes()).hexdigest()
        return observer

    def test_format_contract_and_source_full_suite_requirements(self):
        observer=self.contract();self.assertEqual(validate(observer),SOURCES)
        for key,value in FIELDS.items():
            bad=copy.deepcopy(observer);bad[key]=not value if type(value) is bool else value+1
            with self.subTest(field=key),self.assertRaises(AssertionError):validate(bad)
        bad=copy.deepcopy(observer);bad['poster_container_snapshot_readonly']=1
        with self.assertRaises(AssertionError):validate(bad)
        for key,value in [('target_process','AegirPoster'),('background_full_suite',False),('registration_only',True),
                          ('presentation_probe',False),('cif10_PNG_reference',False),('cif10_reference_cases',5),
                          ('poster_config_snapshot_revision',1),('program_cache_prime',True)]:
            bad=copy.deepcopy(observer);bad[key]=value
            with self.subTest(role=key),self.assertRaises(AssertionError):validate(bad)
        bad=copy.deepcopy(observer);bad['source_files'].pop('gpu/diagnostics/PosterContainerSnapshot.h')
        with self.assertRaises(AssertionError):validate(bad)

    def test_current_signed_RAM_cannot_gain_container_capability_by_metadata(self):
        self.assertEqual(validate_wled_gpu_ram(self.package,self.binding),'native')
        bad=copy.deepcopy(self.package);bad['exception_observer']=self.contract()
        with self.assertRaisesRegex(AssertionError,'no reviewed signed candidate'):validate_wled_gpu_ram(bad,self.binding)

    def test_actual_builder_rejects_invalid_requests_before_output(self):
        for args,error in [(['--poster-container-snapshot'],'Container snapshot follows complete native21 and PNG6'),
                           (['--poster-container-snapshot','--target-process','backboardd'],'Container snapshot follows complete native21 and PNG6'),
                           (['--poster-container-snapshot','--poster-config-snapshot'],'One metadata collector per boot'),
                           (['--poster-container-snapshot','--program-cache-prime'],'must not prime programs')]:
            output=ROOT/'build'/('rejected-container-'+uuid.uuid4().hex)
            result=subprocess.run([sys.executable,'-B',ROOT/'tools/build/build_exception_observer.py','--output',output,*args],capture_output=True,timeout=10)
            with self.subTest(args=args):
                self.assertNotEqual(result.returncode,0);self.assertIn(error,result.stderr.decode('utf8','replace'))
                self.assertFalse(output.exists())

    def test_actual_C_transcript_current_scope_and_after_PNG6_order(self):
        log=Path(os.environ['LILAC_CONTAINER_HOST_LOG']).read_text()
        roots=[str(Path(os.environ['LILAC_CONTAINER_FIXTURES'])/'normal'/str(i)).replace('\\','/') for i in range(4)]
        token='0123456789abcdef'
        live='SWEET LOOP LIVE proto=1 token='+token+'\n';ack='SWEET LOOP ACK FASTBOOT token='+token+'\n'
        png='SWEET CIF10 REFERENCE END result=0 cases=6 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0\n'
        suite='SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1\n'
        text=live+png+log+suite+ack
        result=inspect(text,token,roots)
        self.assertTrue(result['native_PNG6_then_metadata_then_suite_boundary_order_checked'])
        self.assertTrue(result['exact_PosterBoard_identity_unique'])
        self.assertTrue(inspect('SWEET PCON invalid outside scope\n'+text+'SWEET PCON invalid after ACK\n',token,roots)['catalog_enumeration_complete'])
        invalid=dict(duplicate_live=live+text,ack_first=ack+text,missing_ACK=text.replace(ack,''),
                     before_PNG=live+log+png+suite+ack,after_suite=live+png+suite+log+ack,
                     missing_PNG=text.replace(png,''),missing_suite=text.replace(suite,''),
                     wrong_token=text.replace(token,'fedcba9876543210'),
                     token_prefix=text.replace(token,token+'0'))
        for name,value in invalid.items():
            with self.subTest(boundary=name),self.assertRaises(ValueError):inspect(value,token,roots)


if __name__=='__main__':unittest.main(verbosity=2)
