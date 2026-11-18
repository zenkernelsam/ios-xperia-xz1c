"""Host fixtures: exact reference version stays mandatory after adding v730."""
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
import png_encoder_prerequisite as target


class PNGReferenceVersion(unittest.TestCase):
    def fixture(self):
        source='4'*64;kernel='1'*64
        observer=dict(target_process='backboardd',cif10_reference_shared_encoder=True,png_imageio_encoder_source_sha256=source)
        row=dict(artifact=dict(version=730,exception_observer=observer,metal_resources=dict(library_sha256='3'*64)))
        names=('Metal_resources','Metal_queue','AIR_compute','AIR_render','AIR_constants','AIR_XR','Metal_IOSurface','Metal_mipmaps','AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half')
        png=dict(encoder_source_sha256=source,filter_policies_match_original=dict(up=True),
            cases=[dict(filters=dict(up=dict(pixels_equal=True,ICC_equal=True,metadata_equal=True))) for _ in range(6)])
        actual=dict(exceptions={**{name:{} for name in names},'CIF10_PNG_reference':png},
            ram_library_binding=dict(library_sha256='2'*64),token='5'*16,image_sha256='6'*64,log_sha256='7'*64)
        resources=dict(gles_library_sha256='2'*64,library_sha256='3'*64)
        return row,actual,resources,kernel,source

    def test_exact_new_version_is_forwarded_to_full_verifier(self):
        row,actual,resources,kernel,source=self.fixture()
        with patch.object(target,'Path') as path,patch.object(target,'verify',return_value=actual) as verify:
            (path.return_value/'results.json').read_bytes.return_value=json.dumps([row]).encode()
            result=target.inspect('host fixture',resources,kernel,source,version=730)
            verify.assert_called_once_with('host fixture','gpu',kernel,730)
            self.assertEqual(result['reference_token'],'5'*16)

    def test_other_version_and_failed_actual_stage_remain_rejected(self):
        row,actual,resources,kernel,source=self.fixture()
        with patch.object(target,'Path') as path,patch.object(target,'verify') as verify:
            (path.return_value/'results.json').read_bytes.return_value=json.dumps([row]).encode()
            with self.assertRaises(AssertionError):target.inspect('host fixture',resources,kernel,source,version=729)
            verify.assert_not_called()
            verify.side_effect=AssertionError('STOP cannot become a reference')
            with self.assertRaises(AssertionError):target.inspect('host fixture',resources,kernel,source,version=730)


if __name__=='__main__':unittest.main()
