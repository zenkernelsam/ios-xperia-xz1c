"""Decode the production C reader's real filesystem output and inject faults."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/device'))
from poster_config_evidence import inspect_scope
parser=argparse.ArgumentParser();parser.add_argument('--transcript',required=True);parser.add_argument('--output',required=True);args=parser.parse_args()
raw=Path(args.transcript).read_bytes();text=raw.decode('utf-8')
def block(name):
    before='HOST PCFG '+name+' BEGIN\n';after='HOST PCFG '+name+' END\n'
    assert text.count(before)==text.count(after)==1
    return text.split(before,1)[1].split(after,1)[0]
normal=block('normal');decoded=inspect_scope(normal,keep_data=True)
assert decoded['summary']['files']==4 and decoded['summary']['errors']==0
assert any(any(r['value']=='com.apple.NanoUniverse.AegirProxyApp.AegirPoster' for r in f['metadata']['references']) for f in decoded['files'])
for name in ('EINTR','read_fault','race','fifo_race','root_link','oversize'):
    r=inspect_scope(block(name))
    if name=='EINTR':assert r['summary']['files']==4
    else:assert r['summary']['files']==0
lines=normal.splitlines();data_line=next(x for x in lines if x.startswith('SWEET PCFG DATA '));file_line=next(x for x in lines if x.startswith('SWEET PCFG FILE '));root_line=next(x for x in lines if x.startswith('SWEET PCFG ROOT '));end_line=next(x for x in lines if x.startswith('SWEET PCFG END '))
bad_cases={'missing_data':normal.replace(data_line+'\n',''),
    'duplicate_data':normal.replace(data_line,data_line+'\n'+data_line),
    'changed_data':normal.replace(data_line,data_line[:-1]+('0' if data_line[-1]!='0' else '1')),
    'wrong_sha':normal.replace(file_line,file_line.replace('sha256=','sha256=0',1)),
    'duplicate_root':normal.replace(root_line,root_line+'\n'+root_line),
    'wrong_count':normal.replace(end_line,end_line.replace('files=4','files=3')),
    'unknown_record':normal+'SWEET PCFG invented=1\n',
    'missing_END':normal.replace(end_line+'\n',''),
    'early_END':end_line+'\n'+normal.replace(end_line+'\n','')}
rejected={}
for name,value in bad_cases.items():
    try:inspect_scope(value)
    except (ValueError,KeyError) as e:rejected[name]=str(e)
    else:raise AssertionError('Corrupted original metadata accepted: '+name)
result=dict(production_C_transcript_sha256=hashlib.sha256(raw).hexdigest(),actual_files_decoded=len(decoded['files']),
            actual_filesystem_cases_decoded=7,corruptions_rejected=rejected,readonly_metadata_not_selected_wallpaper_proof=True,
            current_phone_configuration_not_captured=True,device_operations=0)
out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.write_bytes((json.dumps(result,indent=2)+'\n').encode())
print(json.dumps(result,indent=2))
