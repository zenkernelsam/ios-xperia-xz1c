"""Install only the fixed additive IR3 patch/header into the pinned Mesa tree."""
import hashlib,json,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
def install():
    lock=json.loads((ROOT/'reproducibility/source-lock.json').read_bytes())
    spec=lock['mesa']['IR3_variant_cache'];tree=ROOT/'deps/mesa';patch=ROOT/spec['patch']
    assert hashlib.sha256(patch.read_bytes()).hexdigest()==spec['patch_sha256']
    assert subprocess.check_output(['git','-C',str(tree),'rev-parse','HEAD'],text=True).strip()==lock['mesa']['base']
    reverse=subprocess.run(['git','-C',str(tree),'apply','--reverse','--check',str(patch)],capture_output=True)
    if reverse.returncode:
        subprocess.run(['git','-C',str(tree),'apply','--check',str(patch)],check=True)
        subprocess.run(['git','-C',str(tree),'apply',str(patch)],check=True)
    source=ROOT/spec['header'];destination=tree/'src/freedreno/ir3/lilac_ir3_variant_cache.h';raw=source.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==spec['header_sha256']
    if destination.exists():assert destination.read_bytes()==raw,'Unknown pre-existing IR3 cache header'
    else:destination.write_bytes(raw)
    for name,digest in spec['patched_sources'].items():assert hashlib.sha256((tree/name).read_bytes()).hexdigest()==digest,name
    return spec

if __name__=='__main__':print(json.dumps(install()))
