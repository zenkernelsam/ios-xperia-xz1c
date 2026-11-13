"""Fault replay for actual Metal command/fence ordering and guard hash."""
import unittest
from metal_queue_evidence import verify,expected_hash,packet_hash

class QueueTests(unittest.TestCase):
    def fixture(self):
        token='0123456789abcdef';rows=['SWEET LOOP LIVE proto=1 token='+token,
            'SWEET METAL QUEUE begin bytes=33024 native_queue=1 enqueue_fifo=1 full_guards=1 gpu_commands=1']
        for sequence,bos in ((21,2),(22,3),(23,3),(24,3)):
            rows.append(f'SWEET SCATTER SUBMIT owner=9 ibs=1 bos={bos} seq={sequence} result=0')
        rows += ['SWEET METAL QUEUE completed first_status=4 second_status=4 first_seq=22 second_seq=24 first_ops=2 second_ops=1 scheduled=2 callbacks=2 checked=33024 mismatches=0 hash='+expected_hash()+' valid=1',
            'SWEET METAL QUEUE END result=0 final_allocated=0 copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1',
            'SWEET LOOP ACK FASTBOOT token='+token]
        return token,'\n'.join(rows)
    def test_real_commands_match_fences_and_bytes(self):
        token,text=self.fixture();proof=verify(text,token);self.assertEqual(proof['records'][0]['native_submissions'],4)
        for changed in (text.replace('first_status=4','first_status=5'),text.replace('callbacks=2','callbacks=1'),
                        text.replace('mismatches=0','mismatches=1'),text.replace('checked=33024','checked=1024'),
                        text.replace(expected_hash(),'0'*16),text.replace('final_allocated=0','final_allocated=49152'),
                        text.replace('seq=23 result=0','seq=23 result=-1'),text.replace('owner=9 ibs=1 bos=3 seq=24','owner=8 ibs=1 bos=3 seq=24'),
                        text.replace('second_seq=24','second_seq=22'),text.replace('seq=23 result=0','seq=25 result=0')):
            with self.assertRaises(AssertionError):verify(changed,token)
    def test_missing_current_boot_submission_not_borrowed(self):
        token,text=self.fixture();missing=text.replace('SWEET SCATTER SUBMIT owner=9 ibs=1 bos=3 seq=23 result=0','lost')
        with self.assertRaises(AssertionError):verify(missing,token)
        with self.assertRaises(ValueError):verify(text,'0'*16)
    def test_global_fences_may_interleave_other_clients(self):
        token,text=self.fixture()
        changed=text.replace('seq=23 result=0','seq=26 result=0').replace('seq=24 result=0','seq=29 result=0').replace('second_seq=24','second_seq=29')
        changed=changed.replace('SWEET SCATTER SUBMIT owner=9 ibs=1 bos=3 seq=26',
            'SWEET SCATTER SUBMIT owner=7 ibs=1 bos=3 seq=23 result=0\nSWEET SCATTER SUBMIT owner=9 ibs=1 bos=3 seq=26')
        proof=verify(changed,token)
        self.assertEqual(proof['records'][0]['submission_sequences'],[21,22,26,29])
    def transport_fixture(self):
        token,text=self.fixture();rows=[]
        for call,(source,so,do,bytes_,fill) in enumerate(((0,0,16,64,60),(1,40,96,64,0),(1,512,512,16396,0)),1):
            sg=0x40040000 if source else 0;dg=0x40080000
            rows.append(f'SWEET METAL BLIT BEGIN call={call} source={source} destination=2 source_gpu={hex(sg) if source else "0"} destination_gpu={hex(dg)} source_offset={so} destination_offset={do} bytes={bytes_} fill={fill}')
            sequences=[20+call] if call<3 else [23,24]
            for chunk,seq in enumerate(sequences):
                offset=chunk*16384;length=min(16384,bytes_-offset);words,hash_=packet_hash(sg+so+offset if source else 0,dg+do+offset,length,fill)
                rows.append(f'SWEET METAL BLIT submit call={call} chunk={chunk} offset={offset} bytes={length} words={words} hash={hash_} sequence={seq} result=0')
                rows.append(f'SWEET METAL BLIT wait call={call} sequence={seq} result=0')
            rows.append(f'SWEET METAL BLIT END call={call} result=0 sequence={sequences[-1]} chunks={len(sequences)} released=0 synchronized=0 native_return=1')
        start=text.index('SWEET SCATTER SUBMIT');end=text.index('SWEET METAL QUEUE completed')
        return token,text[:start]+'\n'.join(rows)+'\n'+text[end:]
    def test_actual_transport_return_and_packet_bytes(self):
        token,text=self.transport_fixture();proof=verify(text,token,1)
        self.assertEqual(proof['records'][0]['submission_sequences'],[21,22,23,24])
        for changed in (text.replace('native_return=1','native_return=0'),text.replace('words=98','words=97'),
                        text.replace('destination_offset=96','destination_offset=100'),text.replace('fill=60','fill=61'),
                        text.replace('sequence=23 result=0','sequence=23 result=-60'),text.replace('synchronized=0','synchronized=-1'),
                        text.replace('offset=16384 bytes=12','offset=16380 bytes=12')):
            with self.assertRaises(AssertionError):verify(changed,token,1)

if __name__=='__main__':unittest.main()
