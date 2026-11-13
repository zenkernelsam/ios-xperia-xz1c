"""Build the explicit native resource factory against its actual signed GLES input."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--ram-package',required=True);parser.add_argument('--output',required=True)
args=parser.parse_args();out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
package=Path(args.ram_package).resolve();manifest=json.loads((package/'manifest.json').read_bytes());gles=package/'sweetgl.dylib'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(gles)==manifest['library_sha256']
files=['gpu/metal/MetalResources.mm','gpu/metal/MetalResources.h','gpu/metal/MetalBufferRules.h',
       'gpu/metal/MetalDeviceLimits16.h','gpu/sweetgl/MetalBackingAPI.h','gpu/sweetgl/MetalBackingRules.h']
sources={p:sha(root/p) for p in files}
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
library=out/'MetalResources.dylib'
run(['xcrun','--sdk','iphoneos','clang++','-arch','arm64','-miphoneos-version-min=16.0','-std=c++17','-fno-objc-arc',
    '-O2','-dynamiclib','-Wl,-undefined,dynamic_lookup','-Wl,-not_for_dyld_shared_cache',
    '-install_name','/dev/sglram/MetalResources.dylib',root/'gpu/metal/MetalResources.mm',gles,
    '-framework','Foundation','-framework','Metal','-o',library],'ios-link')
run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'verify-signature')
assert all(sha(root/p)==digest for p,digest in sources.items()) and sha(gles)==manifest['library_sha256']
report=dict(source_files=sources,library_file='MetalResources.dylib',library_sha256=sha(library),library_bytes=library.stat().st_size,
    gles_library_sha256=sha(gles),only_explicit_resource_factory=True,system_Metal_plugin_not_advertised=True,
    resource_device_acceptance_pending=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
