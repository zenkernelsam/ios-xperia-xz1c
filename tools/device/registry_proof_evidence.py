"""Bind original short registration logs to the exact verified full native stage."""
from native_proof import validate,digest
def verify(text,token,proof):
    validate(proof);scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert 'SWEET ENV: AegirPoster += DYLD_INSERT_LIBRARIES=/dev/sglram/e' in scope
    markers=[
        'SWEET METAL REGISTRY proof token='+proof['native_run_token']+' log_sha='+proof['native_run_log_sha256']+' external_hardware_stage=1',
        'SWEET METAL REGISTRY kernel_sha='+proof['kernel_sha256'],
        'SWEET METAL REGISTRY gles_sha='+proof['gles_library_sha256']+' metal_sha='+proof['metal_library_sha256'],
        'SWEET METAL REGISTRY proof_sha='+digest(proof)+' long_probes_in_constructor=0']
    for marker in markers:assert marker in scope,'Embedded prior hardware proof absent/mismatched'
    assert 'SWEET METAL MIPS begin' not in scope and 'SWEET AIR XR begin' not in scope and 'SWEET METAL PROBES begin' not in scope
    return dict(exact_prior_native_hardware_proof_logged=True,proof_sha256=digest(proof),prior_native_token=proof['native_run_token'],
        constructor_no_long_diagnostic_probes=True,current_original_UI_still_requires_separate_evidence=True)
