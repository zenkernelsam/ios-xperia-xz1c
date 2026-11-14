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
from ram_library import load as load_ram,require_same_library
from owner_metadata import verify as verify_owner_metadata

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
        require_same_library(record,proof['ramcpu'])
        if 'metal_resources' in record:
            assert getattr(args,'gles_evidence',None),'Native resource probe needs actual full GLES stage first'
            proof['gles']=verify(args.gles_evidence,'gpu',kernel,version)
            assert proof['gles']['ram_library_binding']['library_sha256']==record['metal_resources']['gles_library_sha256']
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
            if re.search(r'SWEET METAL RESOURCE END result=[1-9]',line):self.recoverable='METAL_RESOURCE_PROBE_FAILED'
            if re.search(r'SWEET METAL QUEUE END result=[1-9]',line):self.recoverable='METAL_QUEUE_PROBE_FAILED'
            if re.search(r'SWEET AIR COMPUTE END result=[1-9]',line):self.recoverable='AIR_COMPUTE_PROBE_FAILED'
            if re.search(r'SWEET AIR RENDER END result=[1-9]',line):self.recoverable='AIR_RENDER_PROBE_FAILED'
            if re.search(r'SWEET AIR CONSTANTS END result=[1-9]',line):self.recoverable='AIR_CONSTANTS_PROBE_FAILED'
            if re.search(r'SWEET METAL DISCOVERY END result=[1-9]',line):self.recoverable='METAL_DISCOVERY_PROBE_FAILED'
            if 'failed assertion ' in line and 'MTLAddDevice' in line:self.recoverable='METAL_REGISTRY_ASSERTION'
            if 'SWEET METAL missing class=' in line and ' selector=' in line:self.recoverable='METAL_API_UNIMPLEMENTED'
            if re.search(r'SWEET EXCEPTION detail call=\d+ reason="-\[SweetMTL\w+ [^ ]+\]: unrecognized selector',line):self.recoverable='METAL_API_UNIMPLEMENTED'
            if re.search(r'SWEET AIR XR END result=[1-9]',line):self.recoverable='AIR_XR_PROBE_FAILED'
            if re.search(r'SWEET METAL SURFACE END result=[1-9]',line):self.recoverable='METAL_IOSURFACE_PROBE_FAILED'
            if re.search(r'SWEET METAL MIPS END result=[1-9]',line):self.recoverable='METAL_MIPMAP_PROBE_FAILED'
            if re.search(r'SWEET METAL PROBES END process=backboardd result=[1-9]',line):self.recoverable='BACKGROUND_METAL_SUITE_FAILED'
            if re.search(r'SWEET GLES SUITE END result=[1-9]',line):self.recoverable='GPU_SELFTEST_FAILED'
            if 'CRASH AegirPoster reason ns 7 code 0x3: extension check-in timeout' in line:self.recoverable='EXTENSION_CHECKIN_TIMEOUT'
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
            if result.kind=='PASS' and artifact.data['trial_mode']=='owner':
                raw=(self.directory/'serial.raw.log').read_bytes().decode('ascii','replace')
                live_offset=raw.index('SWEET LOOP LIVE proto=1 token='+artifact.token)
                normalized=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',raw[live_offset:])
                try:verify_owner_metadata(normalized)
                except (AssertionError,ValueError,IndexError):
                    return Decision('REVISE','OWNER_EVIDENCE_INCOMPLETE'),policy.live
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
    p.add_argument('--gles-evidence')
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
    if 'ram_library_binding' in record:
        _,binding,ram=load_ram(record['ram_package'],record['ram_library_binding'])
        wanted=ram['frontend_build_binding']
        relative=Path(record['ram_package']).resolve().relative_to(ROOT).as_posix()+'/manifest.json'
        remote="import sys,json,hashlib;from pathlib import Path;r=Path('Desktop/ios-xperia-xz1c');m=json.load(open(r/"+repr(relative)+"));b=m['frontend_build_binding'];s=b['source_files'];d=b['mesa_dependencies'];assert all(hashlib.sha256((r/'gpu/sweetgl'/p).read_bytes()).hexdigest()==h for p,h in s.items());assert all(hashlib.sha256((r/'build/mesa-ios'/p).read_bytes()).hexdigest()==h for p,h in d.items());print(json.dumps(b))"
        if 'exception_observer' in ram:
            observer=ram['exception_observer']
            sources=observer.get('source_files',{observer['source']:observer['source_sha256']})
            if 'metal_resources' in ram:sources={**sources,**ram['metal_resources']['source_files']}
            remote="import hashlib;from pathlib import Path;r=Path('Desktop/ios-xperia-xz1c');s="+repr(sources)+";assert all(hashlib.sha256((r/p).read_bytes()).hexdigest()==h for p,h in s.items());"+remote
        check=subprocess.run(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ciscohe@192.168.68.54','python3 -c '+__import__('shlex').quote(remote)],capture_output=True,text=True,timeout=25)
        assert check.returncode==0,check.stdout+check.stderr
        assert json.loads(check.stdout)==wanted
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
