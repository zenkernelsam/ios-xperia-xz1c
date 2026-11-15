"""Regenerate authored AIR on the actual Mac frontend and test production XR shaders.

This host GPU/compiler test does not prove FD540, original pages or device return.
"""
import argparse,hashlib,json,re,shlex,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--fixture',required=True)
p.add_argument('--graphics-cli',required=True);p.add_argument('--mac',default='ciscohe@192.168.68.54')
p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
out=Path(a.output).resolve();fixture=Path(a.fixture).resolve();graphics=Path(a.graphics_cli)
assert out.is_relative_to(root/'build') and not out.exists() and fixture.is_relative_to(root/'build')
assert not graphics.is_absolute() and graphics.parts[0]=='build' and '..' not in graphics.parts
out.mkdir(parents=True);manifest=json.loads((fixture/'manifest.json').read_bytes())
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert manifest['fixture']=='mrt' and manifest['authored_semantic_fixture']
assert sha(root/manifest['source'])==manifest['source_sha256'] and sha(fixture/'mrt.air.bc')==manifest['bitcode_sha256']
paths=['tools/build/test_mrt_gles.c','tools/build/air_graphics_cli.cpp','tools/build/check_mrt_gles.py',
    'gpu/sweetgl/MetalShaderAPI.h','gpu/sweetgl/XRBlendShader.h','gpu/sweetgl/MultiTargetShader.h','gpu/sweetgl/RenderUniformRules.h',
    'gpu/sweetgl/XRColourShader.h','gpu/sweetgl/XRColourTables.h','gpu/sweetgl/MipColourShader.h','gpu/diagnostics/MRTProbeValues.h',manifest['source']]
before={name:sha(root/name) for name in paths};relative=out.relative_to(root).as_posix()
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,result.stderr.decode(errors='replace');return result
def mac(command,name):
    return run(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8',a.mac,'cd ~/Desktop/ios-xperia-xz1c && '+shlex.join(list(map(str,command)))],name)
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
mac(['mkdir','-p',relative],'remote-output')
run(['scp',fixture/'mrt.air.bc',a.mac+':Desktop/ios-xperia-xz1c/'+relative+'/mrt.air.bc'],'AIR-transfer')
mac([graphics.as_posix(),relative+'/mrt.air.bc',relative],'actual-AIR-regeneration')
cli=mac(['shasum','-a','256',graphics.as_posix()],'actual-CLI-SHA').stdout.decode().split()[0];assert re.fullmatch('[0-9a-f]{64}',cli)
shaders={}
for function in manifest['functions']:
    name=function['name']+('.vert' if function['MTLB_type']==0 else '.frag')
    run(['scp',a.mac+':Desktop/ios-xperia-xz1c/'+relative+'/'+name,out/name],'transfer-'+name);shaders[name]=sha(out/name)
compiled=out/'test-mrt-gles'
run(['wsl.exe','-d',a.distribution,'--','gcc','-g','-O1','-fsanitize=address,undefined',linux(root/'tools/build/test_mrt_gles.c'),
    '-lEGL','-lGLESv2','-lm','-o',linux(compiled)],'host-compile')
result=run(['wsl.exe','-d',a.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(compiled),linux(out)],'actual-host-GLES')
assert b'ordered overlapping HDR/instances PASS' in result.stdout and result.stderr.count(b'four_overlapping_primitives=1 instance_ID=1 first3=1 negative_and_above_one=1 zero_mismatch=1')==4
assert result.stderr.count(b'actual production uniform scan XR_targets=1 owned_images=2 verified=1')==12 and b'incorrect image binding rejected=1' in result.stderr
assert before=={name:sha(root/name) for name in paths} and sha(fixture/'mrt.air.bc')==manifest['bitcode_sha256']
report=dict(source_files=before,fixture=manifest,actual_Mac_AIR_regeneration=True,graphics_CLI_sha256=cli,ESSL_shaders=shaders,
    host_GLES_log_sha256=sha(out/'actual-host-GLES.log'),actual_host_GPU_MRT_eight_targets_and_four_XR_formats_verified=True,
    actual_host_GPU_independent_blends_masks_original_code_lowbits_verified=True,
    actual_host_GPU_ordered_overlapping_primitives_instance_ID_nonzero_start_negative_above_one_verified=True,
    float32_images_readonly_and_writeonly_separate_bindings=True,per_primitive_XR_rounding=True,host_ASan_UBSan=True,
    production_uniform_rules_applied_to_actual_GLES_uniforms_and_wrong_image_binding_rejected=True,
    actual_FD540_original_pages_and_fences_still_required=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
