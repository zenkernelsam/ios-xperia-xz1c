"""Require the source-derived cache root before any native/original work."""
import re


def verify(text, token, spec):
    start = 'SWEET LOOP LIVE proto=1 token='+token
    ack = 'SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(start) == text.count(ack) == 1
    scope = text[text.index(start):text.index(ack)]
    identities = list(re.finditer(r'SWEET PROGRAM CACHE SCOPE identity=([0-9a-f]{64})', scope))
    roots = list(re.finditer(r'SWEET PROGRAM CACHE SCOPE result=(-?\d+) inherited=([01]) directory=(\S+) mode=1777 old_files_preserved=1', scope))
    assert len(identities) == len(roots) == 1
    assert identities[0].group(1) == spec['identity']
    assert roots[0].group(1) == '0' and roots[0].group(3) == spec['directory']
    assert identities[0].end() < roots[0].start()
    for marker in ('SWEET METAL PROBES begin', 'SWEET METAL REGISTRY proof', 'SWEET AIR RENDER compile_begin'):
        at = scope.find(marker)
        if at >= 0:
            assert roots[0].end() < at
    return dict(complete_samples=1, incomplete_samples=0, directory=spec['directory'],
                source_scope_identity=spec['identity'], old_files_preserved=True)
