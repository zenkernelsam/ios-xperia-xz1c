"""Import pinned OSS source and preserve notices; never modify the reference checkout."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
revision='898998cf1b74ea103e0c16d6dc0016822d8d644b'
paths=['tools/gpu-compiler/'+name for name in ('AIRRuntime.cpp','Semantic.h','AIRControlFlow.h','AIRSpecialization.h','FunctionConstants.h','Compiler.h','LibraryArchive.h','NOTICE.txt')]
paths += ['drivers/include/'+name for name in ('GPUShaderInterface.h','GPUShaderProtocol.h','GPUUserProtocol.h','GPUJITProtocol.h')]

def main():
    p=argparse.ArgumentParser();p.add_argument('--reference',required=True);args=p.parse_args()
    reference=Path(args.reference).resolve();actual=subprocess.check_output(['git','-C',str(reference),'rev-parse','HEAD'],text=True).strip()
    assert actual==revision
    output=root/'gpu/metal/air/reference';output.mkdir(parents=True,exist_ok=False)
    sources={}
    for path in paths:
        raw=subprocess.check_output(['git','-C',str(reference),'show',revision+':'+path])
        target=output/Path(path).name;target.write_bytes(raw)
        sources[path]={'destination':str(target.relative_to(root)).replace('\\','/'),'sha256':hashlib.sha256(raw).hexdigest()}
    report=dict(repository='https://github.com/MC-XiaoXiao/ios_sagit',revision=revision,sources=sources,
        reference_checkout_unmodified=True,licenses='BSD-2-Clause and MIT where indicated; original notices retained',device_operations=0)
    (root/'reproducibility/air-reference.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({'files':len(sources),'revision':revision}))
if __name__=='__main__':main()
