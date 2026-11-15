import unittest
from ramcpu_prefix_evidence import verify


class PrefixTests(unittest.TestCase):
    token = '0123456789abcdef'
    library = 'SWEET GLRAM: library read=0 bytes=14729008 fnv=322975a3af2792d0 ro=1'

    def capture(self, replay=False):
        block = 'SWEET LOOP READY proto=1 token=' + self.token + ' mode=graceful pon=0x06\n'
        block += 'SWEET GLRAM: mount result=0\n' + self.library + '\n'
        body = block + 'Darwin Kernel Version 22.6.0\n'
        if replay:
            body += block + 'VERMEER DEBUG: replay retained RAM log\n'
        return body + 'SWEET LOOP LIVE proto=1 token=' + self.token + '\nSWEET LOOP ACK FASTBOOT token=' + self.token

    def test_single_read(self):
        self.assertEqual(verify(self.capture().encode(), self.token, self.library)['raw_library_reads'], 1)

    def test_explicit_exact_replay(self):
        result = verify(self.capture(True).encode(), self.token, self.library)
        self.assertTrue(result['explicit_exact_prefix_replay'])
        self.assertEqual(result['logical_library_reads'], 1)
        self.assertEqual(result['raw_library_reads'], 2)

    def test_nonidentical_or_unmarked_prefix_refused(self):
        for text in [self.capture(True).replace('mount result=0', 'mount result=1', 1),
                     self.capture(True).replace('VERMEER DEBUG: replay retained RAM log', ''),
                     self.capture(True).replace('Darwin Kernel Version', '')]:
            with self.assertRaises(AssertionError):
                verify(text.encode(), self.token, self.library)

    def test_changed_hash_or_duplicate_read_refused(self):
        for text in [self.capture(True).replace('fnv=322975a3af2792d0', 'fnv=0000000000000000', 1),
                     self.capture().replace(self.library, self.library + '\n' + self.library)]:
            with self.assertRaises(AssertionError):
                verify(text.encode(), self.token, self.library)

    def test_foreign_token_or_another_boot_refused(self):
        for text in [self.capture(True).replace(self.token, '1111111111111111', 1),
                     self.capture(True).replace('Darwin Kernel Version', 'Darwin Kernel Version\nDarwin Kernel Version')]:
            with self.assertRaises(AssertionError):
                verify(text.encode(), self.token, self.library)

    def test_raw_bytes_not_lossy_text(self):
        raw = self.capture(True).encode().replace(b'mount result=0', b'mount result=\xff', 1)
        raw = raw.replace(b'mount result=0', b'mount result=\xfe', 1)
        with self.assertRaises(AssertionError):
            verify(raw, self.token, self.library)

    def test_single_live_read_still_valid(self):
        raw = self.capture().replace(self.library + '\n', '')
        raw = raw.replace('SWEET LOOP ACK', self.library + '\nSWEET LOOP ACK').encode()
        self.assertEqual(verify(raw, self.token, self.library)['raw_library_reads'], 1)

    def partial_capture(self):
        prefix = 'SWEET LOOP READY proto=1 token=' + self.token + ' mode=graceful pon=0x06\n'
        prefix += 'SWEET GLRAM: inflate begin packed=123 raw=33554432 virtual=1\n'
        first = prefix + 'Darwin Kernel Version 22.6.0\nSWEET GLRAM: mount begin\n'
        replay = prefix + 'VERMEER DEBUG: replay retained RAM log\n\n'
        live = 'SWEET LOOP LIVE proto=1 token=' + self.token + '\n'
        return first + replay + live + self.library + '\nSWEET LOOP ACK FASTBOOT token=' + self.token

    def test_exact_partial_replay_then_one_live_read(self):
        result = verify(self.partial_capture().encode(), self.token, self.library)
        self.assertTrue(result['explicit_exact_partial_prefix_replay'])
        self.assertTrue(result['active_read_not_counted_as_replay'])
        self.assertFalse(result['explicit_exact_prefix_replay'])
        self.assertEqual(result['raw_library_reads'], 1)

    def test_partial_replay_faults_refused(self):
        original = self.partial_capture()
        cases = [original.replace('packed=123', 'packed=124', 1),
                 original.replace('VERMEER DEBUG: replay retained RAM log', ''),
                 original.replace('replay retained RAM log\n\n', 'replay retained RAM log\nother record\n'),
                 original.replace('Darwin Kernel Version', 'Darwin Kernel Version\nDarwin Kernel Version'),
                 original.replace('fnv=322975a3af2792d0', 'fnv=0000000000000000'),
                 original.replace(self.library, self.library + '\n' + self.library)]
        for text in cases:
            with self.subTest(text=text), self.assertRaises(AssertionError):
                verify(text.encode(), self.token, self.library)

    def test_partial_replay_raw_bytes_are_exact(self):
        raw = self.partial_capture().encode().replace(b'packed=123', b'packed=\xff23', 1)
        raw = raw.replace(b'packed=123', b'packed=\xfe23', 1)
        with self.assertRaises(AssertionError):
            verify(raw, self.token, self.library)


if __name__ == '__main__':
    unittest.main(verbosity=2)
