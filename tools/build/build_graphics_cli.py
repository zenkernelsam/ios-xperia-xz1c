"""Build the actual host AIR->ESSL CLI with current target helpers, without a device operation."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--air-package',required=True);p.add_argument('--llvm-headers',required=True);p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).resolve();air=Path(a.air_package).resolve();headers=Path(a.llvm_headers).resolve()
assert out.is_relative_to(root/'build') and not out.exists() and air.is_relative_to(root/'build');out.mkdir(parents=True)
manifest=json.loads((air/'manifest.json').read_bytes());sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert manifest['actual_AIR_stage_translation_verified'] and manifest['forced_legacy_interface_host_test_passed']
assert all(sha(root/path)==digest for path,digest in manifest['source_files'].items())
source_files={name:sha(root/name) for name in ('tools/build/air_graphics_cli.cpp','tools/build/build_graphics_cli.py','gpu/metal/air/ESSLTarget.h','gpu/metal/air/HalfESSL.h')}
source_files.update(manifest['source_files'])
library='/System/Library/PrivateFrameworks/GPUCompiler.framework/Versions/32023/Libraries/libLLVM.dylib'
command=['clang++','-std=c++17','-DSWEET_LLVM_PRE23=1','-DSWEET_LLVM16_FORCE_FALLBACK=1','-O2',
    '-I',air/'sources','-I',headers,root/'tools/build/air_graphics_cli.cpp',air/'sources/Frontend.cpp',
    '-Wl,-undefined,dynamic_lookup','-Wl,-needed_library,'+library,'-o',out/'graphics-cli']
result=subprocess.run(list(map(str,command)),capture_output=True);(out/'build.log').write_bytes(result.stdout+result.stderr)
assert result.returncode==0,result.stderr.decode(errors='replace')
assert all(sha(root/path)==digest for path,digest in source_files.items())
report=dict(source_files=source_files,AIR_package_manifest_sha256=sha(air/'manifest.json'),host_graphics_CLI_sha256=sha(out/'graphics-cli'),
    current_ESSL_helpers_compiled=True,forced_legacy_LLVM_interface=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
