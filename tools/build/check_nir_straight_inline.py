"""Compare isolated NIR prototype with pinned Mesa; never alters production.

Run on a POSIX host with a native Mesa24.3.4 build and its compile_commands.
Only override objects in a new build output; retain original source/archives.
"""
import argparse
import hashlib
import json
import shlex
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BASE_SHA = 'be2167eda3bcb39dc62762fbc0d7b016e6b59798f33c8d20775b48c50e9dbba1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mesa-source', required=True)
    parser.add_argument('--mesa-build', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    source, build, out = (Path(p).resolve() for p in (args.mesa_source, args.mesa_build, args.output))
    assert out.is_relative_to(ROOT/'build') and not out.exists()
    original = source/'src/compiler/nir/nir_functions.c'
    assert sha(original) == BASE_SHA
    machines = json.loads((build/'meson-info/intro-machines.json').read_bytes())
    assert machines['host']['cpu_family'] == machines['build']['cpu_family']
    rows = json.loads((build/'compile_commands.json').read_bytes())
    patch = ROOT/'tools/build/fixtures/nir-straight-inline-prototype.patch'
    test = ROOT/'tools/build/test_nir_straight_inline.c'
    library_names = ['src/compiler/nir/libnir.a', 'src/compiler/libcompiler.a',
                     'src/util/libmesa_util.a', 'src/c11/impl/libmesa_util_c11.a',
                     'src/util/blake3/libblake3.a']
    libraries = [build/name for name in library_names]
    protected = [original, *libraries, build/'compile_commands.json']
    before = {str(p): sha(p) for p in protected}
    out.mkdir(parents=True)
    commands = []

    def run(command, name, cwd=build, binary=False):
        started = time.monotonic()
        result = subprocess.run(list(map(str, command)), cwd=cwd, capture_output=True, timeout=60)
        (out/(name+('.bin' if binary else '.log'))).write_bytes(result.stdout if binary else result.stdout+result.stderr)
        if binary:
            (out/(name+'.stderr')).write_bytes(result.stderr)
        commands.append(dict(name=name, command=list(map(str, command)), cwd=str(cwd),
                             returncode=result.returncode, seconds=time.monotonic()-started))
        assert result.returncode == 0, name+': '+result.stderr.decode(errors='replace')
        return result

    def compile_flags(filename):
        row = next(row for row in rows if row['file'].endswith('/'+filename))
        flags = shlex.split(row['command'])
        assert '-MD' in flags
        flags = flags[:flags.index('-MD')]
        flags = [v for v in flags if v not in ('-DNDEBUG', '-O3', '-O2') and not v.startswith('-DMESA_DEBUG=')]
        return flags+['-DDEBUG', '-DMESA_DEBUG=1', '-O1', '-g', '-fsanitize=address,undefined']

    copies = out/'prototype'
    copies.mkdir()
    (copies/'nir_functions.c').write_bytes(original.read_bytes())
    (out/'baseline.c').write_bytes(original.read_bytes())
    run(['git', 'apply', '--check', '--directory='+copies.relative_to(ROOT).as_posix(), patch], 'patch-check', ROOT)
    run(['git', 'apply', '--directory='+copies.relative_to(ROOT).as_posix(), patch], 'patch-apply', ROOT)
    flags = compile_flags('nir_validate.c')
    run([*flags, '-c', source/'src/compiler/nir/nir_validate.c', '-o', out/'validator.o'], 'debug-validator')
    run([*flags, '-c', test, '-o', out/'test.o'], 'authored-test')
    reports = []
    for variant, implementation in [('baseline', out/'baseline.c'), ('prototype', copies/'nir_functions.c')]:
        run([*compile_flags('nir_functions.c'), '-c', implementation, '-o', out/(variant+'.o')], variant+'-compile')
        executable = out/('test-'+variant)
        # All external functions from nir_functions.c are supplied by the
        # override object, so that archive member is never selected by ld.
        run(['c++', '-g', '-fsanitize=address,undefined', out/'test.o', out/'validator.o', out/(variant+'.o'),
             '-Wl,--start-group', *libraries, '-Wl,--end-group', '-ldl', '-lz', '-lpthread', '-lm',
             '-o', executable], variant+'-link')
        for count in (1, 64, 4096):
            name=variant+'-'+str(count)
            result=run([executable, str(count)], name, binary=True)
            assert b'validated=1' in result.stderr and b'remaining=0' in result.stderr
            reports.append(dict(variant=variant, calls=count, serialized_bytes=len(result.stdout),
                                serialized_sha256=hashlib.sha256(result.stdout).hexdigest(),
                                validator_summary=result.stderr.decode().strip()))
    for count in (1, 64, 4096):
        assert (out/('baseline-'+str(count)+'.bin')).read_bytes() == (out/('prototype-'+str(count)+'.bin')).read_bytes()
    assert before == {str(p): sha(p) for p in protected}
    report=dict(pinned_Mesa_commit='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de',
                source_files={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__), test, patch)},
                original_and_archives=before, commands=commands, graphs=reports,
                full_serialized_NIR_equal=True, ASan_UBSan_and_real_debug_validator=True,
                prototype_not_in_production=True, original_sources_and_archives_preserved=True, device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps(report))


if __name__ == '__main__':
    main()
