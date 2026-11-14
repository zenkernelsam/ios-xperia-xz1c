"""Run actual production mip shaders on host Mesa through WSL, never a phone."""
import argparse,ast,hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu');a=p.parse_args()
out=Path(a.output).resolve();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
text=(root/'gpu/sweetgl/metal_render.c').read_text()
template=ast.literal_eval(re.search(r'sprintf\(xr,("(?:[^"\\]|\\.)*"),sweet_xr_shader,target_format\)',text)[1])
(out/'ActualMipFixture.h').write_bytes(('static const char actual_xr_mip_template[]='+json.dumps(template)+';\n').encode())
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
command=['wsl.exe','-d',a.distribution,'--','gcc','-g','-O1','-fsanitize=address,undefined','-I',linux(out),linux(root/'tools/build/test_mip_gles.c'),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test-mip-gles')]
result=subprocess.run(command,capture_output=True);(out/'compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr).decode(errors='replace')
result=subprocess.run(['wsl.exe','-d',a.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(out/'test-mip-gles')],capture_output=True)
(out/'test.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr).decode(errors='replace')
assert b'PASS' in result.stdout
paths=['gpu/sweetgl/metal_render.c','gpu/sweetgl/MipColourShader.h','gpu/sweetgl/XRColourShader.h','gpu/sweetgl/XRColourTables.h','tools/build/test_mip_gles.c','tools/build/check_mip_gles.py']
report=dict(source_files={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},actual_production_XR_shader_template_extracted=True,
    actual_Mesa_linear_sRGB_XR_mipmaps_and_trilinear_passed=True,host_asan_ubsan=True,host_leak_detection_disabled_for_Mesa_runtime=True,FD540_original_importer_not_exercised=True,device_operations=0)
report.update(UNorm_and_XR_exact=True,sRGB_colour_tolerance_LSB=1,padding_and_FD540_fences_require_device_probe=True)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
