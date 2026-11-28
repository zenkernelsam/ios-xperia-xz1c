import copy
import unittest
from controller import console_status_binding
from diagnostic_candidate import LOG_VERSION
from scanout_trial_evidence import check_status_command


class StatusCommand(unittest.TestCase):
    def sample(self):
        token='0123456789abcdef'
        raw=('SWEET LOOP LIVE proto=1 token='+token+'\nready\n').encode()
        offset=len(raw)
        raw+=('VERMEER DEBUG: K70DBG004 events=1 tx=5 rx=6 log=5 dropped=0\nSWEET LOOP ACK FASTBOOT token='+token+'\n').encode()
        a=dict(token=token,version=LOG_VERSION,console_status_observation=console_status_binding())
        events=[dict(state='OBSERVING',time='2026-11-28T12:00:00+08:00'),dict(state='CONSOLE_STATUS_REQUESTED',time='2026-11-28T12:03:00+08:00',token=token,raw_offset=offset,command_dispatches=1,current_LIVE=True,read_only=True)]
        return a,['swipe','frame','status','fastboot '+token],events,raw

    def test_current_readonly_status_retains_original_controls(self):
        a,c,e,r=self.sample();controls,p=check_status_command(a,c,e,r)
        self.assertEqual(controls,['swipe','frame','fastboot '+a['token']]);self.assertEqual(p['dropped'],0)
        self.assertEqual(p['after_seconds'],180)

    def test_legacy_without_status_is_unchanged(self):
        a,c,e,r=self.sample();c.remove('status');self.assertEqual(check_status_command(a,c,e[:1],r),(c,None))

    def test_wrong_source_count_order_token_time_and_scope_rejected(self):
        original=self.sample()
        for kind in ('binding','version','duplicate','order','token','live','readonly','dispatch','early','before_live','after_ACK','reply','two_replies','event','observing'):
            a,c,e,r=copy.deepcopy(original)
            if kind=='binding':a['console_status_observation']={}
            elif kind=='version':a['version']=LOG_VERSION-1
            elif kind=='duplicate':c.insert(2,'status')
            elif kind=='order':c[1],c[2]=c[2],c[1]
            elif kind=='token':e[1]['token']='0'*16
            elif kind=='live':e[1]['current_LIVE']=False
            elif kind=='readonly':e[1]['read_only']=False
            elif kind=='dispatch':e[1]['command_dispatches']=2
            elif kind=='early':e[1]['time']='2026-11-28T12:02:59+08:00'
            elif kind=='before_live':e[1]['raw_offset']=0
            elif kind=='after_ACK':e[1]['raw_offset']=len(r)
            elif kind=='reply':r=r.replace(b'VERMEER DEBUG:',b'WRONG DEBUG:')
            elif kind=='two_replies':r=r.replace(b'VERMEER DEBUG:',b'VERMEER DEBUG: K70DBG004 events=1 tx=5 rx=6 log=5 dropped=0\nVERMEER DEBUG:')
            elif kind=='event':e.append(copy.deepcopy(e[1]))
            else:e=e[1:]
            with self.subTest(kind=kind),self.assertRaises((AssertionError,ValueError)):check_status_command(a,c,e,r)


if __name__=='__main__':unittest.main()
