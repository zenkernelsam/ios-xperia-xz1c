"""Reproduce real production AIR conversion, host GLES pixels and strict bad signatures."""
import argparse,hashlib,json,subprocess,shlex
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--gles-source',required=True);p.add_argument('--air-cli',required=True);p.add_argument('--graphics-cli',required=True);p.add_argument('--original-air',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');p.add_argument('--mac',default='ciscohe@192.168.68.54');a=p.parse_args()
out=Path(a.output).resolve();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
sources=Path(a.gles_source).resolve();assert sources.is_relative_to(root/'build');air=Path(a.air_cli);graphics=Path(a.graphics_cli)
original=Path(a.original_air).resolve();assert original.is_relative_to(root/'build') and original.is_file()
for remote in (air,graphics):assert not remote.is_absolute() and remote.parts[0]=='build' and '..' not in remote.parts
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
def run(argv,name):
    result=subprocess.run(list(map(str,argv)),capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr);return result
def mac_run(argv,name):
    command='cd ~/Desktop/ios-xperia-xz1c && '+shlex.join(list(map(str,argv)))
    return run(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8',a.mac,command],name)
# Recreate generated shaders on the actual Mac compiler, then compare their exact
# bytes with the sources being given to host GLES. Old shaders cannot substitute.
remote_out=out.relative_to(root).as_posix();result=mac_run(['mkdir','-p',remote_out],'remote-output');assert result.returncode==0
fixture=(root/'build/air-conversion-r1/narrow-fixture/narrow.air.bc').resolve();assert fixture.is_file()
for path in (original,fixture):
    result=run(['scp',path,a.mac+':Desktop/ios-xperia-xz1c/'+remote_out+'/'+path.name],'transfer-'+path.name);assert result.returncode==0
for path,selected in ((fixture,'narrow_convert'),(original,'aegir_pass_vsh')):
    result=mac_run([graphics.as_posix(),remote_out+'/'+path.name,remote_out,selected],'generate-'+selected);assert result.returncode==0,result.stderr.decode(errors='replace')
for name in ('aegir_pass_vsh.vert','narrow_convert.comp'):
    result=run(['scp',a.mac+':Desktop/ios-xperia-xz1c/'+remote_out+'/'+name,out/name],'shader-transfer-'+name);assert result.returncode==0
    assert (out/name).read_bytes()==(sources/name).read_bytes(),'Generated shader differs from host input'
compiled=run(['wsl.exe','-d',a.distribution,'--','gcc','-g','-O1','-fsanitize=address,undefined',linux(root/'tools/build/test_air_narrow_gles.c'),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test-air-narrow-gles')],'compile')
assert compiled.returncode==0,compiled.stderr.decode(errors='replace')
result=run(['wsl.exe','-d',a.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(out/'test-air-narrow-gles'),linux(sources)],'actual-GLES')
assert result.returncode==0 and b'actual original Aegir vertex' in result.stdout and b'PASS' in result.stdout,result.stderr.decode(errors='replace')
# Negative AIR inputs retain LLVM-valid signatures but incorrect intrinsic spellings.
text=(root/'gpu/metal/air/fixtures/narrow.air.ll').read_text();failures=[]
for index,(old,new) in enumerate((('air.convert.f.f32.s.i16','air.convert.f.f32.s.i32'),('air.convert.f.v2f32.u.v2i8','air.convert.f.v3f32.u.v2i8'),('air.convert.f.f32.u.i8','air.convert.f.f32.q.i8'))):
    fixture=out/f'bad-{index}.air.ll';fixture.write_bytes(text.replace(old,new).encode())
    result=run(['scp',fixture,a.mac+':Desktop/ios-xperia-xz1c/'+remote_out+'/'+fixture.name],'bad-transfer-'+str(index));assert result.returncode==0
    result=mac_run([air.as_posix(),remote_out+'/'+fixture.name],'reject-'+str(index))
    assert result.returncode!=0 and b'AIR' in result.stderr,'Wrong intrinsic accepted';failures.append(result.stderr.decode(errors='replace').strip())
paths=['gpu/metal/air/lilac-air-conversion.patch','gpu/metal/air/fixtures/narrow.air.ll','gpu/diagnostics/NarrowProbeValues.h',
    'tools/build/test_air_narrow_gles.c','tools/build/check_air_narrow.py','tools/build/materialize_air_sources.py']
report=dict(source_files={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in paths},
    generated_ESSL={name:hashlib.sha256((sources/name).read_bytes()).hexdigest() for name in ('aegir_pass_vsh.vert','narrow_convert.comp')},
    original_Aegir_AIR_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),actual_Mac_shader_regeneration_byte_matched=True,
    actual_generated_compute_signed_unsigned8_16_scalar_vector_passed=True,actual_original_Aegir_vertex_transform_feedback_guarded_passed=True,
    checked_compute_bytes=98304,original_vertex_components=48,vertex_guard_components=8,original_vertex_max_error=1e-6,
    wrong_intrinsic_sign_mangling_shape_rejected=failures,host_ASan_UBSan=True,host_Mesa_leak_check_disabled=True,phone_acceptance_pending=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
