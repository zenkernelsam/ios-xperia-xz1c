"""Compile assertions for every projected field against independent target metadata."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from inspect_metal_contract import inspect

root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--metadata',required=True);parser.add_argument('--output',required=True)
args=parser.parse_args();out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists()
out.mkdir(parents=True);metadata=Path(args.metadata).read_bytes();header=root/'gpu/metal/MetalDeviceLimits16.h'
report=inspect(metadata.decode(),header.read_text(),'SweetMTLDeviceLimits16')
assert report['referenced_projection_field_order_and_types_match']
source='#include "MetalDeviceLimits16.h"\n'
for field in report['fields']:
    source+='static_assert(offsetof(SweetMTLDeviceLimits16,'+field['name']+')=='+str(field['offset'])+',"target metadata field offset");\n'
source+='int main(){return 0;}\n';test=out/'target_offsets.cpp';test.write_text(source)
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
run(['clang++','-std=c++17','-I',header.parent,test,'-o',out/'host_offsets'],'host-compile')
run([out/'host_offsets'],'host-run')
run(['xcrun','--sdk','iphoneos','clang++','-arch','arm64','-miphoneos-version-min=16.0','-std=c++17',
     '-I',header.parent,'-c',test,'-o',out/'ios_offsets.o'],'ios-compile')
report.update(header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),metadata_sha256=hashlib.sha256(metadata).hexdigest(),
    all_target_field_offsets_compiled=True,host_and_ios_arm64_compilation_passed=True)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
print(json.dumps({k:v for k,v in report.items() if k!='fields'}))
