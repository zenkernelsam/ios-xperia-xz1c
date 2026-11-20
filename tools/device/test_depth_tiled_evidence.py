"""Missing work pages, alias sample bytes, copy fences or guards must refuse."""
import unittest

from depth_tiled_evidence import plane,verify_groups,layout_bytes
from depth32_evidence import verify as verify_depth
import test_depth32_evidence as depth_fixture


def records(group, work_base=101, sequence=1):
    # Fixed tiny A5xx layout values, separate from the production arithmetic.
    layouts={(252,1):(4,256,4096),(252,2):(8,512,8192),(252,4):(16,1024,16384),
             (253,1):(1,128,4096),(253,2):(2,256,4096),(253,4):(4,256,4096)}
    rows=[];copies=[];imports=[];finishes=[];releases=[];work={}
    for index,p in enumerate(group):
        handle=work_base+index;work[p['format']]=handle;cpp,pitch,size=layouts[p['format'],p['samples']]
        rows += [f'SWEET DEPTH TILE PAGE original={p["handle"]} work={handle} format={p["format"]} samples={p["samples"]} pitch={pitch} bytes={size} tile=2 original_pitch={p["pitch"]} original_offset=64',
                 f'SWEET DEPTH TILE MAP original={p["handle"]} work={handle} original_GPU={hex(p["gpu"])} work_GPU={hex(0x50000000+handle*16384)} load={p["load"]} width=17 height=9 cpp={cpp} raw_cpp={cpp} alias_samples=1']
        if p['load']==1:rows.append(f'SWEET DEPTH TILE COPY original={p["handle"]} work={handle} direction=load samples={p["samples"]} width=17 height=9 result=0 sequence={sequence} GPU_only=1 fence=1')
        name='DEPTH' if p['format']==252 else 'STENCIL'
        imports.append(f'SWEET METAL {name} import handle={p["handle"]} width=17 height=9 pitch={p["pitch"]} offset=64 GPU={hex(p["gpu"])} original_pages=1'+(' separate_z32s8=1' if name=='STENCIL' else ''))
        if p['publish']:
            copies.append(f'SWEET DEPTH TILE COPY original={p["handle"]} work={handle} direction=store samples={p["samples"]} width=17 height=9 result=0 sequence={sequence+2+index} GPU_only=1 fence=1')
            finishes.append(f'SWEET METAL {name} retired handle={p["handle"]} result=0 sequence={sequence+2+index} original_pages=1')
        releases.append(f'SWEET DEPTH TILE RETIRE original={p["handle"]} work={handle} result=0 references_retired=1')
    if 253 in work:
        z=next((p['handle'] for p in group if p['format']==252),0);st=next(p for p in group if p['format']==253)
        rows.append(f'SWEET DEPTH TILE PAIR depth={z} stencil={st["handle"]} depth_work={work.get(252,0)} stencil_work={work[253]} samples={st["samples"]} result=0 separate_z32s8=1')
    return '\n'.join(rows+imports+copies+finishes+releases)+'\n'


class TiledEvidence(unittest.TestCase):
    def test_pinned_fd5_small_page_sizes(self):
        expected={(252,1):(4,256,4096),(252,2):(8,512,8192),(252,4):(16,1024,16384),
                  (253,1):(1,128,4096),(253,2):(2,256,4096),(253,4):(4,256,4096)}
        for key,value in expected.items():self.assertEqual(layout_bytes(*key),value)

    def test_original_pair_and_all_sample_aliases(self):
        for samples in (1,2,4):
            group=[plane(1,0x40004000,252,samples,128 if samples==1 else 192 if samples==2 else 320,1),
                   plane(2,0x40008000,253,samples,128 if samples==4 else 64,2)]
            result=verify_groups(records(group),[group]);self.assertEqual(result['planes'],2)
            self.assertTrue(result['GPU_copy_and_fence_verified'])

    def test_missing_wrong_alias_page_or_fence_is_refused(self):
        group=[plane(1,0x40004000,252,4,320,1),plane(2,0x40008000,253,4,128,2)]
        raw=records(group)
        cases=[raw.replace('original_offset=64','original_offset=0',1),raw.replace('tile=2','tile=0',1),
               raw.replace('cpp=16 raw_cpp=16','cpp=16 raw_cpp=4',1),raw.replace('alias_samples=1','alias_samples=4',1),
               raw.replace('work_GPU=0x50194000','work_GPU=0x40004000',1),raw.replace('bytes=16384','bytes=4096',1),
               raw.replace('direction=load','direction=store',1),raw.replace('GPU_only=1 fence=1','GPU_only=1 fence=0',1),
               raw.replace('stencil_work=102','stencil_work=101',1),raw.replace('result=0 references_retired=1','result=-1 references_retired=1',1),
               raw.replace('result=0 sequence=3','result=0 sequence=1',1)]
        for name in ('MAP','COPY','RETIRE','PAIR'):
            line=next(line for line in raw.splitlines() if line.startswith('SWEET DEPTH TILE '+name+' '))
            cases.append(raw.replace(line+'\n','',1))
        for bad in cases:
            with self.subTest(changed=bad!=raw),self.assertRaises(AssertionError):
                self.assertNotEqual(bad,raw);verify_groups(bad,[group])

    def test_only_changed_plane_publishes_and_stencil_only_pairs(self):
        group=[plane(1,0x40004000,252,1,128,1,False),plane(2,0x40008000,253,1,64,2)]
        self.assertEqual(verify_groups(records(group),[group])['planes'],2)
        only=[plane(2,0x40008000,253,1,64,2)]
        self.assertEqual(verify_groups(records(only),[only])['planes'],1)
        with self.assertRaises(AssertionError):verify_groups(records(only).replace('depth=0','depth=1'),[only])

    def test_byte_recipe_still_required_for_tiled_depth(self):
        old=depth_fixture.DepthEvidence().sample();lines=old.splitlines();out=[];phase=0;group=None
        for line in lines:
            if line.startswith('SWEET METAL DEPTH import '):
                group=[plane(1,0x40004000,252,1,128,2 if phase<25 or phase==28 else 1)]
                # Include the real import/retirement records from this backend
                # operation, preserving the author's independent raw hash rows.
                out.extend(records(group,101+phase,phase*10+1).splitlines())
            elif line.startswith('SWEET METAL DEPTH retired '):continue
            elif line.startswith('SWEET DEPTH32 PHASE '):
                out.append(line.replace('sequence='+str(phase+1),'sequence='+str(phase*10+3)));phase+=1
            else:out.append(line)
        raw='\n'.join(out)+'\n';tester=depth_fixture.DepthEvidence()
        self.assertEqual(verify_depth(raw,tester.token,tester.fixture,2)['tiled_work_planes']['planes'],29)
        with self.assertRaises(AssertionError):verify_depth(old,tester.token,tester.fixture,2)
        with self.assertRaises(AssertionError):verify_depth(raw.replace('depth='+__import__('depth32_evidence').expected(1)[1],'depth='+'0'*16,1),tester.token,tester.fixture,2)

    def test_tiled_msaa_keeps_complete_original_sample_recipe(self):
        import multisample_evidence as msaa
        import test_multisample_evidence as source
        fixture=source.MultisampleEvidence();lines=fixture.sample().splitlines();out=[];phase=0;operation=0
        for line in lines:
            if line.startswith('SWEET MULTISAMPLE PAGE '):
                # Concurrent pages are authored before either operation; select
                # the phase by its actual submission/import rather than PAGE.
                out.append(line);continue
            if line.startswith('SWEET MSAA import '):
                import re
                h=int(re.search(r'handle=(\d+)',line)[1]);fmt=int(re.search(r'format=(\d+)',line)[1])
                if fmt==80 and h==fixture.page(phase,0)['handle'] and operation < (1 if phase<14 and phase%7==0 else len(msaa.draw_masks(phase))):
                    count=msaa.samples(phase)
                    group=[plane(fixture.page(phase,slot)['handle'],fixture.page(phase,slot)['gpu'],252 if slot==8 else 253,
                                 count,msaa.pitch(count,slot),2 if phase%7==0 and phase<14 or phase>=14 and operation==0 else 1)
                           for slot in (8,9)]
                    out.extend(records(group,1000+phase*50+operation*2,100*phase+10+operation*5).splitlines())
                    out.append(line)
                else:
                    out.append(line)
                    if fmt==252:
                        info=fixture.page(phase,11);group=[plane(info['handle'],info['gpu'],252,1,128,0)]
                        out.extend(records(group,4000+phase,100*phase+95).splitlines())
            elif line.startswith(('SWEET MSAA stencil_import ','SWEET MSAA pair ','SWEET METAL DEPTH retired ','SWEET METAL STENCIL retired ')):continue
            elif line.startswith(('SWEET MSAA draw ','SWEET MSAA clear ')):
                import re
                # Copy-store fences are separate backend submissions; completed
                # operation metadata still carries the max original-page fence.
                seq=100*phase+13+operation*5
                out.append(re.sub(r'sequence=\d+',f'sequence={seq}',line));operation+=1
            elif line.startswith('SWEET MSAA resolve '):
                import re
                aspect=int(re.search(r'aspect=(\d+)',line)[1]);seq=100*phase+(97 if aspect else 96)
                out.append(re.sub(r'sequence=\d+',f'sequence={seq}',line))
            elif line.startswith('SWEET MULTISAMPLE PHASE '):
                import re
                seq=100*phase+97 if phase>=14 or phase%7>=3 else 100*phase+13+(operation-1)*5
                out.append(re.sub(r'sequence=\d+',f'sequence={seq}',line));phase+=1;operation=0
            elif line.startswith('SWEET METAL MRT retired '):
                import re
                out.append(re.sub(r'sequence=\d+',f'sequence={100*phase+11+operation*5}',line))
            else:out.append(line)
        raw='\n'.join(out)+'\n'
        result=msaa.verify(raw,fixture.token,fixture.fixture,2)
        self.assertEqual(len(result['tiled_work_planes']),18)
        with self.assertRaises(AssertionError):msaa.verify(fixture.sample(),fixture.token,fixture.fixture,2)
        with self.assertRaises(AssertionError):msaa.verify(raw.replace('alias_samples=1','alias_samples=4',1),fixture.token,fixture.fixture,2)
        with self.assertRaises(AssertionError):msaa.verify(raw.replace('mismatches=0','mismatches=1',1),fixture.token,fixture.fixture,2)


if __name__=='__main__':unittest.main(verbosity=2)
