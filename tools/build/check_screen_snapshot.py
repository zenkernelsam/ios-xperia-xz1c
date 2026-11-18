"""Check the real shared codec and host packet receiver before kernel installation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).absolute()
    assert out.is_relative_to(ROOT / 'build') and not out.exists()
    out.mkdir(parents=True)

    def run(command, name):
        result = subprocess.run(list(map(str, command)), capture_output=True, text=True)
        (out / (name + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        assert result.returncode == 0, result.stdout + result.stderr
        return result.stdout

    source = ROOT / 'tools/build/test_screen_codec.c'
    run(['clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
         source, '-o', out / 'codec-asan'], 'asan-ubsan-compile')
    assert 'PASS' in run([out / 'codec-asan', out], 'asan-ubsan-test')
    assert 'PASS' in run([sys.executable, ROOT / 'tools/build/test_screen_wire.py', out], 'wire-test')
    run(['xcrun', '--sdk', 'macosx', 'clang', '-arch', 'arm64', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
         '-c', source, '-o', out / 'arm64.o'], 'arm64-compile')
    files = ['xnu/support/SweetFramebufferScreen.h', 'xnu/support/SweetFramebufferScreenRuntime.h',
             'tools/device/scanout_snapshot.py', 'tools/build/test_screen_codec.c', 'tools/build/test_screen_wire.py']
    report = dict(source_files={path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in files},
        ASan_UBSan_passed=True, ARM64_compile_passed=True, wire_fault_tests_passed=True,
        full_panel_exact_pixels_passed=True, host_fixtures_only=True, device_operations=0, hardware_acceptance_pending=True)
    (out / 'manifest.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
