"""Check exact layout refusal and extracted production shaders on host Mesa."""
import argparse,hashlib,json,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
source=(ROOT/'gpu/sweetgl/eagl.c').read_text()
def extract(start,end):
    result=source[source.index(start):source.index(end,source.index(start))]
    return result
# Declarations are before their use in the standalone host translation unit.
functions=extract('static char *\nfbfetch_rewrite(', '\n/*\n * Texture functions')
declarations=extract('static const char fbfetch_decl[] =','\n/* src without the extension')
rewrite=extract('static char *\ntexfn_rewrite(', '\nstatic const char rect_prelude[]')
prelude=extract('static const char nop_func[] =','\n/*\n * The sgl_ texture functions')
extracted=(declarations+'\n'+prelude+'\n'+functions+'\n'+rewrite).encode()
(out/'ProductionCAMatrix.h').write_bytes(extracted)
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
def run(argv,name):
    result=subprocess.run(argv,capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr).decode('utf-8','replace');return result.stdout
run(['wsl.exe','-d',a.distribution,'--','gcc','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all',linux(ROOT/'tools/build/test_ca_matrix_layout.c'),'-o',linux(out/'test-layout')],'layout-compile')
layout_result=run(['wsl.exe','-d',a.distribution,'--',linux(out/'test-layout')],'layout-test')
assert b'CA matrix layout PASS' in layout_result and b'CA current slots PASS' in layout_result
run(['wsl.exe','-d',a.distribution,'--','gcc','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all','-I',linux(out),linux(ROOT/'tools/build/test_ca_matrix_gles.c'),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test-gles')],'gles-compile')
assert b'production rewrite GLES PASS' in run(['wsl.exe','-d',a.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1',linux(out/'test-gles')],'gles-test')
paths=['gpu/sweetgl/eagl.c','gpu/sweetgl/QuartzCoreMatrixLayout.h','gpu/sweetgl/CAMatrixShaders.h','gpu/sweetgl/CAMatrixProbeValues.h','gpu/sweetgl/ca_matrix_probe.c','tools/build/test_ca_matrix_layout.c','tools/build/test_ca_matrix_gles.c','tools/build/check_ca_matrix.py']
report=dict(source_files={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},production_rewrite_sha256=hashlib.sha256(extracted).hexdigest(),
    exact_UUID_instruction_anchors_nonempty_slots_and_wrong_cases_rejected=True,actual_host_GLES_six_matrix_phases_passed=True,
    original_matrix_equations_unchanged=True,independent_reference_tolerance_LSB=1,guard_bytes_checked=192,UBSan_passed=True,
    CA_matrix_observation_revision=2,current_installed_slots_pointers_branches_and_prior_state_checked=True,
    host_fixture_only=True,actual_FD540_install_pixels_and_UI_pending=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
