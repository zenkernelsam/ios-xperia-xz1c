import unittest
from ca_matrix_evidence import verify,UUID

class MatrixEvidence(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,
            'SWEET CA MATRIX INSTALL result=0 uuid='+UUID+' table=0x1d5b48a10 cases=0x18855e1e4 slots=2 original_empty_checked=1 code_anchors=2 rollback=1']
        lines += [f'SWEET CA MATRIX phase={phase} function={"0x14" if phase<3 else "0x2d"} context=7 bytes=256 tolerance=1 max_diff=1 mismatches=0 guard_fail=0 error=0 hash={phase+1:016x} production_GLI=1' for phase in range(6)]
        lines += ['SWEET CA MATRIX END result=0 phases=6 checked_bytes=1536 guard_bytes=192 context_disposed=1 production_GLI=1 matrix_slots=2','SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(lines)
    def test_six_actual_phases(self):self.assertEqual(verify(self.sample(),self.token)['checked_bytes'],1536)
    def test_missing_wrong_identity_pixel_guard_and_phase_rejected(self):
        text=self.sample()
        for bad in (text.replace(UUID,'0'*32),text.replace('max_diff=1','max_diff=2',1),
            text.replace('context=7','context=8',1),text.replace('guard_fail=0','guard_fail=1',1),
            text.replace('mismatches=0','mismatches=1',1),text.replace('phase=5','phase=4',1),
            text.replace('context_disposed=1','context_disposed=0'),text.replace('code_anchors=2','code_anchors=0')):
            with self.assertRaises(AssertionError):verify(bad,self.token)

if __name__=='__main__':unittest.main()
