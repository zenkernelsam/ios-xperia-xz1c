"""Materialize the retained baseline's exact line endings without changing code."""
import hashlib
import json
from pathlib import Path, PurePosixPath


def materialize(project, tree):
    lock = json.loads((Path(project) / 'reproducibility/source-lock.json').read_bytes())
    recipe = lock['xnu'].get('baseline_source_newlines')
    if not recipe: return []
    assert recipe['base_full_patch_sha256'] == lock['xnu']['patch_sha256']
    changed = []
    sha = lambda data: hashlib.sha256(data).hexdigest()
    for path, spec in recipe['sources'].items():
        relative = PurePosixPath(path)
        assert not relative.is_absolute() and '..' not in relative.parts
        assert spec['after_sha256'] == lock['audited_xnu_files'][path]
        file = Path(tree) / path
        before = file.read_bytes()
        if sha(before) == spec['after_sha256']: continue
        assert sha(before) == spec['before_sha256'], 'Unknown baseline source: ' + path
        lines = before.splitlines(True)
        assert len(lines) == spec['line_count'] and all(line.endswith(b'\n') for line in lines)
        lf = set(spec['lf_only_indices'])
        assert all(type(i) is int and 0 <= i < len(lines) for i in lf)
        after = b''.join(line if i in lf else line[:-1] + b'\r\n' for i, line in enumerate(lines))
        assert after.replace(b'\r\n', b'\n') == before and sha(after) == spec['after_sha256']
        file.write_bytes(after)
        changed.append(path)
    return changed
