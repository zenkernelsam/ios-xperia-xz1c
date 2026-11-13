"""Materialize an isolated frontend build without editing retained OSS files."""
import argparse,hashlib,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2]
def materialize(output):
    output=Path(output).resolve();assert output.is_relative_to(root/'build') and not output.exists();output.mkdir(parents=True)
    binding=json.loads((root/'reproducibility/air-reference.json').read_bytes())
    reference=root/'gpu/metal/air/reference'
    for row in binding['sources'].values():
        source=root/row['destination'];assert hashlib.sha256(source.read_bytes()).hexdigest()==row['sha256']
        shutil.copy2(source,output/source.name)
    # A frontend Entry shares reflection with the OSS lowering but imports no
    # private GPU registration, command transport or AOT binary backends.
    local=(root/'gpu/metal/air/Compiler.h').read_bytes().replace(b'"reference/',b'"')
    (output/'Compiler.upstream.h').write_bytes((output/'Compiler.h').read_bytes())
    (output/'Compiler.h').write_bytes(local)
    (output/'Frontend.cpp').write_bytes(b'''#include <llvm-c/Core.h>
#if defined(SWEET_LLVM_PRE23)
#define LLVMUncondBr ((LLVMOpcode)70)
#define LLVMCondBr ((LLVMOpcode)71)
#endif
#include "LLVM16Compat.h"
#include "AIRRuntime.cpp"
''')
    shutil.copy2(root/'gpu/metal/air/LLVM16Compat.h',output/'LLVM16Compat.h')
    report=dict(reference_revision=binding['revision'],sources={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in output.iterdir() if p.is_file()},
        upstream_source_preserved=True,no_private_kernel_JIT_protocol_used=True,device_operations=0)
    (output/'source-binding.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args();print(json.dumps(materialize(a.output)))
