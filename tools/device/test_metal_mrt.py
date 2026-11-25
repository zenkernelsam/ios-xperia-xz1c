import unittest
from metal_mrt_evidence import verify,expected,eight_expected,order_expected
class MRTEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'8'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET METAL MRT begin XR_formats=552,553,554,555 dual_targets=2 phases=4 eight_targets=8 original_pages=1',
            'SWEET METAL MRT input library_sha256='+self.fixture['library_sha256'],
            'SWEET METAL MRT caps targets=8 fp32_rt=1 fp32_blend=0 fp32_image=1 independent_enable=1 independent_func=1 float_blend=0 fbfetch=0 no_attachment=1 fragment_images=24 image_units=24'];sequence=0
        for f in range(552,556):
            for p in range(4):
                sequence+=5
                if p<3:rows.append(f'SWEET METAL MRT writeback format={f} handle=1 mask={10 if p==2 else 15} result=0 GPU_only=1 original_code_snapshot=1')
                for target in (0,1):rows.append(f'SWEET METAL MRT retired index={target} handle={target+1} format={f if not target else 80} result=0 sequence={sequence} original_pages=1')
                rows.append(f'SWEET METAL MRT phase format={f} phase={p} sequence={sequence} operations=1 status=4 error=0 checked_bytes=65536 mismatches=0 valid=1')
                for b,h in enumerate(expected(f,p)):rows.append(f'SWEET METAL MRT buffer format={f} phase={p} index={b} hash={h} original_pages=1')
        sequence+=1
        for target in range(8):rows.append(f'SWEET METAL MRT retired index={target} handle={target+1} format=80 result=0 sequence={sequence} original_pages=1')
        rows.append(f'SWEET METAL MRT eight sequence={sequence} operations=1 checked_bytes=131072 mismatches=0 valid=1')
        for b in range(8):rows.append(f'SWEET METAL MRT eight_buffer index={b} hash={eight_expected(b)} original_pages=1')
        for f in range(552,556):
            sequence+=5
            rows+=['SWEET METAL MRT ordered_draw image_mask=1 primitives=2 instances=2 fp32=1 per_primitive_XR_round=1 image_barriers=1',
                f'SWEET METAL MRT writeback format={f} handle=1 mask=14 result=0 GPU_only=1 original_code_snapshot=1',
                f'SWEET METAL MRT retired index=0 handle=1 format={f} result=0 sequence={sequence} original_pages=1',
                f'SWEET METAL MRT order format={f} first=3 count=6 instances=2 primitives=4 sequence={sequence} operations=1 checked_bytes=16384 mismatches=0 valid=1 hash={order_expected(f)} original_pages=1 HDR_negative=1 HDR_above_one=1']
        rows+=['SWEET METAL MRT END result=0 XR_formats=4 phases=16 eight_targets=8 ordered_formats=4 checked_bytes=1245184 final_allocated=0 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token];return '\n'.join(rows)
    def test_complete_hardware_contract(self):self.assertEqual(verify(self.sample(),self.token,self.fixture)['complete_samples'],1)
    def test_clear_optimization_requires_current_clear_and_load_records(self):
        text=self.sample()
        with self.assertRaises(AssertionError):verify(text,self.token,self.fixture,require_xr_clear_elision=True)
        for f in range(552,556):
            key=f'SWEET METAL MRT writeback format={f} handle=1 mask=15'
            clear=f'SWEET METAL MRT clear_target index=0 format={f} handle=1 width=17 height=9 result=0 decode_skipped=1 original_code_snapshot=1 original_clear=1\n'
            text=text.replace(key,clear+key,1)
            for phase in (1,2):
                key=f'SWEET METAL MRT phase format={f} phase={phase} '
                load=f'SWEET METAL MRT decode format={f} handle=1 width=17 height=9 result=0 GPU_only=1 original_target_pages=1\n'
                text=text.replace(key,load+key,1)
        result=verify(text,self.token,self.fixture,require_xr_clear_elision=True)
        self.assertTrue(result['records'][0]['XR_clear_decode_elision_verified'])
        for bad in (text.replace('decode_skipped=1','decode_skipped=0'),text.replace('original_clear=1','original_clear=0'),
                    text.replace('SWEET METAL MRT decode format=553','SWEET METAL MRT decode format=552',1)):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture,require_xr_clear_elision=True)
    def test_raw_code_blend_mask_retirement_fence_guard_rejected(self):
        text=self.sample()
        for bad in (text.replace(expected(553,2)[0],'0'*16),text.replace(expected(552,3)[2],'0'*16),text.replace(eight_expected(7),'0'*16),
            text.replace('mask=10','mask=15'),text.replace('retired index=1','retired index=0'),text.replace('final_allocated=0','final_allocated=16384'),
            text.replace('original_code_snapshot=1','original_code_snapshot=0')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_order_HDR_and_actual_capabilities_required(self):
        text=self.sample()
        for bad in (text.replace(order_expected(555),'0'*16),text.replace('primitives=2 instances=2','primitives=2 instances=1'),
            text.replace('first=3 count=6','first=0 count=6'),text.replace('fp32_blend=0','fp32_blend=1'),text.replace('HDR_negative=1','HDR_negative=0')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_timestamp_barriers_require_every_ordered_update(self):
        text=self.sample()
        for f in range(552,556):
            for phase in (1,2):
                key=f'SWEET METAL MRT writeback format={f} handle=1 mask={15 if phase==1 else 10}'
                record='SWEET XR BARRIER revision=2 primitives=1 instances=1 cached=1 fallback=0 timestamp_wait=1 UCHE_invalidate=1 final_fence_required=1\n'
                # The first mask15 is Clear; insert at the second mask15 for Load.
                if phase==1:
                    first=text.index(key);second=text.index(key,first+1)
                    text=text[:second]+record+text[second:]
                else:text=text.replace(key,record+key,1)
        marker='SWEET METAL MRT ordered_draw image_mask=1 primitives=2 instances=2 fp32=1 per_primitive_XR_round=1 image_barriers=1'
        record='SWEET XR BARRIER revision=2 primitives=2 instances=2 cached=4 fallback=0 timestamp_wait=1 UCHE_invalidate=1 final_fence_required=1'
        text=text.replace(marker,marker+'\n'+record)
        result=verify(text,self.token,self.fixture,require_image_barrier=2)
        self.assertTrue(result['records'][0]['optimized_A5xx_image_barrier_verified'])
        for changed in (text.replace('cached=4','cached=3'),text.replace('fallback=0','fallback=1'),
                        text.replace('timestamp_wait=1','timestamp_wait=0'),text.replace(record,'',1)):
            with self.assertRaises(AssertionError):verify(changed,self.token,self.fixture,require_image_barrier=2)
if __name__=='__main__':unittest.main()
