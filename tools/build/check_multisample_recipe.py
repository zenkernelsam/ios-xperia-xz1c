"""Compare actual compiled C byte recipe with independent Python verification."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
from check_depth_stencil_gles import ROOT,linux,bounded_linux_test
sys.path.insert(0,str(ROOT/'tools/device'))
from multisample_evidence import expected,samples,size,SLOTS

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--distribution',default='Ubuntu');args=parser.parse_args()
    out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['gpu/diagnostics/MultisampleProbeValues.h','tools/build/test_multisample_probe_values.c','tools/device/multisample_evidence.py','tools/build/check_multisample_recipe.py']
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    command=['wsl.exe','-d',args.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/paths[1]),'-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').open('xb').write(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-4000:].decode(errors='replace')
    data=bounded_linux_test(args.distribution,out,'test',['UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    rows=re.findall(rb'phase=(\d+) slot=(\d+) samples=(\d+) bytes=(\d+) hash=([0-9a-f]{16})',data);assert len(rows)==90
    for index,row in enumerate(rows):
        phase=index//5;slot=SLOTS[index%5];count=samples(phase)
        assert tuple(int(v) for v in row[:4])==(phase,slot,count,size(count,slot)) and row[4].decode()==expected(phase)[index%5]
    assert data.count(b'Author MSAA recipe END checked_bytes=151488 guard_bytes=55098 host_only=1')==1
    assert sources=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    report=dict(source_files=sources,actual_C_byte_recipe_matches_independent_Python_hashes=True,phases=18,pages=90,checked_bytes=151488,guard_bytes=55098,UBSan_passed=True,host_recipe_only_not_device_GPU_evidence=True,device_operations=0)
    (out/'manifest.json').open('xb').write((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
