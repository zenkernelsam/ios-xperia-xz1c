"""Missing original sample byte/page/resolve/retirement must fail MSAA admission."""
import unittest
from multisample_evidence import BEGIN,END,SLOTS,samples,pitch,size,operations,draw_masks,filter_mode,expected,verify

class MultisampleEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'a'*64}
    def page(self,phase,slot):
        count=samples(phase)
        base=(1 if phase<7 else 2) if phase<14 else phase+1
        handle=base*10+SLOTS.index(slot)+1
        public=(1 if phase<7 else 2) if phase<14 else (3 if phase<16 else 4)
        return dict(handle=handle,gpu=0x40000000+handle*16384,public=0x100000000+public*0x100000+SLOTS.index(slot)*0x10000,
                    pass_id=phase+1 if phase>=14 and slot<10 else 0)
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,BEGIN,'SWEET MULTISAMPLE INPUT fixture_sha256='+self.fixture['library_sha256']]
        for phase in range(18):
            count=samples(phase);sequence=100*phase+operations(phase)
            for slot in SLOTS:
                info=self.page(phase,slot);format_=80 if slot in (0,10) else 253 if slot==9 else 252
                rows.append(f'SWEET MULTISAMPLE PAGE phase={phase} slot={slot} public={info["public"]:016x} pass={info["pass_id"]} handle={info["handle"]} GPU={hex(info["gpu"])} bytes={size(count,slot)} pitch={pitch(count,slot)} offset=64 format={format_} samples={count if slot<10 else 1}')
            c,z,st,rc,rz=[self.page(phase,slot)['handle'] for slot in SLOTS]
            masks=draw_masks(phase);clear=phase<14 and phase%7==0
            for index,mask in enumerate([None] if clear else masks):
                for slot in (0,9):
                    info=self.page(phase,slot);h=info['handle'];gpu=hex(info['gpu'])
                    if slot==0:rows.append(f'SWEET MSAA import handle={h} width=17 height=9 pitch={pitch(count,slot)} offset=64 format=80 samples={count} GPU={gpu} result=0 original_pages=1')
                    else:rows.append(f'SWEET MSAA stencil_import handle={h} width=17 height=9 pitch={pitch(count,slot)} offset=64 samples={count} GPU={gpu} result=0 separate_z32s8=1 original_pages=1')
                rows.append(f'SWEET MSAA pair depth={z} stencil={st} samples={count} result=0 separate_z32s8=1 original_pages=1')
                rows += [f'SWEET METAL MRT retired index=0 handle={c} format=80 result=0 sequence={100*phase+index+1} original_pages=1',
                         f'SWEET METAL DEPTH retired handle={z} result=0 sequence={100*phase+index+1} original_pages=1',
                         f'SWEET METAL STENCIL retired handle={st} result=0 sequence={100*phase+index+1} original_pages=1']
                rows.append(f'SWEET MSAA '+('clear' if clear else f'draw samples={count} mask={mask}')+(f' samples={count}' if clear else '')+f' color={c} depth={z} stencil={st} result=0 sequence={100*phase+index+1} original_pages=1')
            if phase>=14 or phase%7>=3:
                for slot,target,aspect,mode in ((0,rc,0,0),(8,rz,1,filter_mode(phase))):
                    info=self.page(phase,slot);h=info['handle'];format_=80 if slot==0 else 252
                    rows.append(f'SWEET MSAA import handle={h} width=17 height=9 pitch={pitch(count,slot)} offset=64 format={format_} samples={count} GPU={hex(info["gpu"])} result=0 original_pages=1')
                    rows.append(f'SWEET MSAA resolve src={h} dst={target} samples={count} aspect={aspect} filter={mode} result=0 sequence={sequence-1+aspect} original_pages=1 GPU_only=1')
            checked=sum(size(count,slot) for slot in SLOTS);guard=checked-17*9*(9*count+8);hashes=expected(phase)
            rows += [f'SWEET MULTISAMPLE PHASE index={phase} valid=1 sequence={sequence} operations={operations(phase)} checked_bytes={checked} guard_bytes={guard} mismatches=0 original_pages=1',
                     f'SWEET MULTISAMPLE SOURCE index={phase} color={hashes[0]} depth={hashes[1]} stencil={hashes[2]}',
                     f'SWEET MULTISAMPLE RESOLVED index={phase} color={hashes[3]} depth={hashes[4]}']
        # Both transient encoders materialize their pages before either queue
        # executes. Move the second set's page rows to this actual order.
        for start in (14,16):
            moved=[row for row in rows if row.startswith(f'SWEET MULTISAMPLE PAGE phase={start+1} ')];rows=[row for row in rows if row not in moved]
            at=next(i for i,row in enumerate(rows) if row.startswith(f'SWEET MULTISAMPLE PAGE phase={start} '))+5
            rows[at:at]=moved+[f'SWEET MULTISAMPLE ISOLATION samples={samples(start)} valid=1 two_queues=1 original_public_shared=1 private_pages_differ=1 encoders_before_commit=1']
        rows += [END,'SWEET LOOP ACK FASTBOOT token='+self.token];return '\n'.join(rows)+'\n'
    def test_complete_and_postresolve_source_recipe(self):
        result=verify(self.sample(),self.token,self.fixture);self.assertEqual(result['phases'],18);self.assertEqual(len(result['original_pages']),90)
        self.assertEqual(expected(2)[:3],expected(3)[:3]);self.assertNotEqual(expected(2)[3:],expected(3)[3:])
        self.assertNotEqual(expected(3)[4],expected(4)[4]);self.assertEqual(len({expected(10)[4],expected(11)[4],expected(12)[4]}),3)
        self.assertNotEqual(expected(14)[:3],expected(15)[:3])
    def test_missing_corrupt_or_postACK_data_rejected(self):
        text=self.sample();hashes=expected(3)
        cases=[text.replace('phases=18 checked_bytes','phases=17 checked_bytes'),text.replace('final_allocated=0','final_allocated=16384'),
               text.replace('offset=64 format=80 samples=2','offset=0 format=80 samples=2',1),text.replace('pitch=192 offset=64','pitch=128 offset=64',1),
               text.replace('valid=1 sequence=1','valid=0 sequence=1'),text.replace('mask=2 color=11','mask=1 color=11',1),
               text.replace('aspect=1 filter=2','aspect=1 filter=0'),text.replace('private_pages_differ=1','private_pages_differ=0'),
               text.replace('index=3 color='+hashes[0],'index=3 color='+'0'*16),text.replace('SWEET MULTISAMPLE SOURCE index=3','SWEET MULTISAMPLE UNKNOWN index=3'),
               text.replace('separate_z32s8=1','separate_z32s8=0',1),text.replace('samples=2,4 source','samples=1,4 source'),
               text.replace('SWEET METAL STENCIL retired handle=13 result=0 sequence=1 original_pages=1\n','',1),
               text.replace(END+'\n','')+END+'\n']
        for bad in cases:
            with self.subTest(changed=bad!=text),self.assertRaises(AssertionError):
                self.assertNotEqual(bad,text);verify(bad,self.token,self.fixture)
    def test_original_handle_address_count_and_queues_rejected(self):
        text=self.sample()
        for bad in (text.replace('handle=151','handle=141'),text.replace('pass=16','pass=15'),
                    text.replace('public=0000000100300000','public=0000000100400000',1),text.replace('GPU=0x4002c000','GPU=0x4002c040',1),
                    text.replace('result=0 sequence=302','result=0 sequence=99999',1),text.replace('SWEET MSAA import handle=11','SWEET MSAA UNKNOWN handle=11',1)):
            with self.subTest(changed=bad!=text),self.assertRaises(AssertionError):
                self.assertNotEqual(bad,text);verify(bad,self.token,self.fixture)

if __name__=='__main__':unittest.main()
