import unittest
from metal_request_evidence import inspect,APIS,ARGUMENTS,EXTRA_APIS,EXTRA_ARGUMENTS

class RequestEvidence(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        text='SWEET LOOP LIVE proto=1 token='+self.token+'\n'
        for api in APIS:text+=f'SWEET METAL REQUEST METHOD pid=278 api={api} present=1 owned=1 installed=1 arguments={ARGUMENTS[api]} exact_ABI=1\n'
        text+='SWEET METAL REQUEST CONFIG pid=278 installed=6 expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1\n'
        text+='SWEET METAL REQUEST BEGIN pid=278 call=1 api=function receiver=SweetMTLLibrary argument_nil=0 argument_class=NSString original_forwarded=1\n'
        text+='SWEET METAL REQUEST NAME pid=278 call=1 bytes=4 hex=74657374 truncated=0 original_name_argument=1\n'
        text+='SWEET METAL REQUEST END pid=278 call=1 api=function outcome=return returned_nil=1 return_class=nil error_pointer=0 error_code=0 original_behavior=1\n'
        return text+'SWEET LOOP ACK FASTBOOT token='+self.token+'\n'
    def test_original_nil_and_absence_preserved(self):
        v=inspect(self.sample(),self.token);self.assertEqual(v['original_nil_returns'][0]['name'],'test');self.assertEqual(v['observed_API_calls']['next_drawable'],0)
    def test_faults_reject(self):
        text=self.sample()
        for bad in (text.replace('bytes=4','bytes=5'),text.replace('api=function outcome=','api=next_drawable outcome='),
            text.replace('return_class=nil','return_class=SweetMTLFunction'),text.replace('call=1','call=65'),
            text.replace('installed=6','installed=5'),text.replace('pid=278 call=1 api=function outcome=','pid=279 call=1 api=function outcome='),
            text.replace('original_behavior=1','original_behavior=')):
            with self.assertRaises(AssertionError):inspect(bad,self.token)
    def test_pending_and_exit_period_not_promoted(self):
        text=self.sample();line='SWEET METAL REQUEST END pid=278 call=1 api=function outcome=return returned_nil=1 return_class=nil error_pointer=0 error_code=0 original_behavior=1\n'
        pending=text.replace(line,'')+line;v=inspect(pending,self.token)
        self.assertEqual((v['complete_samples'],v['incomplete_samples']),(0,1))
    def short_sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token]
        arguments={**ARGUMENTS,**EXTRA_ARGUMENTS}
        for api in APIS+EXTRA_APIS:rows.append(f'SWEET METAL REQUEST METHOD pid=278 api={api} present=1 owned=1 installed=1 arguments={arguments[api]} exact_ABI=1')
        for owner,count in [('quad',6),('renderer',4)]:
            for i in range(count):
                rows += [f'SWEET METAL REQUEST ABI pid=278 owner={owner} index={i} present=1 name_bytes=4 type_bytes=1 truncated=0',
                    f'SWEET METAL REQUEST ABI_DATA pid=278 owner={owner} index={i} part=name offset=0 bytes=4 hex=74657374',
                    f'SWEET METAL REQUEST ABI_DATA pid=278 owner={owner} index={i} part=types offset=0 bytes=1 hex=40',
                    f'SWEET METAL REQUEST ABI_END pid=278 owner={owner} index={i}']
        rows += ['SWEET METAL REQUEST CONFIG pid=278 installed=13 expected=13 limit=64 revision=2 original_behavior=1 no_extra_GPU_work=1',
            'SWEET METAL REQUEST BEGIN pid=278 call=1 api=quad_init_async original_forwarded=1',
            'SWEET METAL REQUEST OBJECT pid=278 call=1 receiver=CLKUIMetalQuadView argument_nil=0 argument_class=NSString',
            'SWEET METAL REQUEST INIT pid=278 call=1 options=9223372036854775809 color_space=-7 async_nil=0',
            'SWEET METAL REQUEST ORIGIN pid=278 call=1 x=1.25 y=2.5',
            'SWEET METAL REQUEST SIZE pid=278 call=1 width=375.5 height=812.25 original_arguments=1',
            'SWEET METAL REQUEST RETURN pid=278 call=1 returned_nil=1 return_class=nil',
            'SWEET METAL REQUEST ERROR pid=278 call=1 pointer=0 code=0',
            'SWEET METAL REQUEST END pid=278 call=1 api=quad_init_async outcome=return original_behavior=1',
            'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)+'\n'
    def test_short_framing_preserves_initialization_nil_and_large_arguments(self):
        result=inspect(self.short_sample(),self.token);row=result['original_nil_returns'][0]
        self.assertEqual(row['options'],9223372036854775809);self.assertEqual(row['origin'],[1.25,2.5]);self.assertEqual(len(result['inventory']),10)
    def test_short_fragment_missing_duplicate_or_wrong_offset_rejected(self):
        text=self.short_sample()
        for bad in (text.replace('type_bytes=1','type_bytes=2',1),text.replace('part=name offset=0','part=name offset=1',1),
            text.replace('color_space=-7','color_space=-9223372036854775809'),text.replace('returned_nil=1 return_class=nil','returned_nil=0 return_class=nil'),
            text.replace('SWEET METAL REQUEST ERROR pid=278 call=1 pointer=0 code=0\n',''),
            text.replace('part=types offset=0 bytes=1 hex=40','part=types offset=0 bytes=2 hex=40')):
            with self.assertRaises(AssertionError):inspect(bad,self.token)

if __name__=='__main__':unittest.main()
