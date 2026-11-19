"""Require all matrix GLI phases and the exact local QuartzCore installation."""
import re

UUID='9e40b2eb260c3af1ae45a1a551a8c1b9'

def verify(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)]
    install='SWEET CA MATRIX INSTALL result=0 uuid='+UUID+' table=0x1d5b48a10 cases=0x18855e1e4 slots=2 original_empty_checked=1 code_anchors=2 rollback=1'
    assert install in scope
    assert not re.search(r'SWEET CA MATRIX (?:INSTALL|END) result=[1-9]',scope)
    phases=list(re.finditer(r'SWEET CA MATRIX phase=([0-5]) function=(0x14|0x2d) context=(\d+) bytes=256 tolerance=1 max_diff=([01]) mismatches=0 guard_fail=0 error=0 hash=([0-9a-f]{16}) production_GLI=1',scope))
    assert len(phases)==6 and [int(p[1]) for p in phases]==list(range(6))
    assert [p[2] for p in phases]==['0x14']*3+['0x2d']*3
    identities={int(p[3]) for p in phases};assert len(identities)==1 and next(iter(identities))>0
    assert scope.count('SWEET CA MATRIX phase=')==6
    ends=list(re.finditer(r'SWEET CA MATRIX END result=0 phases=6 checked_bytes=1536 guard_bytes=192 context_disposed=1 production_GLI=1 matrix_slots=2',scope))
    assert len(ends)==1 and phases[-1].end()<ends[0].start()
    return dict(complete_samples=1,incomplete_samples=0,phases=6,checked_bytes=1536,guard_bytes=192,
        exact_QuartzCore_UUID=UUID,original_two_empty_slots_verified=True,
        production_GL_source_link_draw_and_framebuffer_fetch_used=True,
        independent_CPU_reference_tolerance_LSB=1,max_difference_LSB=max(int(p[4]) for p in phases),
        raw_hashes=[p[5] for p in phases],context_disposed=True,full_UI_not_inferred=True)
