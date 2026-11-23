"""Decode real C filesystem fixtures and reject corrupted cache metadata."""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/device'))
from program_cache_inventory_evidence import inspect_scope


def check(transcript):
    text=Path(transcript).read_text(encoding='utf-8')
    identity=bytes((17*i+3)&255 for i in range(32)).hex()
    spec=dict(context_identity=identity,conditional_prime=True,max_files=64,
              max_payload=8388608,max_bytes=33554432,max_prime_entries=8)
    def fixture(kind):
        before='HOST PCINV successful '+kind+' fixture BEGIN\n'
        after='HOST PCINV successful '+kind+' fixture END'
        if text.count(before)!=1 or text.count(after)!=1:
            raise ValueError('Real C fixture markers missing or repeated')
        return text.split(before,1)[1].split(after,1)[0]
    primed,skipped=fixture('conditional'),fixture('skip')
    report=inspect_scope(primed,spec,require_runtime_prime=False)
    skip=inspect_scope(skipped,spec,require_runtime_prime=False)
    if report['snapshots']['B']['end']['missing']!=1 or report['snapshots']['A']['end']['missing'] or skip['action']!={'action':'skip','missing':0}:
        raise ValueError('Actual C conditional branch or snapshot count differs')
    lines=primed.splitlines()
    missing_cfg='\n'.join(s for s in lines if not s.startswith('SWEET PCINV CFG'))+'\n'
    repeated_id=primed.replace(next(s for s in lines if s.startswith('SWEET PCINV ID')),
        '\n'.join([next(s for s in lines if s.startswith('SWEET PCINV ID'))]*2),1)
    changed_identity=primed.replace(identity,'f'+identity[1:],1)
    bad_count=primed.replace('action=prime missing=1','action=prime missing=2',1)
    empty_prime=skipped.replace('action=skip missing=0','action=prime missing=0',1)
    false_complete=primed.replace('compiled=1 failed=0 expected=1','compiled=0 failed=0 expected=1',1)
    after_sha=next(s for s in lines if s.startswith('SWEET PCINV SHA') and 'phase=A' in s and 'index=1' in s)
    changed_sha=primed.replace(after_sha,after_sha[:-1]+('1' if after_sha[-1]!='1' else '2'),1)
    unknown=primed+'SWEET PCINV UNKNOWN invented=1\n'
    phase_order=primed.replace('phase=B revision=1','phase=A revision=1',1)
    no_total='\n'.join(s for s in lines if not(s.startswith('SWEET PCINV TOTAL') and 'phase=A' in s))+'\n'
    completion=next(s for s in lines if s.startswith('SWEET PCINV COMPLETE'))
    early=primed.replace(completion+'\n','').replace('SWEET PCINV BEGIN',completion+'\nSWEET PCINV BEGIN',1)
    corruptions={'missing_cfg':missing_cfg,'repeated_context':repeated_id,'changed_context':changed_identity,
        'wrong_missing_count':bad_count,'empty_primer':empty_prime,'false_completed_count':false_complete,
        'changed_existing_record_SHA':changed_sha,'unknown_record':unknown,'phase_order':phase_order,
        'missing_after_total':no_total,'early_completion':early}
    rejected={}
    for name,case in corruptions.items():
        try:inspect_scope(case,spec,require_runtime_prime=False)
        except ValueError as error:rejected[name]=str(error)
        else:raise AssertionError('Corrupted actual C metadata was accepted: '+name)
    try:inspect_scope(primed,spec,require_runtime_prime=True)
    except ValueError:rejected['host_fake_primer_not_runtime_evidence']=True
    else:raise AssertionError('Host callback became actual Mesa primer evidence')
    return dict(actual_C_filesystem_fixture_decoded=True,conditional=report,skip=skip,
                rejected_faults=rejected,transcript_sha256=hashlib.sha256(Path(transcript).read_bytes()).hexdigest(),
                host_only=True,device_operations=0,GPU_execution_or_UI_not_proven=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--transcript',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();result=check(args.transcript)
    with Path(args.output).open('x',encoding='utf-8',newline='\n') as file:
        json.dump(result,file,indent=2);file.write('\n')
    print(json.dumps({'actual_C_fixture':True,'faults_rejected':len(result['rejected_faults']),
                      'skip_and_prime_verified':True,'device_operations':0}))
