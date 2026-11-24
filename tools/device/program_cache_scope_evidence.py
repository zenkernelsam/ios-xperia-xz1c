"""Require the source-derived cache root before any native/original work."""
import re


def verify_aegir(scope, spec):
    prefix = '|AegirPoster| '
    identity = 'SWEET PROGRAM CACHE SCOPE identity=' + spec['identity']
    root = re.compile(r'SWEET PROGRAM CACHE SCOPE result=0 inherited=[01] directory='
                      + re.escape(spec['directory']) + r' mode=1777 old_files_preserved=1')
    launch = 0
    confirmed = 0
    state = 'awaiting_exec'
    for line in scope.splitlines():
        if line == 'exec AegirPoster':
            assert state != 'pending', 'Cache initialization was interrupted'
            launch += 1
            state = 'awaiting_identity'
        elif line.startswith('CRASH AegirPoster['):
            assert state != 'pending', 'Cache initialization was interrupted'
            state = 'awaiting_exec'
        elif line.startswith(prefix):
            message = line[len(prefix):]
            if message == identity:
                assert state == 'awaiting_identity', 'Cache identity repeated or has no launch'
                state = 'pending'
            elif message.startswith('SWEET PROGRAM CACHE SCOPE '):
                assert state == 'pending' and root.fullmatch(message), 'Aegir cache root differs'
                state = 'ready'
                confirmed += 1
            elif message.startswith(('SWEET PROGRAM CACHE ', 'SWEET METAL REGISTRY proof',
                                     'SWEET AIR RENDER compile_begin')):
                assert state == 'ready', 'Aegir work preceded its cache initialization'
    assert confirmed and state != 'pending', 'No complete Aegir cache initialization'
    return dict(complete_samples=confirmed, incomplete_samples=0, observed_launches=launch,
                directory=spec['directory'], source_scope_identity=spec['identity'],
                old_files_preserved=True)


def verify(text, token, spec, target_process=None):
    start = 'SWEET LOOP LIVE proto=1 token='+token
    ack = 'SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(start) == text.count(ack) == 1
    begin, end = text.index(start), text.index(ack)
    assert begin < end
    scope = text[begin:end]
    if target_process == 'AegirPoster':
        return verify_aegir(scope, spec)
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
