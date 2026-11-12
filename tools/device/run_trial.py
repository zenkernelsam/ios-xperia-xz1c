"""One owned diagnostic stage, gated by real same-kernel prerequisites; no failure replay."""
import argparse,hashlib,json,os,re,subprocess,time
from pathlib import Path
from controller import Artifact,Journal,LiveOps,write_json,execute
from package_trial import ROOT,prove_payload
from policy import LogPolicy,Decision,Refuse
from procguard import LiveLock
from startup_watch import StartupObservation
from trial_evidence import verify
from clock_evidence import verify as verify_clock
from diagnostic_candidate import verify_record

def prerequisites(record, args):
    mode=record['trial_mode'];kernel=record['kernel_sha256'];version=record.get('version');proof={}
    if mode!='private':
        assert args.private_evidence, 'Same-kernel private evidence is required'
        proof['private']=verify(args.private_evidence,'private',kernel,version)
    if mode in ('owner','ready','gpu'):
        assert args.rpm_evidence, 'Actual RPM clock evidence is required'
        proof['rpm']=verify_clock(args.rpm_evidence)
    if mode in ('ready','gpu'):
        assert args.owner_evidence, 'Same-kernel owner evidence is required'
        proof['owner']=verify(args.owner_evidence,'owner',kernel,version)
    if mode=='gpu':
        assert args.ready_evidence and args.cpu_evidence
        proof['ready']=verify(args.ready_evidence,'ready',kernel,version)
        proof['ramcpu']=verify(args.cpu_evidence,'ramcpu',kernel,version)
    return proof

class DiagnosticPolicy(LogPolicy):
    def feed(self,line,now):
        super().feed(line,now)
        if self.live:
            if 'selftest -> WRONG' in line:self.recoverable='GPU_SELFTEST_FAILED'
            if re.search(r'SWEET VM RO END snap=[12] valid=0',line):self.recoverable='VM_METADATA_INVALID'
            for label,pattern in (('GPU_OWNER_FAILED',r'SWEET SG OWN END snap=[12] result=(-?\d+)'),
                                  ('GPU_READY_FAILED',r'SWEET SCATTER READY END result=(-?\d+)')):
                match=re.search(pattern,line)
                if match and int(match[1])!=0:self.recoverable=label
            if 'SWEET GPU: scatter timeout ' in line:self.recoverable='GPU_SCATTER_SUBMIT_TIMEOUT'
    def decision(self,now):
        result=super().decision(now)
        if result.kind=='PASS' and getattr(self,'minimum',0)>now:return Decision('WAIT','DIAGNOSTIC_WINDOW')
        return result

class DiagnosticOps(StartupObservation,LiveOps):
    def observe(self,artifact,step):
        policy=DiagnosticPolicy(artifact.token,step['goals'],step['stable_seconds'])
        policy.minimum=180 if artifact.data['trial_mode']=='gpu' else 0
        start=time.monotonic();next_progress=0
        while time.monotonic()-start<step['observe_seconds']:
            self.check_stop();now=time.monotonic()-start
            for line in self.lines():policy.feed(line,now)
            if policy.actual_watchdog is not None and policy.actual_watchdog!=artifact.watchdog:
                return Decision('STOP','WATCHDOG_CONFIG_MISMATCH'),policy.live
            result=policy.decision(now)
            if result.kind!='WAIT':return result,policy.live
            if not policy.live:
                startup=self.startup_decision(artifact,now)
                if startup:return startup,False
            if self.capture_process.process.poll() is not None:
                return Decision('STOP','SERIAL_WORKER_EXITED'),policy.live
            if now>=next_progress:
                write_json(self.journal.root/'progress.json',{'token':artifact.token,'elapsed_seconds':round(now,2),
                           'live':policy.live,'serial_bytes':(self.directory/'serial.raw.log').stat().st_size if (self.directory/'serial.raw.log').exists() else 0})
                next_progress=now+30
            time.sleep(.1)
        return Decision('STOP','OBSERVATION_INCOMPLETE'),policy.live

def main():
    p=argparse.ArgumentParser();p.add_argument('--bundle',required=True);p.add_argument('--directory',required=True)
    p.add_argument('--private-evidence');p.add_argument('--owner-evidence');p.add_argument('--ready-evidence')
    p.add_argument('--cpu-evidence');p.add_argument('--rpm-evidence');p.add_argument('--live',action='store_true')
    a=p.parse_args();bundle=Path(a.bundle)
    record=json.loads((bundle/'artifacts.json').read_text())['artifacts'][0]
    Artifact(record).validate();mode=record['trial_mode'];kernel=record['kernel_sha256']
    config=verify_record(record)
    payload_proof=prove_payload(record)
    proof=prerequisites(record,a)
    # Audit actual Mac compile inputs before any phone/COM/camera command.
    assert re.fullmatch('[0-9a-f]{64}',kernel)
    remote="import sys,json;sys.path.insert(0,'Desktop/ios-xperia-xz1c/tools');from diagnostic_candidate import audit;print(json.dumps(audit("+str(record['version'])+",kernel_sha='"+kernel+"')))"
    audit=subprocess.run(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ciscohe@192.168.68.54','python3 -c '+__import__('shlex').quote(remote)],capture_output=True,text=True,timeout=25)
    assert audit.returncode==0,audit.stdout+audit.stderr
    actual=json.loads(audit.stdout)
    assert actual['source_binding']==config['binding'] and actual['kernel_sha256']==kernel
    if not a.live:print('OFFLINE: stage gates, artifact and '+str(actual['audited_sources'])+' actual Mac sources/kernel verified');return 0
    directory=Path(a.directory).resolve()
    assert directory.is_relative_to(Path(os.environ['TEMP']).resolve()) and not directory.exists()
    directory.mkdir();write_json(directory/'stage-inputs.json',vars(a))
    write_json(directory/'prerequisites.json',proof)
    write_json(directory/'payload-audit.json',payload_proof)
    (directory/'source-audit.log').write_text(audit.stdout+audit.stderr)
    journal=Journal(directory);lock=LiveLock(directory/'device.lock');ops=DiagnosticOps(journal)
    write_json(directory/'owner.json',{'pid':os.getpid(),'token':record['token'],'mode':mode,'max_cycles':1})
    print('RUN_DIR='+str(directory),flush=True);print(json.dumps({k:record[k] for k in ('token','kernel_sha256','sha256','trial_mode')}),flush=True)
    try:
        rows=execute(json.loads((bundle/'plan.json').read_text()),{record['id']:record},ops,journal)
    finally:ops.close();lock.close()
    if rows and rows[-1]['result']=='PASS':
        proof=verify(directory,mode,kernel,record['version'])
        write_json(directory/'actual-evidence.json',proof)
        print(json.dumps(proof,indent=2),flush=True);return 0
    print(json.dumps(rows,indent=2),flush=True);return 1
if __name__=='__main__':raise SystemExit(main())
