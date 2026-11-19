"""Corrupt route records cannot become evidence of an original draw or resource."""
import unittest
from quad_route_evidence import inspect, APIS, ARGUMENTS, TYPES, EXTRA_APIS, EXTRA_ARGUMENTS, EXTRA_TYPES
from run_trial import DiagnosticPolicy


class QuadRouteEvidence(unittest.TestCase):
    token = '0123456789abcdef'

    def configured(self):
        rows = ['SWEET LOOP LIVE proto=1 token='+self.token]
        for api in APIS:
            types = TYPES[api].encode()
            rows += [f'SWEET QUAD ROUTE METHOD pid=278 api={api} present=1 owned=1 installed=1 arguments={ARGUMENTS[api]} owner_checked=1 meta={int(api in APIS[:2])}',
                     f'SWEET QUAD ROUTE ABI pid=278 api={api} bytes={len(types)} truncated=0']
            for at in range(0, len(types), 24):
                data = types[at:at+24]
                rows.append(f'SWEET QUAD ROUTE ABI_DATA pid=278 api={api} offset={at} bytes={len(data)} hex={data.hex()}')
            rows.append(f'SWEET QUAD ROUTE ABI_END pid=278 api={api}')
        rows.append('SWEET QUAD ROUTE CONFIG pid=278 installed=12 expected=12 per_api=8 max=96 revision=1 original_behavior=1 no_extra_GPU_work=1')
        return '\n'.join(rows)+'\n'

    def call(self, api, number, result_nil=True):
        text = (f'SWEET QUAD ROUTE BEGIN pid=278 call={number} api={api} original_forwarded=1\n'
                f'SWEET QUAD ROUTE OBJECT pid=278 call={number} receiver=CLKUIQuadView argument_nil=1 argument_class=nil\n')
        if api=='paused': text += f'SWEET QUAD ROUTE FLAG pid=278 call={number} value=1\n'
        if api=='sync': text += f'SWEET QUAD ROUTE SYNC pid=278 call={number} time=345.25 check=1 completion_nil=0\nSWEET QUAD ROUTE BOOL pid=278 call={number} value=0\n'
        if api in ('disable', 'enable'):
            reason = b'not in window'
            text += (f'SWEET QUAD ROUTE REASON pid=278 call={number} string=1 bytes={len(reason)} truncated=0 getter_error=0\n'
                     f'SWEET QUAD ROUTE REASON_DATA pid=278 call={number} offset=0 bytes={len(reason)} hex={reason.hex()}\n')
        return text+(f'SWEET QUAD ROUTE RETURN pid=278 call={number} returned_nil={int(result_nil)} return_class={"nil" if result_nil else "SweetMTLCommandQueue"}\n'
                     f'SWEET QUAD ROUTE END pid=278 call={number} api={api} outcome=return original_behavior=1\n')

    def finish(self, text): return text+'SWEET LOOP ACK FASTBOOT token='+self.token+'\n'

    def test_original_nil_reason_and_absence(self):
        report = inspect(self.finish(self.configured()+self.call('manager_queue', 1)+self.call('disable', 2)), self.token)
        self.assertEqual(report['original_nil_resource_returns'][0]['api'], 'manager_queue')
        self.assertEqual(report['records'][1]['reason'], 'not in window')
        self.assertEqual(report['observed_API_calls']['sync'], 0)
        self.assertEqual(report['incomplete_samples'], 0)

    def test_sync_false_and_pending_exit_period(self):
        text = self.configured()+self.call('sync', 1)
        report = inspect(self.finish(text), self.token)
        self.assertFalse(report['records'][0]['boolean_result'])
        end = 'SWEET QUAD ROUTE END pid=278 call=1 api=sync outcome=return original_behavior=1\n'
        report = inspect(self.finish(text.replace(end, ''))+end, self.token)
        self.assertEqual((report['complete_samples'], report['incomplete_samples']), (0, 1))

    def test_bound_and_per_API_limit(self):
        text = self.configured()+''.join(self.call('tick', i) for i in range(1, 9))
        report = inspect(self.finish(text+'SWEET QUAD ROUTE LIMIT pid=278 api=tick per_api=8 original_forwarded=1\n'), self.token)
        self.assertEqual(report['complete_samples'], 8)
        self.assertEqual(report['sampling_limits'], [dict(pid=278, api='tick')])
        for bad in (text+self.call('tick', 9), text.replace('call=2', 'call=3'), self.configured()+'SWEET QUAD ROUTE LIMIT pid=278 api=tick per_api=8 original_forwarded=1\n'):
            with self.assertRaises(AssertionError): inspect(self.finish(bad), self.token)

    def test_corruption_ABI_owner_and_fragments_rejected(self):
        text = self.finish(self.configured()+self.call('disable', 1))
        for bad in (text.replace('owner_checked=1', 'owner_checked=0', 1),
                    text.replace('meta=1', 'meta=0', 1), text.replace('installed=12', 'installed=11', 1),
                    text.replace('api=manager_device offset=0', 'api=manager_device offset=1', 1),
                    text.replace('returned_nil=1 return_class=nil', 'returned_nil=0 return_class=nil'),
                    text.replace('bytes=13 truncated=0 getter_error=0', 'bytes=14 truncated=0 getter_error=0'),
                    text.replace('api=disable outcome=return', 'api=enable outcome=return'),
                    text.replace('SWEET QUAD ROUTE ABI_END pid=278 api=sync\n', ''),
                    text.replace('api=sync bytes=20', 'api=sync bytes=19')):
            with self.assertRaises(AssertionError): inspect(bad, self.token)

    def test_incomplete_install_routes_return(self):
        for expected,counts in ((12,(0,5,10,11)),(26,(0,11,19,20,25))):
            for count in counts:
                policy = DiagnosticPolicy(self.token, [], 0)
                policy.feed('SWEET LOOP LIVE proto=1 token='+self.token, 0)
                policy.feed(f'SWEET QUAD ROUTE CONFIG pid=278 installed={count} expected={expected} per_api=8 max={expected*8} revision={1 if expected==12 else 2} original_behavior=1 no_extra_GPU_work=1', 1)
                self.assertEqual(policy.recoverable, 'QUAD_ROUTE_OBSERVER_INSTALL_INCOMPLETE')

    def configured2(self):
        old = 'SWEET QUAD ROUTE CONFIG pid=278 installed=12 expected=12 per_api=8 max=96 revision=1 original_behavior=1 no_extra_GPU_work=1\n'
        text = self.configured().replace(old,''); rows=[]
        for api in EXTRA_APIS:
            types=EXTRA_TYPES[api].encode(); rows += [f'SWEET QUAD ROUTE METHOD pid=278 api={api} present=1 owned=1 installed=1 arguments={EXTRA_ARGUMENTS[api]} owner_checked=1 meta=0',
                                                   f'SWEET QUAD ROUTE ABI pid=278 api={api} bytes={len(types)} truncated=0']
            for at in range(0,len(types),24):
                data=types[at:at+24];rows.append(f'SWEET QUAD ROUTE ABI_DATA pid=278 api={api} offset={at} bytes={len(data)} hex={data.hex()}')
            rows.append(f'SWEET QUAD ROUTE ABI_END pid=278 api={api}')
        rows.append('SWEET QUAD ROUTE CONFIG pid=278 installed=26 expected=26 per_api=8 max=208 revision=2 original_behavior=1 no_extra_GPU_work=1')
        return text+'\n'.join(rows)+'\n'

    def scalar_sample(self):
        text=self.configured2()
        for call,api in enumerate(('backlight','desired_size','preview','env_init','snapshot_image'),1):
            text += (f'SWEET QUAD ROUTE BEGIN pid=278 call={call} api={api} original_forwarded=1\n'
                     f'SWEET QUAD ROUTE OBJECT pid=278 call={call} receiver=PRPosterEnvironmentImpl argument_nil=1 argument_class=nil\n'
                     f'SWEET QUAD ROUTE ID pid=278 call={call} receiver=0000000100100000 argument=0000000000000000\n')
            if api=='backlight':text+=f'SWEET QUAD ROUTE DOUBLE pid=278 call={call} bits=7ff8000000001234\n'
            if api=='desired_size':text+=f'SWEET QUAD ROUTE SIZE pid=278 call={call} width_bits=4077700000000000 height_bits=8000000000000000\n'
            if api=='preview':text+=f'SWEET QUAD ROUTE BOOL pid=278 call={call} value=0\n'
            if api=='env_init':text+=f'SWEET QUAD ROUTE ENV pid=278 call={call} renderer=0000000100200000 transition=0000000000000000\n'
            text+=(f'SWEET QUAD ROUTE RETURN pid=278 call={call} returned_nil=1 return_class=nil\n'
                   f'SWEET QUAD ROUTE END pid=278 call={call} api={api} outcome=return original_behavior=1\n')
        return self.finish(text)

    def test_original_scalar_bits_and_environment_identity(self):
        result=inspect(self.scalar_sample(),self.token)
        self.assertEqual(result['revision'],2);self.assertEqual(result['complete_samples'],5)
        self.assertEqual(result['records'][0]['double_bits'],'7ff8000000001234')
        self.assertFalse(result['records'][0]['double_finite']);self.assertIsNone(result['records'][0]['double_value'])
        self.assertEqual(result['records'][1]['size_bits'][1],'8000000000000000')
        self.assertEqual(result['records'][3]['renderer_identity'],'0000000100200000')

    def test_scalar_identity_truncation_and_method_revision_rejected(self):
        text=self.scalar_sample()
        for bad in (text.replace('receiver=0000000100100000','receiver=0000000000000000',1),
                    text.replace('bits=7ff8000000001234','bits=7ff800000000123'),
                    text.replace('transition=0000000000000000','transition=0000000000000001'),
                    text.replace('expected=26 per_api=8 max=208 revision=2','expected=12 per_api=8 max=96 revision=1'),
                    text.replace('SWEET QUAD ROUTE DOUBLE pid=278 call=1 bits=7ff8000000001234\n','')):
            with self.assertRaises(AssertionError):inspect(bad,self.token)


if __name__=='__main__': unittest.main()
