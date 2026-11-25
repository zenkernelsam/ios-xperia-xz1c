import os
from pathlib import Path
import tempfile
import unittest
import subprocess
from unittest.mock import patch
from types import SimpleNamespace

from policy import Refuse
from wait_trial_slot import idle_transition, validate_request, wait_and_dispatch
from procguard import run as run_process
from contextlib import contextmanager


@contextmanager
def retained_fixture(prefix="lilac-wait-slot-fixture-"):
    # Leave host scratch in TEMP for the task's native recycle close-out.
    yield tempfile.mkdtemp(prefix=prefix)


class TrialSlotWait(unittest.TestCase):
    def test_full_idle_window_resets_on_any_intervening_tool(self):
        since, ready = idle_transition(1, [], None, 30)
        self.assertEqual(since, 1)
        self.assertFalse(ready)
        self.assertEqual(idle_transition(30.9, [], since, 30), (1, False))
        self.assertEqual(idle_transition(31, [], since, 30), (1, True))
        since, ready = idle_transition(31, [{"pid": 7, "exe": "fastboot.exe"}], since, 30)
        self.assertIsNone(since)
        self.assertFalse(ready)
        self.assertEqual(idle_transition(40, [], since, 30), (40, False))
        self.assertEqual(idle_transition(69, [], 40, 30), (40, False))
        self.assertEqual(idle_transition(70, [], 40, 30), (40, True))

    def test_request_is_explicit_new_and_limited_to_formal_bundle(self):
        with retained_fixture() as directory:
            base = Path(directory).resolve()
            formal = base / "formal"
            bundle = formal / "build" / "candidate"
            bundle.mkdir(parents=True)
            (bundle / "artifacts.json").write_text("{}")
            temp = base / "temp"
            temp.mkdir()
            request = dict(bundle=str(bundle), directory=str(temp / "trial"),
                           private_evidence=str(temp / "private"), live=True)
            with patch("wait_trial_slot.PATCH", formal), patch.dict(os.environ, TEMP=str(temp)):
                self.assertEqual(validate_request(request, temp / "wait"), (temp / "wait", bundle, temp / "trial"))
                for changed in (dict(request, live=False), dict(request, command="foreign"),
                                dict(request, directory=str(base / "foreign")),
                                dict(request, bundle=str(base / "missing"))):
                    with self.assertRaises(Refuse):
                        validate_request(changed, temp / "wait")
                (temp / "trial").mkdir()
                with self.assertRaises(Refuse):
                    validate_request(request, temp / "wait")
                (temp / "wait").mkdir()
                with self.assertRaises(Refuse):
                    validate_request(request, temp / "wait")

    def test_single_child_dispatch_uses_argv_and_keeps_no_serial_owner(self):
        with retained_fixture() as directory:
            base=Path(directory).resolve();bundle=base/"formal"/"build"/"candidate"
            bundle.mkdir(parents=True);(bundle/"artifacts.json").write_text("{}")
            temp=base/"temp";temp.mkdir()
            request=dict(bundle=str(bundle),directory=str(temp/"trial"),
                         private_evidence=str(temp/"private"),live=True)
            launches=[]
            class Child:
                def __init__(self,argv,**kwargs):
                    launches.append(argv)
                    self.process=SimpleNamespace(pid=123,returncode=0,poll=lambda:0)
                def __enter__(self):return self
                def __exit__(self,*_):pass
            class Lock:
                def __init__(self,*_,**kwargs):pass
                def close(self):pass
            with patch("wait_trial_slot.PATCH",base/"formal"),patch.dict(os.environ,TEMP=str(temp)), \
                 patch("wait_trial_slot.LiveLock",Lock),patch("wait_trial_slot.OwnedProcess",Child), \
                 patch("wait_trial_slot.active_tool_processes",return_value=[]), \
                 patch("wait_trial_slot.run_process",return_value=(0,"[]","")), \
                 patch("wait_trial_slot.time.monotonic",side_effect=[0,0,0,31,32,32,32]), \
                 patch("wait_trial_slot.time.sleep"):
                self.assertEqual(wait_and_dispatch(request,temp/"wait",stable_seconds=30,limit_seconds=300),0)
            self.assertEqual(len(launches),1)
            self.assertIn(str(bundle),launches[0])
            self.assertEqual(launches[0].count("--live"),1)
            self.assertTrue((temp/"wait"/"controller.log").is_file())

    def test_real_owned_child_preserves_space_in_paths_and_arguments(self):
        with retained_fixture(prefix="lilac-wait-slot-space-fixture-") as directory:
            root=Path(directory);script=root/"child script.py"
            script.write_text("import sys\nprint(sys.argv[1])\n")
            code,out,err=run_process([os.sys.executable,str(script),"one literal argument"],cwd=root,timeout=10)
            self.assertEqual(code,0,err)
            self.assertEqual(out.strip(),"one literal argument")


if __name__ == "__main__":
    unittest.main()
