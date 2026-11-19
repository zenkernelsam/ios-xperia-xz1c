import unittest
from metal_request_evidence import inspect,APIS,ARGUMENTS

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

if __name__=='__main__':unittest.main()
