import unittest

from host_idle_gate import wait_idle
from policy import Refuse


class HostIdleGate(unittest.TestCase):
    def test_new_fast_or_legacy_conflict_restarts_the_idle_interval(self):
        class Clock:
            value=0
            def now(self): return self.value
            def sleep(self,_): self.value+=1
        clock=Clock();events=[];legacy_calls=[]
        def fast():
            return [{"pid":7,"exe":"fastboot.exe"}] if clock.value<5 else []
        def legacy():
            legacy_calls.append(clock.value)
            return [{"pid":8,"name":"fastboot.exe"}] if len(legacy_calls)==1 else []
        wait_idle(legacy,query_fast=fast,clock=clock.now,pause=clock.sleep,
                  emit=lambda **row:events.append(row),stable_seconds=30,timeout_seconds=100)
        self.assertEqual(legacy_calls,[35,66])
        self.assertEqual(events[-1]["event"],"HOST_IDLE_READY")
        self.assertGreaterEqual(events[-1]["continuous_idle_seconds"],30)
        self.assertEqual(sum(e["event"]=="HOST_IDLE_RACE" for e in events),1)

    def test_timeout_and_preflight_failure_cannot_dispatch(self):
        now=[0]
        def pause(_):now[0]+=1
        with self.assertRaises(Refuse):
            wait_idle(lambda:[],query_fast=lambda:[{"pid":7,"exe":"xperable.exe"}],
                      clock=lambda:now[0],pause=pause,timeout_seconds=30)
        now[0]=0
        def failed():raise Refuse("preflight unreadable")
        with self.assertRaisesRegex(Refuse,"unreadable"):
            wait_idle(failed,query_fast=lambda:[],clock=lambda:now[0],pause=pause,timeout_seconds=60)


if __name__=="__main__":
    unittest.main()
