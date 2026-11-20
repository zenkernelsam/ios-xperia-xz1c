"""Reject damaged failure snapshots and keep partial stages rejected."""
import unittest
from attachment_snapshot_evidence import decode,fnv
from failed_trial_evidence import attachment_failures

class FailureSnapshots(unittest.TestCase):
    def fixture(self):
        data=bytes(range(53));digest=fnv(data)
        lines=[f'SWEET ATTACH SNAP BEGIN suite=depth32 phase=0 slot=8 bytes=53 GPU=40004000 sequence=13 fnv={digest} blocks=3']
        for block,at in enumerate((0,24,48)):
            chunk=data[at:at+24]
            lines.append(f'SWEET ATTACH SNAP DATA suite=depth32 phase=0 slot=8 block={block} offset={at} n={len(chunk)} hex={chunk.hex()}')
        lines.append(f'SWEET ATTACH SNAP END suite=depth32 phase=0 slot=8 bytes=53 fnv={digest} stable=1 complete=1')
        return '\n'.join(lines)+'\n',data
    def test_exact_authored_failure_page(self):
        text,data=self.fixture();pages=decode(text)
        self.assertEqual(pages[('depth32',0,8)]['bytes'],data)
        self.assertEqual(pages[('depth32',0,8)]['sequence'],13)
    def test_missing_corrupt_duplicate_reordered_foreign_or_unstable_records(self):
        text,_=self.fixture();lines=text.splitlines();swapped=lines.copy();swapped[1],swapped[2]=swapped[2],swapped[1]
        damaged=(text.replace(lines[2]+'\n',''),text+text,'\n'.join(swapped),
            text.replace('offset=24','offset=23'),text.replace('sequence=13','sequence=0'),text.replace('GPU=40004000','GPU=40004001'),
            text.replace('stable=1','stable=0'),text.replace('bytes=53','bytes=4097'),text.replace('000102','010102'),
            text.replace('block=1','block=0'),text.replace('slot=8 block=1','slot=9 block=1'),text.replace('n=5','n=4'),
            text.replace(lines[-1]+'\n',''),text.replace('suite=depth32','suite=UI'))
        for bad in damaged:
            with self.subTest(value=bad[:120]),self.assertRaises((AssertionError,ValueError)):decode(bad)
    def test_no_missing_unknown_or_false_success_attachment_result(self):
        lines='\n'.join(f'SWEET {name} END result=1 phases=0 checked_bytes=1280 final_allocated=0 GPU_only=1' for name in ('DEPTH32','STENCIL8','MEMORYLESS','MULTISAMPLE','PRESENT PROBE'))
        records=attachment_failures(lines)
        self.assertEqual([r['suite'] for r in records],['DEPTH32','STENCIL8','MEMORYLESS','MULTISAMPLE','PRESENT PROBE'])
        self.assertTrue(all(r['result']==1 and r['completed_phases']==0 for r in records))
        self.assertEqual(attachment_failures(lines.replace('result=1','result=0')),[])
        self.assertEqual(attachment_failures('SWEET DEPTH32 END result=1 phases=0'),[])

if __name__=='__main__':unittest.main()
