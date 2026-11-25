"""Validate one original full GPU run once for its native21 and PNG6 metadata."""

import json
from pathlib import Path

from trial_evidence import verify as verify_trial
from native_prerequisite import from_verified as native_from_verified
from png_encoder_prerequisite import from_verified as png_from_verified


def verify(directory,resources,kernel_sha256,encoder_source_sha256,version):
    root=Path(directory).resolve()
    artifact=json.loads((root/"results.json").read_bytes())[-1]["artifact"]
    observer=artifact.get("exception_observer",{})
    assert observer.get("target_process")=="backboardd" and observer.get("background_full_suite")
    assert not observer.get("registration_only") and observer.get("cif10_reference_shared_encoder")
    # Every artifact/payload/source binding, raw byte, pixel, guard, fence,
    # current token and return is recomputed by this unchanged full verifier.
    # No persisted proof or process-wide cache replaces that work.
    evidence=verify_trial(root,"gpu",kernel_sha256,version)
    native=native_from_verified(artifact,evidence,resources)
    png=png_from_verified(artifact,evidence,resources,kernel_sha256,encoder_source_sha256)
    assert native["native_run_token"]==png["reference_token"]
    assert native["native_run_log_sha256"]==png["reference_raw_sha256"]
    return native,png
