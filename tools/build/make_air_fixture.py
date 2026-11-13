"""Assemble authored AIR with LLVM14 and wrap real bitcode in a bounded MTLB v2."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--llvm-as',required=True);parser.add_argument('--output',required=True)
parser.add_argument('--fixture',choices=('basic','render'),default='basic')
args=parser.parse_args();out=Path(args.output).resolve();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/('gpu/metal/air/fixtures/'+args.fixture+'.air.ll');bitcode=out/(args.fixture+'.air.bc')
result=subprocess.run([args.llvm_as,str(source),'-o',str(bitcode)],capture_output=True,text=True)
(out/'assemble.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
air=bitcode.read_bytes();assert air[:4] in (b'BC\xc0\xde',b'\xde\xc0\x17\x0b');air_hash=hashlib.sha256(air).digest()
def tag(name,data):return name.encode()+struct.pack('<H',len(data))+data
names=[('add_seven',2),('position',0),('color',1)] if args.fixture=='basic' else [('position',0),('texture_position',0),('color',1),('textured',1)];groups=[]
for name,type_ in names:
    body=tag('NAME',name.encode()+b'\0')+tag('TYPE',bytes([type_]))+tag('OFFT',struct.pack('<QQQ',0,0,0))+tag('HASH',air_hash)+tag('MDSZ',struct.pack('<Q',len(air)))+b'ENDT'
    groups.append(struct.pack('<I',len(body)+4)+body)
list_=struct.pack('<I',len(groups))+b''.join(groups);pub=b'\0'*4;private=b'\0'*4
positions=[88,88+len(list_),88+len(list_)+4,88+len(list_)+8];sizes=[len(list_)-4,4,4,len(air)]
header=bytearray(88);header[:4]=b'MTLB';struct.pack_into('<H',header,6,2)
struct.pack_into('<Q',header,16,positions[3]+len(air))
for i,(position,size) in enumerate(zip(positions,sizes)):struct.pack_into('<QQ',header,24+i*16,position,size)
library=bytes(header)+list_+pub+private+air;(out/(args.fixture+'.metallib')).write_bytes(library)
variable='sweet_air_fixture' if args.fixture=='basic' else 'sweet_air_render_fixture'
header_name='AIRFixture.h' if args.fixture=='basic' else 'AIRRenderFixture.h'
definition='/* Generated authored AIR fixture; source/blob checks in manifest.json. */\nstatic const unsigned char '+variable+'[] = {\n'
definition+='\n'.join('    '+','.join('0x%02x'%b for b in library[i:i+20])+',' for i in range(0,len(library),20))
definition+='\n};\n';(out/header_name).write_bytes(definition.encode())
report=dict(source=str(source.relative_to(root)),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    bitcode_sha256=air_hash.hex(),bitcode_bytes=len(air),library_sha256=hashlib.sha256(library).hexdigest(),library_bytes=len(library),
    header_sha256=hashlib.sha256(definition.encode()).hexdigest(),functions=[dict(name=name,MTLB_type=type_) for name,type_ in names],
    compiler_path=args.llvm_as,compiler_sha256=hashlib.sha256(Path(args.llvm_as).read_bytes()).hexdigest(),
    fixture=args.fixture,header_file=header_name,library_file=args.fixture+'.metallib',authored_semantic_fixture=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
