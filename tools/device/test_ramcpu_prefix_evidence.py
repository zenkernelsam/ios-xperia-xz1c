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


if __name__ == '__main__':
    unittest.main(verbosity=2)
