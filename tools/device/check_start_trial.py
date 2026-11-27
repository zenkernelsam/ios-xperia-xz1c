"""Two-phase Windows host test for the hidden launcher; no device operations.

Prepare launches an authored fake supervisor/owned child and returns promptly.
Verify is called later from a separate shell, proving that stdout, the owned
child and the supervisor survive the prepare shell's exit. All fixtures: TEMP.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
FAKE=b'''import argparse,json,os,sys,time
from pathlib import Path
sys.path.insert(0,'D:/Desktop/Git/ios-xperia-xz1c/tools/device')
from procguard import OwnedProcess
p=argparse.ArgumentParser();p.add_argument('--bundle');p.add_argument('--directory');p.add_argument('--private-evidence');p.add_argument('--wait-before-guard',type=int);p.add_argument('--live',action='store_true');a=p.parse_args()
out=Path(a.directory);out.mkdir(exist_ok=False)
def main():
    worker=OwnedProcess([sys.executable,'-c','import time;time.sleep(20)'],cwd=out)
    state=dict(pid=os.getpid(),worker_pid=worker.process.pid,argv=sys.argv,device_operations=0,phase='running')
    (out/'state.json').write_text(json.dumps(state))
    print('HOST_ONLY supervisor started',flush=True)
    time.sleep(20);worker.process.wait(timeout=5);worker.close()
    print('HOST_ONLY owned worker finished',flush=True)
    state.update(phase='finished',worker_exit_code=worker.process.returncode)
    (out/'state.json').write_text(json.dumps(state))
    return 0
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(folder):
    assert not folder.exists()
    folder.mkdir(parents=True)
    fake=folder/'fake repo'
    device=fake/'tools/device';device.mkdir(parents=True)
    bundle=fake/'build/fake';bundle.mkdir(parents=True)
    (bundle/'artifacts.json').write_bytes(b'{"host_only":true}')
    launcher=device/'start_trial.ps1';shutil.copy2(ROOT/'tools/device/start_trial.ps1',launcher)
    shutil.copy2(ROOT/'tools/device/background_trial.py',device/'background_trial.py')
    (device/'run_trial.py').write_bytes(FAKE)
    spec=dict(bundle='build/fake',directory=str(folder/'run'),private_evidence=str(folder/'proof with spaces'),wait_before_guard=30,live=True)
    request=folder/'request.json';request.write_bytes(json.dumps(spec).encode())
    metadata=dict(launcher_sha256=sha(launcher),fake_runner_sha256=sha(device/'run_trial.py'),prepare_pid=os.getpid(),device_operations=0,fixture_is_not_real_device_runner=True)
    (folder/'metadata.json').write_bytes(json.dumps(metadata).encode())
    command=['powershell','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',str(launcher),'-Request',str(request),'-Directory',str(folder/'launch')]
    started=time.monotonic();result=subprocess.run(command,capture_output=True,timeout=30)
    (folder/'prepare.stdout').write_bytes(result.stdout);(folder/'prepare.stderr').write_bytes(result.stderr)
    assert result.returncode==0,result.stderr.decode(errors='replace')
    elapsed=time.monotonic()-started
    assert elapsed<12,'Launching shell waited for the 20-second worker'
    metadata['prepare_shell_seconds']=elapsed
    (folder/'metadata.json').write_bytes(json.dumps(metadata).encode())
    launch=json.loads((folder/'launch/launch.json').read_text(encoding='utf-8-sig'))
    assert launch['retries']==0 and launch['device_operations_by_launcher']==0 and launch['window_style']=='Hidden'
    print(json.dumps(dict(folder=str(folder),pid=launch['pid'],phase='started_host_only_wait_for_finish',device_operations=0)))


def verify(folder):
    metadata=json.loads((folder/'metadata.json').read_bytes())
    launch=json.loads((folder/'launch/launch.json').read_text(encoding='utf-8-sig'))
    state=json.loads((folder/'run/state.json').read_bytes())
    assert state['pid']==launch['pid'] and state['phase']=='finished' and state['worker_exit_code']==0
    assert metadata['prepare_shell_seconds']<12
    supervisor=json.loads((folder/'launch/supervisor.json').read_bytes())
    assert supervisor['phase']=='finished' and supervisor['exit_code']==0
    assert state['device_operations']==0
    stdout=(folder/'launch/stdout.log').read_bytes()
    assert stdout.count(b'HOST_ONLY supervisor started')==stdout.count(b'HOST_ONLY owned worker finished')==1
    assert not (folder/'launch/stderr.log').read_bytes()
    assert state['argv'][state['argv'].index('--private-evidence')+1]==str(folder/'proof with spaces')
    launcher=folder/'fake repo/tools/device/start_trial.ps1'
    assert sha(launcher)==metadata['launcher_sha256']==sha(ROOT/'tools/device/start_trial.ps1')
    negative=[]
    spec=json.loads((folder/'request.json').read_bytes())
    for name,bad in [('offline',dict(spec,live=False)),('unknown_field',dict(spec,extra_command='x'))]:
        request=folder/(name+'.json');request.write_bytes(json.dumps(bad).encode())
        output=folder/('refused-'+name)
        result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',str(launcher),'-Request',str(request),'-Directory',str(output)],capture_output=True,timeout=30)
        (folder/(name+'.stderr')).write_bytes(result.stderr)
        assert result.returncode!=0 and not output.exists()
        negative.append(name)
    report=dict(hidden_supervisor_and_owned_worker_survived_prepare_shell_exit=True,
                stdout_after_prepare_exit_captured=True,quoted_space_paths_passed=True,
                prepare_shell_seconds=metadata['prepare_shell_seconds'],worker_seconds=20,
                rejected_before_launch=negative,launcher_sha256=sha(launcher),
                fake_runner_sha256=metadata['fake_runner_sha256'],fixture_is_not_real_device_runner=True,device_operations=0)
    (folder/'receipt.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('mode',choices=['prepare','verify']);parser.add_argument('--directory',required=True);args=parser.parse_args()
    folder=Path(args.directory).resolve()
    assert os.name=='nt' and folder.is_relative_to(Path(os.environ['TEMP']).resolve())
    (prepare if args.mode=='prepare' else verify)(folder)
