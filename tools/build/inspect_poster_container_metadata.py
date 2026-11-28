"""Read original gl5 container metadata without mounting or modifying the image."""
import argparse
import hashlib
import json
import plistlib
from pathlib import Path, PurePosixPath

from hfs_sparse_reader import AndroidSparseImage, HFS

GL5_SHA = 'e9bf1547b6a7e9a33b51c2749c643bc2eff0134c31cc3a63f9435e4cea1f9b29'
METADATA = '.com.apple.mobile_container_manager.metadata.plist'
IDENTIFIER = 'MCMMetadataIdentifier'
TARGET = 'com.apple.PosterBoard'
BASES = ('/private/var/mobile/Containers/Data/Application',
         '/private/var/mobile/Containers/Shared/AppGroup',
         '/private/var/containers/Data/System',
         '/private/var/containers/Shared/SystemGroup')


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def relevant(identifier):
    return isinstance(identifier, str) and any(
        word in identifier.lower() for word in ('poster', 'wallpaper', 'aegir'))


def inspect(fs):
    rows, matches = [], []
    for path in sorted(fs.files):
        if PurePosixPath(path).name != METADATA:
            continue
        fork = fs.open(path)
        if not 0 < fork.size <= 65536:
            raise ValueError('Container metadata exceeds bounds: ' + path)
        raw = fork.read_at(0, fork.size)
        obj = plistlib.loads(raw)
        if not isinstance(obj, dict) or not isinstance(obj.get(IDENTIFIER), str):
            raise ValueError('Container identity missing: ' + path)
        identifier = obj[IDENTIFIER]
        if relevant(identifier):
            row = dict(path=path, bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                       identifier=identifier, identity_key=IDENTIFIER,
                       container_path=str(PurePosixPath(path).parent))
            rows.append(row)
            if identifier == TARGET:
                matches.append(row)
    files = []
    for row in matches:
        root = row['container_path'] + '/'
        for path in sorted(fs.files):
            if not path.startswith(root) or PurePosixPath(path).name == METADATA:
                continue
            fork = fs.open(path)
            record = dict(path=path, bytes=fork.size)
            # Inspect metadata only; do not dump assets or unrelated payloads.
            if 0 < fork.size <= 65536 and PurePosixPath(path).suffix in ('.plist', '.json'):
                raw = fork.read_at(0, fork.size)
                record['sha256'] = hashlib.sha256(raw).hexdigest()
                try:
                    obj = json.loads(raw) if path.endswith('.json') else plistlib.loads(raw)
                except (ValueError, plistlib.InvalidFileException):
                    obj = None
                record['top_level_keys'] = sorted(map(str, obj)) if isinstance(obj, dict) else None
            files.append(record)
            if len(files) > 4096:
                raise ValueError('PosterBoard container file count exceeds bounds')
    return dict(metadata_files_checked=sum(PurePosixPath(p).name == METADATA for p in fs.files),
                relevant_containers=rows, exact_PosterBoard_containers=matches,
                exact_PosterBoard_container_files=files,
                expected_current_roots=list(BASES),
                original_image_only=True, current_phone_configuration_not_proven=True,
                selected_wallpaper_not_inferred=True, device_operations=0, image_writes=0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--image', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    image_path, output = Path(args.image).resolve(), Path(args.output).resolve()
    if output.exists():
        raise ValueError('Output must be new')
    before = image_path.stat()
    image_sha = digest(image_path)
    if image_sha != GL5_SHA:
        raise ValueError('Expected protected gl5 input')
    image = AndroidSparseImage(image_path)
    try:
        report = inspect(HFS(image))
    finally:
        image.close()
    after = image_path.stat()
    if (before.st_size, before.st_mtime_ns, before.st_ino) != (
            after.st_size, after.st_mtime_ns, after.st_ino):
        raise ValueError('Input changed during inspection')
    report.update(image_sha256=image_sha, image_bytes=before.st_size,
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
