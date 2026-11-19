"""Reject mixed, reordered and truncated original thread samples."""
import unittest
from symbolicate_render_threads import read_sample

TOKEN='0123456789abcdef'


def fixture():
    return ('SWEET LOOP LIVE proto=1 token='+TOKEN+'\n'
            'VERMEER DEBUG: sample queued (coalesced; selected tasks only)\n'
            'SMP AegirPoster[299] slide 0x45230000 mh 0x100420000\n'
            'SMP AegirPoster t0 pc 0x20784187c lr 0x216dd29ac sp 0x16f9dcd30 bt 0x1005aaed4\n'
            'SMPBT AegirPoster t0 bt 0x1d410d688\n'
            'SMP AegirPoster t1 pc 0x101aa1b50 lr 0x101aa0d7c sp 0x16faf21e0 bt\n'
            'SWEET LOOP ACK FASTBOOT token='+TOKEN+'\n'
            'SMP AegirPoster t2 pc 0x1 lr 0x2 sp 0x3 bt\n')


class ThreadSampleParsing(unittest.TestCase):
    def test_continuation_and_exit_boundary(self):
        value=read_sample(fixture(),TOKEN)
        self.assertEqual(value['pid'],299)
        self.assertEqual(len(value['threads']),2)
        self.assertEqual(value['threads'][0]['backtrace'],[0x1005aaed4,0x1d410d688])
    def test_mixed_duplicate_order_and_truncation_refused(self):
        text=fixture()
        for broken in [text.replace('t1 pc','t2 pc'),text.replace('SMPBT AegirPoster t0','SMPBT AegirPoster t5'),
                       text.replace('lr 0x101aa0d7c sp','lr sp'),
                       text.replace('SMP AegirPoster[299]','SMP AegirPoster[299] slide 0x45230000 mh 0x100420000\nSMP AegirPoster[299]'),
                       text.replace('sample queued','sample missing')]:
            with self.subTest(broken=broken[:100]),self.assertRaises(AssertionError):read_sample(broken,TOKEN)
        with self.assertRaises(AssertionError):read_sample(text,'f'*16)


if __name__=='__main__':unittest.main()
