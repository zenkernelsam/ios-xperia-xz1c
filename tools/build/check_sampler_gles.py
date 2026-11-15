"""Reproduce actual host GLES sampler pixels; target FD540 proof stays separate."""
import argparse,hashlib,json,os,re,subprocess,tempfile,uuid
from pathlib import Path

root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output');p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
out=Path(a.output).resolve() if a.output else Path(tempfile.gettempdir())/('lilac-sampler-host-'+uuid.uuid4().hex[:8])
assert not out.exists() and (out.is_relative_to(root/'build') or out.resolve().is_relative_to(Path(os.environ['TEMP']).resolve()))
out.mkdir(parents=True)
def linux(path):
    path=path.resolve();assert re.fullmatch('[A-Za-z]:',path.drive)
    return '/mnt/'+path.drive[0].lower()+path.as_posix()[2:]
def run(argv,name):
    result=subprocess.run(list(map(str,argv)),capture_output=True,timeout=60)
    (out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr).decode('utf-8','replace')[-2000:]
    return result
source=root/'tools/build/test_sampler_gles.c';header=root/'gpu/sweetgl/RenderSamplerRules.h';api=root/'gpu/sweetgl/MetalShaderAPI.h'
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
before={str(path.relative_to(root)):sha(path) for path in (source,header,api)}
base=['wsl.exe','-d',a.distribution,'--'];binary=out/'test-sampler-gles'
run([*base,'gcc','-std=c11','-O1','-g','-fsanitize=address,undefined',linux(source),'-lEGL','-lGLESv2','-lm','-o',linux(binary)],'compile')
result=run([*base,'env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(binary)],'actual-GLES')
text=result.stdout.decode('utf-8');match=re.search(r'sampler host PASS cases=(\d+) exact_pixels=(\d+) border_cap=([01]) anisotropic_cap=([01])',text);assert match
assert before=={str(path.relative_to(root)):sha(path) for path in (source,header,api)}
report={'source_files':before,'checks':int(match[1]),'exact_pixels':int(match[2]),'border_cap':int(match[3]),'anisotropic_cap':int(match[4]),
        'actual_GLES_uniform_introspection_and_sampling':True,'ASan_UBSan':True,'original_host_log_sha256':sha(out/'actual-GLES.log'),
        'target_FD540_original_page_fence_acceptance_required':True,'device_operations':0}
(out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps({'output':str(out),**report}))
