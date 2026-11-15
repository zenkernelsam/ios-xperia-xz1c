"""Resolve current-library link PCs using its verified glLinkProgram call site."""
import argparse,bisect,hashlib,json,re,subprocess
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--raw',required=True);p.add_argument('--library',required=True)
p.add_argument('--library-sha256',required=True);p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
raw=Path(a.raw).read_bytes();library=Path(a.library).resolve();out=Path(a.output).resolve()
assert hashlib.sha256(library.read_bytes()).hexdigest()==a.library_sha256 and not out.exists()
def linux(path):
    assert re.fullmatch('[A-Za-z]:',path.drive)
    return '/mnt/'+path.drive[0].lower()+path.as_posix()[2:]
def run(tool,*args):
    r=subprocess.run(['wsl.exe','-d',a.distribution,'--',tool,*args],capture_output=True,check=True)
    return r.stdout.decode('utf-8')
symbols=[]
for line in run('llvm-nm-14','--numeric-sort','--demangle',linux(library)).splitlines():
    m=re.match(r'([0-9a-f]{16})\s+([tT])\s+(.+)',line)
    if m:symbols.append((int(m[1],16),m[3]))
disassembly=run('llvm-objdump-14','--disassemble','--disassemble-symbols=_sweetgl_metal_render_create',linux(library))
calls=re.findall(r'^\s*([0-9a-f]+):[^\n]*\bbl\s+[^\n]*<_glLinkProgram>',disassembly,re.M);assert len(calls)==1
return_pc=int(calls[0],16)+4;addresses=[n for n,_ in symbols]
def lookup(pc,base):
    value=pc-base;i=bisect.bisect_right(addresses,value)-1
    if i<0 or value>=0x700000:return None
    start,name=symbols[i]
    return {'relative_pc':hex(value),'symbol':name,'offset':hex(value-start)}
rows=[]
for m in re.finditer(r'CRASH AegirPoster\[(\d+)\] sig 9[^\n]*\nCRASH AegirPoster pc (0x[0-9a-f]+) lr (0x[0-9a-f]+)[^\n]*\nCRASH AegirPoster bt ([^\n]+)',raw.decode('ascii','replace')):
    bt=[int(x,16) for x in m[4].split()];bases=[pc-return_pc for pc in bt if pc>=0x100000000 and (pc-return_pc)%0x4000==0]
    assert len(bases)==1,'Ambiguous native-library return anchor'
    base=bases[0];frames=[lookup(int(m[2],16),base),lookup(int(m[3],16),base)]+[lookup(pc,base) for pc in bt]
    frames=[frame for frame in frames if frame]
    assert any(frame['symbol']=='_sweetgl_metal_render_create' and frame['relative_pc']==hex(return_pc) for frame in frames)
    assert any(frame['symbol']=='_st_link_shader' for frame in frames) and any('gl_nir_link_glsl' in frame['symbol'] for frame in frames)
    rows.append({'pid':int(m[1]),'frames':frames,'current_library_base_inferred_from_unique_16K_aligned_return':True})
assert rows
out.mkdir(parents=True);(out/'link-callsite.txt').write_text(disassembly,encoding='utf-8')
report={'library_sha256':a.library_sha256,'raw_log_sha256':hashlib.sha256(raw).hexdigest(),
        'glLinkProgram_return_offset':hex(return_pc),'crashes':rows,'actual_IR3_execution_not_seen_in_these_stacks':True,
        'original_absolute_process_addresses_not_published':True}
(out/'symbols.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))
