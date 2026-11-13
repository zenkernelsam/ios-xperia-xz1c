"""Resolve observed frames using the following same-process crash's actual slide."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--raw',required=True);parser.add_argument('--evidence',required=True)
parser.add_argument('--cache',required=True);parser.add_argument('--ipsw',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args();raw=Path(args.raw).read_bytes();proof=json.loads(Path(args.evidence).read_bytes())
assert hashlib.sha256(raw).hexdigest()==proof['return_complete']['log_sha256']
record=proof['exceptions']['records'][0];text=raw.decode('ascii','replace')
begin=text.index('SWEET EXCEPTION begin call='+str(record['call']))
end=text.index('SWEET EXCEPTION end call='+str(record['call']),begin)
crash=re.search(r'CRASH AegirPoster\[(\d+)\] sig 6 .*slide (0x[0-9a-f]+) mh (0x[0-9a-f]+)',text[end:])
assert crash and crash.start()<8000
scope=text[begin:end]
assert all(f['pc'] in scope and f['image_base'] in scope for f in record['frames'])
assert 'SWEET EXCEPTION begin' not in text[end:end+crash.start()]
assert 'observer installed' not in text[end:end+crash.start()]
slide=int(crash[2],16);output=Path(args.output);output.mkdir(parents=True,exist_ok=False)
cache=Path(args.cache).resolve();assert cache.is_file()
rows=[]
for frame in record['frames']:
    row=dict(frame)
    if frame['image'] not in ('e','AegirPoster'):
        address=int(frame['pc'],16)-slide
        command=[args.ipsw,'dyld','a2s',str(cache),hex(address),'--cache',str(output/'symbols.a2s'),'-i','--no-color']
        result=subprocess.run(command,capture_output=True,text=True,check=True)
        (output/f"frame-{frame['index']:02d}.txt").write_text(result.stdout+result.stderr)
        assert '/'+frame['image']+' (' in result.stdout
        symbol=re.search(r'^'+re.escape(hex(address))+r': (.+)$',result.stdout,re.M)
        assert symbol
        row.update(unslid_pc=hex(address),unslid_image_base=hex(int(frame['image_base'],16)-slide),symbol=symbol[1])
    rows.append(row)
report=dict(raw_sha256=hashlib.sha256(raw).hexdigest(),token=proof['token'],pid=int(crash[1]),
    observed_same_process_slide=hex(slide),main_header=crash[3],frames=rows,
    cache_header_sha256=hashlib.sha256(cache.read_bytes()).hexdigest(),cache_path=str(cache),
    symbol_tool_sha256=hashlib.sha256(Path(args.ipsw).read_bytes()).hexdigest(),source_only_diagnostic=True)
(output/'symbols.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
