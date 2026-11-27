"""A status query must use the owned LIVE worker once; fixtures are retained."""
import json,os,tempfile,unittest
from pathlib import Path
from types import SimpleNamespace
from controller import LiveOps,console_status_binding
from diagnostic_candidate import LOG_VERSION
from policy import Refuse
class ConsoleStatus(unittest.TestCase):
 def setup_ops(self):
  directory=Path(tempfile.mkdtemp(prefix='lilac-console-status-test-',dir=os.environ['TEMP']))
  (directory/'serial.raw.log').write_bytes(b'authored LIVE fixture\n')
  events=[]
  ops=SimpleNamespace(check_stop=lambda:None,directory=directory,
   capture_process=SimpleNamespace(process=SimpleNamespace(poll=lambda:None)),
   journal=SimpleNamespace(event=lambda name,**fields:events.append((name,fields))))
  artifact=SimpleNamespace(token='0123456789abcdef',data={'version':LOG_VERSION,'console_status_observation':console_status_binding()})
  return ops,artifact,events
 def test_current_live_once_and_exact_raw_offset(self):
  ops,artifact,events=self.setup_ops()
  LiveOps.request_console_status(ops,artifact,True)
  rows=[json.loads(s) for s in (ops.directory/'serial-commands.jsonl').read_text().splitlines()]
  self.assertEqual(rows,[{'command':'status'}]);self.assertEqual(len(events),1)
  self.assertEqual(events[0][1]['raw_offset'],len(b'authored LIVE fixture\n'))
  self.assertEqual(events[0][1]['token'],artifact.token)
  with self.assertRaises(Refuse):LiveOps.request_console_status(ops,artifact,True)
  self.assertEqual(len((ops.directory/'serial-commands.jsonl').read_text().splitlines()),1)
 def test_no_live_wrong_binding_or_exited_worker_refused(self):
  for bad in ('not_live','version','binding','worker'):
   ops,artifact,events=self.setup_ops()
   if bad=='version':artifact.data['version']=731
   if bad=='binding':artifact.data['console_status_observation']['max_commands']=2
   if bad=='worker':ops.capture_process.process.poll=lambda:0
   with self.subTest(bad=bad),self.assertRaises(Refuse):LiveOps.request_console_status(ops,artifact,bad!='not_live')
   self.assertFalse((ops.directory/'serial-commands.jsonl').exists());self.assertFalse(events)
if __name__=='__main__':unittest.main()
