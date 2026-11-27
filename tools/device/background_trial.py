"""File-backed stdout/stderr for one ordinary run_trial supervisor.

The launcher exits without owning pipes to this process. All device gates,
the sole serial worker and 0x06 return remain in run_trial, unchanged.
"""
import argparse
import json
from pathlib import Path
import sys
import time
import os


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--request',required=True);parser.add_argument('--directory',required=True);args=parser.parse_args()
    folder=Path(args.directory).resolve()
    spec=json.loads(Path(args.request).read_text(encoding='utf-8-sig'))
    allowed={'bundle','directory','private_evidence','owner_evidence','ready_evidence','rpm_evidence','cpu_evidence','gles_evidence','native_evidence','png_evidence','wait_before_guard','live'}
    assert set(spec)<=allowed and spec['live'] is True
    command=['run_trial.py','--bundle',spec['bundle'],'--directory',spec['directory']]
    for name in ('private_evidence','owner_evidence','ready_evidence','rpm_evidence','cpu_evidence','gles_evidence','native_evidence','png_evidence','wait_before_guard'):
        if spec.get(name) is not None:command+=['--'+name.replace('_','-'),str(spec[name])]
    command+=['--live']
    status=dict(pid=os.getpid(),started_at=time.time(),phase='running',runner_arguments=command,launch_attempts=1,retries=0)
    def save():
        (folder/'supervisor.json').write_bytes((json.dumps(status,indent=2)+'\n').encode())
    # The process owns real files, rather than a pipe whose reader lives in
    # the launching PowerShell. Late diagnostic output must survive its exit.
    original_stdout,original_stderr=sys.stdout,sys.stderr
    with (folder/'stdout.log').open('x',encoding='utf-8',newline='\n') as stdout, (folder/'stderr.log').open('x',encoding='utf-8',newline='\n') as stderr:
        sys.stdout,sys.stderr=stdout,stderr
        save();sys.argv=command
        try:
            import run_trial
            code=run_trial.main()
            status.update(phase='finished',exit_code=code,finished_at=time.time())
        except BaseException as error:
            import traceback
            traceback.print_exc();code=1
            status.update(phase='finished',exit_code=code,error=str(error),finished_at=time.time())
        finally:
            save();stdout.flush();stderr.flush()
            sys.stdout,sys.stderr=original_stdout,original_stderr
    return code


if __name__=='__main__':
    raise SystemExit(main())
