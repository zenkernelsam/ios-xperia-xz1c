"""Require the same-process real GLES pass before the complete background suite."""
import re
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert 'SWEET ENV: backboardd += DYLD_INSERT_LIBRARIES=/dev/sglram/m:/dev/sglram/e' in scope
    assert 'SWEET ENV: AegirPoster += DYLD_INSERT_LIBRARIES=/dev/sglram/e' not in scope
    assert not re.search(r'SWEET (?:GLES SUITE|METAL PROBES) END(?: process=backboardd)? result=[1-9]',scope)
    assert not re.search(r'SWEET (?:AIR (?:COMPUTE|RENDER|CONSTANTS|XR)|METAL (?:RESOURCE|QUEUE|SURFACE|MIPS)) END result=[1-9]',scope)
    installed=list(re.finditer(r'SWEET EXCEPTION observer installed process=backboardd limit=16 original_chain=1',scope))
    scheduled=list(re.finditer(r'SWEET METAL PROBES scheduled process=backboardd constructor_waited=0 background=1',scope))
    ready=list(re.finditer(r'SWEET GLES SUITE END result=0 tests=8 state=2 original_tests=1',scope))
    begins=list(re.finditer(r'SWEET METAL PROBES begin process=backboardd GLES_state=2 full_suite=1 background=1',scope))
    ends=list(re.finditer(r'SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1',scope))
    assert len(installed)==len(scheduled)==len(ready)==len(begins)==len(ends)==1,'Exactly one current-process suite required'
    assert installed[0].start()<scheduled[0].start()<begins[0].start()<ends[0].start() and ready[0].end()<begins[0].start()
    block=scope[begins[0].end():ends[0].start()]
    for marker in ('SWEET METAL RESOURCE END result=0','SWEET METAL QUEUE END result=0','SWEET AIR COMPUTE END result=0','SWEET AIR RENDER END result=0',
        'SWEET AIR CONSTANTS END result=0','SWEET AIR XR END result=0','SWEET METAL SURFACE END result=0','SWEET METAL MIPS END result=0'):
        assert marker in block,marker
    assert 'SWEET METAL DISCOVERY END' not in block
    return dict(background_probe_process='backboardd',actual_GLES_suite_before_native_work=True,all_native_probes_present=True,
        constructor_did_not_wait_for_probe=True,original_Aegir_not_injected_or_registered=True,probe_suites=1,full_UI_not_verified=True)
