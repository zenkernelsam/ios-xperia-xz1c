"""Faults in stage pairing must not become completed original draws."""
import unittest
from render_stage_evidence import inspect, STAGES

TOKEN = '0123456789abcdef'


def draw(pid=285, call=1, stages=STAGES):
    lines = [f'|AegirPoster| SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1',
             f'|AegirPoster| SWEET SNAPSHOT INIT pid={pid} at_ns=1 expected=8 metadata_only=1',
             f'|AegirPoster| SWEET RENDER REQUEST call={call} width=750 height=1624 primitive=3 first=0 count=6 instances=1 ordered_mask=1 buffers=3 textures=2 samplers=1 metadata_only=1']
    for i, stage in enumerate(stages):
        lines.append(f'|AegirPoster| SWEET RENDER STAGE BEGIN call={call} stage={stage} at_ns={100 + i * 100} metadata_only=1')
        if stage == 'retire':
            lines.append('|AegirPoster| SWEET AIR RENDER draw width=750 height=1624 primitive=3 first=0 count=6 instances=1 target_handle=7 result=0 sequence=9 original_pages=1')
        lines.append(f'|AegirPoster| SWEET RENDER STAGE END call={call} stage={stage} wall_ns=10 cpu_ns=8 cpu_valid=1 result=0 original_order=1')
    return '\n'.join(lines) + '\n'


def scope(body):
    return f'SWEET LOOP LIVE proto=1 token={TOKEN}\n{body}SWEET LOOP ACK FASTBOOT token={TOKEN}\n'


class RenderStageTests(unittest.TestCase):
    def test_process_restart_and_exit_period_are_separate(self):
        value = scope(draw(285) + draw(288)) + draw(291)
        report = inspect(value, TOKEN)
        self.assertEqual((report['observed_requests'], report['complete_draws']), (2, 2))
        self.assertEqual([r['pid'] for r in report['records']], [285, 288])
        self.assertTrue(all(r['all_stages_completed_successfully'] for r in report['records']))
        with self.assertRaises(AssertionError):
            inspect(value, 'f' * 16)

    def test_unfinished_stage_and_no_draw_remain_pending(self):
        body = draw(stages=STAGES[:2]) + '|AegirPoster| SWEET RENDER STAGE BEGIN call=1 stage=targets at_ns=300 metadata_only=1\n'
        report = inspect(scope(body), TOKEN)
        self.assertEqual((report['complete_draws'], report['incomplete_draws']), (0, 1))
        self.assertFalse(report['pending_draws'][0]['stages'][-1]['complete'])
        self.assertEqual(inspect(scope(''), TOKEN)['observed_requests'], 0)

    def test_truncated_duplicate_and_out_of_order_records_refused(self):
        value = scope(draw())
        gate = '|AegirPoster| SWEET RENDER STAGE END call=1 stage=gate wall_ns=10 cpu_ns=8 cpu_valid=1 result=0 original_order=1\n'
        changes = [value.replace(gate, gate + gate),
                   value.replace(gate, gate.replace('result=0', 'result=')),
                   value.replace('stage=make_current at_ns=200', 'stage=make_current at_ns=105'),
                   value.replace('stage=targets at_ns=300', 'stage=writeback at_ns=300'),
                   value.replace('call=1 width=750', 'call=33 width=750', 1),
                   value.replace('cpu_ns=8 cpu_valid=1', 'cpu_ns=8 cpu_valid=0', 1),
                   value.replace('SWEET AIR RENDER draw width=750', 'SWEET AIR RENDER draw width=720'),
                   value.replace('SWEET EXCEPTION observer installed process=AegirPoster', 'SWEET EXCEPTION observer installed process=backboardd')]
        for changed in changes:
            with self.subTest(changed=changed[:150]), self.assertRaises(AssertionError):
                inspect(changed, TOKEN)

    def test_failed_request_is_distinct_from_successful_draw(self):
        body = draw(stages=STAGES[:2])
        body = body.replace('stage=make_current wall_ns=10 cpu_ns=8 cpu_valid=1 result=0',
                            'stage=make_current wall_ns=10 cpu_ns=8 cpu_valid=1 result=-5')
        body += '|AegirPoster| SWEET RENDER STAGE BEGIN call=1 stage=retire at_ns=300 metadata_only=1\n'
        body += '|AegirPoster| SWEET AIR RENDER draw width=750 height=1624 primitive=3 first=0 count=6 instances=1 target_handle=7 result=-5 sequence=0 original_pages=1\n'
        body += '|AegirPoster| SWEET RENDER STAGE END call=1 stage=retire wall_ns=10 cpu_ns=8 cpu_valid=1 result=-5 original_order=1\n'
        record = inspect(scope(body), TOKEN)['records'][0]
        self.assertTrue(record['complete'])
        self.assertFalse(record['all_stages_completed_successfully'])
        self.assertEqual(record['draw']['result'], -5)

    def test_two_requests_waiting_on_the_shared_gate_keep_call_identity(self):
        first = draw().splitlines()
        second = draw(call=2).splitlines()[2:]
        # A second queue can enter the gate while the first draws. Its later
        # stages must remain attached to call 2, even when logs interleave.
        body = first[:4] + second[:2] + first[4:] + second[2:]
        report = inspect(scope('\n'.join(body) + '\n'), TOKEN)
        self.assertEqual(report['complete_draws'], 2)
        self.assertEqual([r['call'] for r in report['records']], [1, 2])
        with self.assertRaises(AssertionError):
            inspect(scope('\n'.join(body).replace('BEGIN call=2 stage=targets', 'BEGIN call=1 stage=targets') + '\n'), TOKEN)


if __name__ == '__main__':
    unittest.main()
