"""Compare actual compiled LLVM C imports against the retained iOS16 cache exports."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--object',required=True);parser.add_argument('--target-symbols',required=True)
parser.add_argument('--output',required=True);args=parser.parse_args()
obj=Path(args.object);symbols=Path(args.target_symbols);raw=symbols.read_bytes()
nm=subprocess.check_output(['nm','-u',str(obj)],text=True)
imports=sorted(set(re.findall(r'\b_(LLVM[A-Za-z0-9_]+)\b',nm)))
exports=set(re.findall(r'\b_(LLVM[A-Za-z0-9_]+)\b',raw.decode('ascii','replace')))
assert imports and exports
report=dict(object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),target_symbols_sha256=hashlib.sha256(raw).hexdigest(),
    imports=imports,missing_target_imports=sorted(set(imports)-exports),target='iOS16.7.16/20H392 libLLVM.dylib',
    exact_compiled_imports_checked=True,device_operations=0)
Path(args.output).write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps({'imports':len(imports),'missing':report['missing_target_imports']}))
