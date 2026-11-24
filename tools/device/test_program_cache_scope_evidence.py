import unittest
from program_cache_scope_evidence import verify


class ScopeEvidence(unittest.TestCase):
    token = '0123456789abcdef'
    spec = {'identity': 'a'*64, 'directory': '/private/var/tmp/sgm-'+'a'*32}

    def capture(self):
        return '\n'.join(['SWEET LOOP LIVE proto=1 token='+self.token,
            'SWEET PROGRAM CACHE SCOPE identity='+self.spec['identity'],
            'SWEET PROGRAM CACHE SCOPE result=0 inherited=0 directory='+self.spec['directory']+' mode=1777 old_files_preserved=1',
            'SWEET METAL PROBES begin process=backboardd',
            'SWEET LOOP ACK FASTBOOT token='+self.token])

    def test_current_scope_before_native_work(self):
        self.assertEqual(verify(self.capture(), self.token, self.spec)['complete_samples'], 1)

    def test_faults_and_late_scope_refused(self):
        text = self.capture()
        for bad in (text.replace('result=0', 'result=-1'), text.replace('mode=1777', 'mode=0777'),
                    text.replace('old_files_preserved=1', 'old_files_preserved=0'),
                    text.replace('directory=/private/var/tmp/', 'directory=/tmp/'),
                    text.replace('SWEET PROGRAM CACHE SCOPE identity=', 'SWEET METAL PROBES begin\nSWEET PROGRAM CACHE SCOPE identity=')):
            with self.subTest(), self.assertRaises(AssertionError):
                verify(bad, self.token, self.spec)

    def test_aegir_relaunch_reinitializes_scope(self):
        scope_lines = self.capture().splitlines()
        epoch = ['exec AegirPoster', *['|AegirPoster| ' + line for line in scope_lines[1:3]],
                 '|AegirPoster| SWEET METAL REGISTRY proof_sha=example']
        crash = 'CRASH AegirPoster[301] sig 9'
        verify_epoch = lambda rows: verify('\n'.join([scope_lines[0], *rows, scope_lines[-1]]),
                                          self.token, self.spec, 'AegirPoster')
        self.assertEqual(verify_epoch([*epoch, crash, *epoch])['complete_samples'], 2)
        for rows in ([*epoch, *epoch[1:3]], [*epoch, 'exec AegirPoster', epoch[-1]],
                     [*epoch, crash, epoch[-1]], [*epoch, 'exec AegirPoster', epoch[1]],
                     [epoch[0], epoch[1], epoch[2].replace('result=0', 'result=-1')]):
            with self.subTest(rows=rows), self.assertRaises(AssertionError):
                verify_epoch(rows)


if __name__ == '__main__':
    unittest.main(verbosity=2)
