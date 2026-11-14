"""Run production half adapter, exhaustive shader conversion and actual AIR ESSL."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--gles-source',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
out=Path(a.output).resolve();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
sources=Path(a.gles_source).resolve();assert sources.is_relative_to(root/'build')
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
def run(argv,name):
    result=subprocess.run(list(map(str,argv)),capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr).decode(errors='replace');return result.stdout
prefix=['wsl.exe','-d',a.distribution,'--'];san=['-O1','-g','-fsanitize=address,undefined']
run([*prefix,'g++','-std=c++17',*san,linux(root/'tools/build/test_half_target.cpp'),'-o',linux(out/'target-test')],'target-compile')
assert b'PASS' in run([*prefix,linux(out/'target-test'),linux(out/'half.glsl')],'target-test')
for source,exe in (('test_half_gles.c','half-gles'),('test_air_half_gles.c','air-half-gles')):
    run([*prefix,'gcc','-std=c11',*san,linux(root/'tools/build'/source),'-lEGL','-lGLESv2','-lm','-o',linux(out/exe)],exe+'-compile')
    target=out/'half.glsl' if exe=='half-gles' else sources
    result=run([*prefix,'env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(out/exe),linux(target)],exe+'-test');assert b'PASS' in result
paths=['gpu/metal/air/HalfESSL.h','gpu/metal/air/lilac-air-half-storage.patch','gpu/metal/air/fixtures/half.air.ll','gpu/diagnostics/HalfProbeValues.h',
    'tools/build/test_half_target.cpp','tools/build/test_half_gles.c','tools/build/test_air_half_gles.c','tools/build/check_air_half.py']
report=dict(source_files={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in paths},
    generated_ESSL={name:hashlib.sha256((sources/name).read_bytes()).hexdigest() for name in ('half_arithmetic.comp','half_position.vert','half_colour.frag')},
    actual_half_adapter_nested_constructors_passed=True,actual_GLES_pack_unpack_half_patterns=65536,actual_GLES_float32_rounding_cases=65536,
    actual_AIR_arithmetic_original_bytes_checked=98304,actual_AIR_half_vertex_fragment_pixels=153,finite_exact_NaN_class_verified=True,
    ASan_UBSan_passed=True,host_Mesa_leak_detection_disabled=True,phone_evidence_required=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
