"""Versioned actual rebuild, retaining objects, output and symlink before invalidation."""
import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT, audit, configuration, sha


def main():
    p = argparse.ArgumentParser(); p.add_argument('--version', type=int, choices=(725, 726,727,728), default=725)
    args = p.parse_args(); config = configuration(args.version)
    proof = audit(args.version)
    paths = [str(p) for p in (Path('/usr/local/bin'), Path('/opt/homebrew/bin')) if p.is_dir()]
    env = dict(os.environ, PATH=os.pathsep.join(paths + [os.environ.get('PATH', '')]),
               HOMEBREW_NO_AUTO_UPDATE='1', HOMEBREW_NO_INSTALL_CLEANUP='1',
               KERNEL_CONFIG='DEVELOPMENT', ARCH_CONFIG='ARM64', MACHINE_CONFIG='SWEET',
               MACOS_VERSION='13.5', INCREMENTAL='1')
    missing = [name for name in ('jq','gum','xcodes','cmake','ninja') if not shutil.which(name, path=env['PATH'])]
    assert not missing, 'Install the missing build tools before invalidation: ' + ', '.join(missing)
    stage = config['stage']; assert stage.is_dir()
    saved = stage / ('prior-build-' + time.strftime('%Y%m%d-%H%M%S'))
    assert not saved.exists(); saved.mkdir()
    build = (ROOT / 'build/dxb/build').resolve()
    moved = []

    def preserve(file, name=None):
        if not file.exists() and not file.is_symlink(): return
        target = saved / (name or file.name)
        assert not target.exists()
        # All moves are explicit files within this project's build output.
        assert file.absolute().is_relative_to(ROOT / 'build')
        file.rename(target); moved.append({'source': str(file), 'saved': str(target)})

    for name in ('build.log', 'build.rc', f'kernel.v{args.version}.macho', 'build-proof.json'):
        preserve(stage / name)
    folder = build / 'xnu.obj/DEVELOPMENT_ARM64_SWEET/iokit/DEVELOPMENT'
    for path in config['changed_sources']:
        if Path(path).suffix not in ('.cpp','.c'):continue
        stem = Path(path).stem
        for suffix in ('.cpo', '.cpd'): preserve(folder / (stem + suffix))
    symbol = build / 'xnu.sym/kernel.development.sweet'
    assert symbol.is_file()
    shutil.copy2(symbol, saved / 'kernel.previous.macho')
    link = build / 'xnu.obj/kernel.development.sweet'
    if link.is_symlink():
        stripped = link.resolve()
        assert stripped.is_relative_to(build) and stripped.is_file() and stripped != symbol
        preserve(link, 'kernel.development.sweet.link')
        preserve(stripped, 'kernel.development.sweet.stripped')
    else:
        # A preceding failed build may already have preserved this link/target.
        assert not link.exists(), 'Unexpected regular top-level kernel output'
    (saved / 'preserved.json').write_bytes((json.dumps(moved, indent=2) + '\n').encode())
    (stage / 'source-audit-before.json').write_bytes((json.dumps(proof, indent=2) + '\n').encode())
    print(f"v{args.version}: {proof['audited_sources']} sources verified; prior outputs retained", flush=True)
    with (stage / 'build.log').open('wb') as log:
        process = subprocess.run(['bash', 'build.sh'], cwd=ROOT / 'build/dxb', env=env, stdout=log, stderr=subprocess.STDOUT)
    (stage / 'build.rc').write_text(str(process.returncode) + '\n')
    if process.returncode: return process.returncode
    raw = symbol.read_bytes(); log_text = (stage / 'build.log').read_text(errors='replace')
    for path in config['changed_sources']:
        if Path(path).suffix not in ('.cpp','.c'):continue
        assert Path(path).name in log_text, 'Build skipped changed source: ' + path
    destination = stage / f'kernel.v{args.version}.macho'
    assert not destination.exists(); shutil.copy2(symbol, destination)
    proof = audit(args.version, sha(raw))
    proof.update(build_return_code=0, changed_sources_recompiled=[p for p in config['changed_sources'] if Path(p).suffix in ('.cpp','.c')],
                 changed_headers=[p for p in config['changed_sources'] if Path(p).suffix=='.h'],
                 prior_outputs_preserved=str(saved), kernel_bytes=len(raw))
    (stage / 'build-proof.json').write_bytes((json.dumps(proof, indent=2) + '\n').encode())
    print(json.dumps(proof, indent=2), flush=True)
    return 0


if __name__ == '__main__': raise SystemExit(main())
