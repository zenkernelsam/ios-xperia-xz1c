"""Read complete authored failure pages; never turn their failed trial into PASS."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from failed_trial_evidence import verify as verify_failure

BEGIN = re.compile(r'SWEET ATTACH SNAP BEGIN suite=(depth32|stencil8|memoryless|msaa) phase=(\d+) slot=(\d+) bytes=(\d+) GPU=([0-9a-f]+) sequence=(\d+) fnv=([0-9a-f]{16}) blocks=(\d+)')
DATA = re.compile(r'SWEET ATTACH SNAP DATA suite=(depth32|stencil8|memoryless|msaa) phase=(\d+) slot=(\d+) block=(\d+) offset=(\d+) n=(\d+) hex=([0-9a-f]+)')
END = re.compile(r'SWEET ATTACH SNAP END suite=(depth32|stencil8|memoryless|msaa) phase=(\d+) slot=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) stable=1 complete=1')

def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def decode(scope):
    lines=[line[line.index('SWEET ATTACH SNAP '):] for line in scope.splitlines() if 'SWEET ATTACH SNAP ' in line]
    pages={};current=None
    for line in lines:
        begin=BEGIN.fullmatch(line)
        data=DATA.fullmatch(line)
        end=END.fullmatch(line)
        assert sum(match is not None for match in (begin,data,end))==1,'Malformed or truncated snapshot record'
        match=begin or data or end;key=(match[1],int(match[2]),int(match[3]))
        if begin:
            assert current is None and key not in pages,'Duplicate or interleaved snapshot'
            size=int(begin[4]);GPU=int(begin[5],16);sequence=int(begin[6]);blocks=int(begin[8])
            assert 0<size<=4096 and 0x40000000<=GPU<0x80000000 and GPU%16384==0 and sequence>0
            assert blocks==(size+23)//24
            pages[key]=dict(bytes=bytearray(),size=size,GPU=GPU,sequence=sequence,fnv=begin[7],blocks=blocks,count=0)
            current=key
        elif data:
            assert current==key and key in pages,'Snapshot data outside its begin/end'
            page=pages[key];chunk=bytes.fromhex(data[7]);n=int(data[6]);at=int(data[5]);block=int(data[4])
            assert at==len(page['bytes']) and block==page['count'] and n==min(24,page['size']-at) and len(chunk)==n and n>0
            page['bytes']+=chunk;page['count']+=1
        else:
            assert current==key and key in pages,'Snapshot end outside its begin/end'
            page=pages[key]
            assert int(end[4])==page['size']==len(page['bytes']) and page['count']==page['blocks']
            assert end[5]==page['fnv']==fnv(page['bytes']),'Snapshot FNV or stability mismatch'
            page['bytes']=bytes(page['bytes']);current=None
    assert current is None and pages,'Missing or incomplete authored failure snapshot'
    return pages

def verify(directory,output):
    root=Path(directory);original=(root/'results.json').read_bytes();failure=verify_failure(root)
    row=json.loads(original)[-1];artifact=row['artifact'];observer=artifact['exception_observer']
    assert observer.get('attachment_failure_snapshot_revision')==1
    raw=(root/'cycle-01/serial.raw.log').read_bytes();text=raw.decode('ascii','replace')
    live='SWEET LOOP LIVE proto=1 token='+artifact['token'];ack='SWEET LOOP ACK FASTBOOT token='+artifact['token']
    assert text.count(live)==text.count(ack)==1;scope=text[text.index(live):text.index(ack)]
    pages=decode(scope);out=Path(output).resolve();assert not out.exists();out.mkdir(parents=True)
    records=[]
    suite_names={'depth32':'DEPTH32','stencil8':'STENCIL8','memoryless':'MEMORYLESS','msaa':'MULTISAMPLE'}
    for (suite,phase,slot),page in pages.items():
        name=suite_names[suite];phase_records=re.findall(r'SWEET '+name+r' PHASE index='+str(phase)+r' valid=0 sequence=(\d+)',scope)
        assert phase_records==[str(page['sequence'])],'Snapshot must bind to one failed retired phase'
        assert any(record['suite']==name for record in failure['attachment_failure_results'])
        if suite=='msaa':
            records_=re.findall(r'SWEET MULTISAMPLE PAGE phase='+str(phase)+r' slot='+str(slot)+r' public=[0-9a-f]{16} pass=\d+ handle=(\d+) GPU=(0x[0-9a-f]+) bytes=(\d+) pitch=(\d+) offset=(\d+) format=(\d+) samples=(\d+)',scope)
            assert len(records_)==1;handle,gpu,size,pitch,offset,format_,samples=records_[0]
            assert int(gpu,16)==page['GPU'] and int(size)==page['size']
        else:
            samples='1';pitch='64' if slot==9 else '128';offset='64';format_=str({0:80,8:252,9:253}[slot])
            start=scope.index('SWEET '+name+' BEGIN');end=scope.index('SWEET '+name+' END',start);block=scope[start:end]
            if suite=='memoryless' and slot in (8,9):
                records_=re.findall(r'SWEET MEMORYLESS PAGE phase='+str(phase)+r' slot='+str(slot)+r' pass=\d+ public=[0-9a-f]{16} handle=(\d+) GPU=(0x[0-9a-f]+) bytes=(\d+) pitch=\d+ offset=64 format=\d+',block)
                assert len(records_)==1;handle,gpu,size=records_[0];assert int(size)==page['size']
            else:
                label='SWEET AIR RENDER texture' if slot==0 else 'SWEET METAL '+('DEPTH' if slot==8 else 'STENCIL')+' import'
                records_=re.findall(re.escape(label)+r' handle=(\d+) width=17 height=9 pitch='+pitch+r' [^\r\n]*?(?:gpu|GPU)=(0x[0-9a-f]+)',block)
                assert records_ and len(set(records_))==1;handle,gpu=records_[0]
            assert int(gpu,16)==page['GPU']
        assert re.search(r'SWEET METAL '+('MRT retired index=0' if slot==0 else 'DEPTH retired' if slot in (8,11) else 'STENCIL retired' if slot==9 else 'MRT retired index=0')+r' handle='+handle+r' [^\r\n]*result=0 sequence=\d+ original_pages=1',scope) or slot>=10
        filename=f'{suite}-phase{phase}-slot{slot}.bin';data=page['bytes'];(out/filename).write_bytes(data)
        records.append(dict(suite=suite,phase=phase,slot=slot,handle=int(handle),GPU=hex(page['GPU']),sequence=page['sequence'],
            byte_count=len(data),pitch=int(pitch),offset=int(offset),format=int(format_),samples=int(samples),fnv=page['fnv'],
            sha256=hashlib.sha256(data).hexdigest(),file=filename))
    assert (root/'results.json').read_bytes()==original
    report=dict(token=artifact['token'],original_result=row['result'],original_reason=row['reason'],source=str(root.resolve()),
        raw_sha256=hashlib.sha256(raw).hexdigest(),pages=records,failure=failure,readonly_retired_author_pages=True,
        complete_native_stage_or_UI_not_accepted=True)
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');return report

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory');parser.add_argument('--output',required=True);args=parser.parse_args()
    report=verify(args.directory,args.output);print(json.dumps({'token':report['token'],'pages':len(report['pages']),'original_result':report['original_result']}))
