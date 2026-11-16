import hashlib
import struct
import unittest
from program_request_evidence import decode_record, extract


class ProgramRequestEvidenceTests(unittest.TestCase):
    token = '0123456789abcdef'
    spec = {'context_identity': '01' + '00'*31, 'framing_revision': 2, 'chunk_bytes': 32, 'max_line_bytes': 180}

    def record(self, fs):
        identity = bytes.fromhex(self.spec['context_identity'])
        config = bytes(316)
        payload = config + b'v'*4840 + b'f'*fs
        key = hashlib.sha256(identity + struct.pack('<III', 4840, fs, 316) + payload).digest()
        return b'SGPB0001' + struct.pack('<8I', 1, 1, 0, len(payload), 4840, fs, 316, 0) + identity + key + hashlib.sha256(payload).digest() + payload

    def capture(self, framing=2):
        lines = ['SWEET LOOP LIVE proto=1 token='+self.token]
        total = 0
        for index, fs in enumerate((86136, 45773)):
            record = self.record(fs)
            info = decode_record(record, self.spec)
            total += len(record)
            if framing == 2:
                lines += [f'SWEET PROGRAM REQUEST begin index={index} bytes={len(record)} vertex_bytes=4840 fragment_bytes={fs} read_only=1 chunks=32',
                          f"SWEET PROGRAM REQUEST identity index={index} value={self.spec['context_identity']}",
                          f"SWEET PROGRAM REQUEST key index={index} value={info['key']}",
                          f"SWEET PROGRAM REQUEST digest index={index} record_sha={info['record_sha256']}"]
            else:
                lines.append(f"SWEET PROGRAM REQUEST begin index={index} bytes={len(record)} record_sha={info['record_sha256']} identity={self.spec['context_identity']} key={info['key']} vertex_bytes=4840 fragment_bytes={fs} read_only=1")
            width = 32 if framing == 2 else 128
            for offset in range(0, len(record), width):
                chunk = record[offset:offset+width]
                lines.append(f'SWEET PROGRAM REQUEST data index={index} offset={offset} bytes={len(chunk)} hex={chunk.hex()}')
            lines.append(f"SWEET PROGRAM REQUEST END index={index} bytes={len(record)} record_sha={info['record_sha256']} complete=1 read_only=1")
        lines += [f'SWEET PROGRAM REQUEST EXPORT END result=0 records=2 bytes={total} limit=262144 read_only=1 no_compile=1', 'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(lines)

    def test_full_records(self):
        records, report = extract(self.capture(), self.token, self.spec)
        self.assertEqual(records, [self.record(86136), self.record(45773)])
        self.assertEqual(report['bytes_checked'], 142493)

    def test_truncation_offset_duplicate_and_summary_refused(self):
        text = self.capture()
        chunk = next(line for line in text.splitlines() if ' data ' in line)
        variants = (text.replace(chunk, '', 1), text.replace('offset=32 ', 'offset=33 ', 1),
                    text.replace(chunk, chunk+'\n'+chunk, 1), text.replace('records=2 bytes=', 'records=1 bytes=', 1),
                    text.replace('complete=1 read_only=1', 'complete=0 read_only=1', 1),
                    text.replace('no_compile=1', 'no_compile=0', 1),
                    text.replace('SWEET PROGRAM REQUEST EXPORT END', chunk+'\nSWEET PROGRAM REQUEST EXPORT END', 1))
        for bad in variants:
            with self.subTest(), self.assertRaises(AssertionError):
                extract(bad, self.token, self.spec)

    def test_wrong_identity_token_and_live_boundary_refused(self):
        text = self.capture()
        with self.assertRaises(AssertionError):
            extract(text, self.token, {'context_identity': '02'+'00'*31})
        with self.assertRaises((AssertionError, ValueError)):
            extract(text, '123456789abcdef0', self.spec)
        with self.assertRaises(AssertionError):
            extract(text + '\nSWEET LOOP LIVE proto=1 token='+self.token, self.token, self.spec)

    def test_payload_key_and_header_refused(self):
        record = self.record(86136)
        for offset in (8, 12, 16, 32, 36, 72, 104, 150, len(record)-1):
            bad = bytearray(record)
            bad[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(AssertionError):
                decode_record(bytes(bad), self.spec)

    def test_short_lines_and_legacy_integrity(self):
        text = self.capture()
        self.assertLessEqual(max(map(len, text.splitlines())), 180)
        legacy = self.capture(framing=1)
        extract(legacy, self.token, {'context_identity': self.spec['context_identity']})
        line = next(line for line in legacy.splitlines() if ' data ' in line)
        with self.assertRaises(AssertionError):
            extract(legacy.replace(line, line[:-14], 1), self.token, {'context_identity': self.spec['context_identity']})


if __name__ == '__main__':
    unittest.main(verbosity=2)
