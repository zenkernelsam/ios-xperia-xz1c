"""Pin the MIT Mesa binary16 source/notices used by the ESSL adaptation."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--mesa',required=True);a=p.parse_args();mesa=Path(a.mesa).resolve()
lock=json.loads((root/'reproducibility/source-lock.json').read_bytes());revision=lock['mesa']['base']['revision'] if isinstance(lock['mesa']['base'],dict) else lock['mesa']['base']
if not isinstance(revision,str) or len(revision)!=40:revision='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de'
actual=subprocess.check_output(['git','-C',str(mesa),'rev-parse','HEAD'],text=True).strip();assert actual==revision
destination=root/'gpu/metal/air/half/reference';assert not destination.exists();destination.mkdir(parents=True)
sources={}
for name in ('src/util/half_float.c','src/util/half_float.h'):
    raw=subprocess.check_output(['git','-C',str(mesa),'show',revision+':'+name]);target=destination/Path(name).name;target.write_bytes(raw)
    sources[name]=dict(destination=target.relative_to(root).as_posix(),sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw))
report=dict(repository='https://gitlab.freedesktop.org/mesa/mesa',revision=revision,sources=sources,license='MIT notices retained in original files and HalfESSL.h',
    adaptation='gpu/metal/air/HalfESSL.h',GPU_conversion_uses_integer_bit_rounding=True,device_operations=0)
(root/'reproducibility/half-reference.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
