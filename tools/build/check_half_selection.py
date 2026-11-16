"""Reproduce all-bit scalar equivalence, folding and host GLES checks."""
import argparse
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--output', required=True)
parser.add_argument('--distribution', default='Ubuntu')
args = parser.parse_args()
out = Path(args.output).resolve()
assert not out.exists() and (out.is_relative_to(root/'build') or out.is_relative_to(Path(os.environ['TEMP']).resolve()))
out.mkdir(parents=True)
reference = root/'gpu/metal/air/half/reference/lilac_half_shader_before_selection.glsl'
helper = root/'gpu/metal/air/HalfESSL.h'
old = reference.read_text()
new = re.search(r'R"GLSL\((.*?)\)GLSL"', helper.read_text(), re.S).group(1)


def scalar(text, suffix):
    text = text[:text.index('vec2 _smtl_half_round')]
    for name in ('pack', 'unpack', 'round', 'select'):
        text = text.replace('_smtl_half_'+name, 'half_'+name+'_'+suffix)
    text = text.replace('uint(', '(uint32_t)(').replace('float(', '(float)(')
    return re.sub(r'\buint\b', 'uint32_t', text)


code = '''#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
static float uintBitsToFloat(uint32_t v){float f;memcpy(&f,&v,4);return f;}
static uint32_t floatBitsToUint(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static uint32_t max(uint32_t a,uint32_t b){return a>b?a:b;}
static uint32_t clamp(uint32_t x,uint32_t a,uint32_t b){return x<a?a:x>b?b:x;}
static int findMSB(uint32_t v){return v?31-__builtin_clz(v):-1;}
'''+scalar(old, 'ref')+scalar(new, 'new')+'''
int main(void){uint64_t errors=0;double start=omp_get_wtime();
#pragma omp parallel for reduction(+:errors) schedule(static)
for(uint64_t i=0;i<(UINT64_C(1)<<32);++i){float f=uintBitsToFloat((uint32_t)i);
 errors+=half_pack_ref(f)!=half_pack_new(f);
 uint32_t rounded=floatBitsToUint(half_round_new(f));
 errors+=floatBitsToUint(half_round_ref(f))!=rounded;
 errors+=floatBitsToUint(half_round_new(uintBitsToFloat(rounded)))!=rounded;}
for(uint32_t i=0;i<65536;++i)errors+=floatBitsToUint(half_unpack_ref(i))!=floatBitsToUint(half_unpack_new(i));
printf("HALF_SELECTION float32_inputs=4294967296 scalar_checks=12884901888 half_unpacks=65536 mismatches=%llu seconds=%.6f\\n",(unsigned long long)errors,omp_get_wtime()-start);return errors?1:0;}
'''
(out/'scalar.c').write_bytes(code.encode())
prefix = ['wsl.exe', '-d', args.distribution, '--exec']


def linux(path):
    path = Path(path).resolve()
    assert path.drive in ('C:', 'D:')
    return '/mnt/'+path.drive[0].lower()+'/'+path.as_posix()[3:]


def run(command, name):
    log = out/(name+'.log')
    with log.open('wb') as stream:
        result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, timeout=75)
    assert result.returncode == 0, name+' failed; preserved '+str(log)
    return log.read_bytes()


run([*prefix, 'gcc', '-O3', '-g', '-fopenmp', '-fsanitize=undefined', linux(out/'scalar.c'), '-lm', '-o', linux(out/'scalar')], 'scalar-compile')
scalar_log = run([*prefix, 'timeout', '--kill-after=3', '60', 'env', 'OMP_NUM_THREADS=8', linux(out/'scalar')], 'scalar-test')
assert b'scalar_checks=12884901888 half_unpacks=65536 mismatches=0' in scalar_log
run([*prefix, 'g++', '-std=c++17', '-O1', '-g', '-fsanitize=undefined', linux(root/'tools/build/test_half_target.cpp'), '-o', linux(out/'target')], 'target-compile')
target_log = run([*prefix, 'timeout', '--kill-after=3', '45', linux(out/'target'), linux(out/'half.glsl')], 'target-test')
assert b'65536 literal bit patterns' in target_log and b'PASS' in target_log
run([*prefix, 'gcc', '-O1', '-g', '-fsanitize=undefined', linux(root/'tools/build/test_half_gles.c'), '-lEGL', '-lGLESv2', '-lm', '-o', linux(out/'gles')], 'gles-compile')
gles_log = run([*prefix, 'timeout', '--kill-after=3', '45', 'env', 'EGL_PLATFORM=surfaceless', 'LIBGL_ALWAYS_SOFTWARE=1', 'MESA_SHADER_CACHE_DISABLE=true', linux(out/'gles'), linux(out/'half.glsl')], 'gles-test')
assert b'131072 values' in gles_log and b'PASS' in gles_log
files = [helper, reference, root/'tools/build/check_half_selection.py', root/'tools/build/test_half_target.cpp', root/'tools/build/test_half_gles.c']
report = dict(source_files={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
              helper_GLSL_sha256=hashlib.sha256(new.encode()).hexdigest(),
              scalar_input_patterns=4294967296, scalar_comparisons=12884901888, half_unpack_patterns=65536,
              exact_literal_patterns=65536, host_GLES_values=131072, scalar_UBSan=True, target_UBSan=True,
              target_ASan_UBSan=False, Mac_target_ASan_UBSan_required=True,
              host_GLES_UBSan=True, host_Mesa_ASan=False,
              Mesa_ASan_failure_preserved_separately=True, phone_GPU_evidence_required=True,
              logs={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.log')}, device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())
print(json.dumps(report))
