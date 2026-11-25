import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from contextlib import contextmanager

from native_png_prerequisite import verify


@contextmanager
def retained_fixture():
    # Disposal is the caller's native Windows/Mac recycle operation.
    yield tempfile.mkdtemp(prefix="lilac-native-png-fixture-")


class CombinedNativePNG(unittest.TestCase):
    def test_full_original_verifier_runs_once_and_both_proofs_share_raw(self):
        with retained_fixture() as directory:
            root=Path(directory).resolve()
            artifact=dict(exception_observer=dict(target_process="backboardd",background_full_suite=True,
                                                   cif10_reference_shared_encoder=True))
            (root/"results.json").write_text(json.dumps([dict(artifact=artifact)]))
            native=dict(native_run_token="a"*16,native_run_log_sha256="b"*64)
            png=dict(reference_token="a"*16,reference_raw_sha256="b"*64)
            evidence=dict(original_raw_completely_verified=True)
            with patch("native_png_prerequisite.verify_trial",return_value=evidence) as full, \
                 patch("native_png_prerequisite.native_from_verified",return_value=native) as n, \
                 patch("native_png_prerequisite.png_from_verified",return_value=png) as p:
                self.assertEqual(verify(root,{},"k","s",731),(native,png))
                full.assert_called_once_with(root,"gpu","k",731)
                n.assert_called_once_with(artifact,evidence,{})
                p.assert_called_once_with(artifact,evidence,{},"k","s")
            for failure in (AssertionError("raw pixel changed"),MemoryError("bounded host failure")):
                with patch("native_png_prerequisite.verify_trial",side_effect=failure), \
                     patch("native_png_prerequisite.native_from_verified") as n, \
                     patch("native_png_prerequisite.png_from_verified") as p:
                    with self.assertRaises(type(failure)):
                        verify(root,{},"k","s",731)
                    n.assert_not_called();p.assert_not_called()
            with patch("native_png_prerequisite.verify_trial",return_value=evidence), \
                 patch("native_png_prerequisite.native_from_verified",return_value=native), \
                 patch("native_png_prerequisite.png_from_verified",return_value=dict(png,reference_raw_sha256="c"*64)):
                with self.assertRaises(AssertionError):
                    verify(root,{},"k","s",731)


if __name__=="__main__":
    unittest.main()
