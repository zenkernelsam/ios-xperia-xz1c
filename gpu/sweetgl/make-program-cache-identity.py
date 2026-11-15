"""Bind private program binaries to exact frontend, Mesa archives and defines."""
import argparse,hashlib,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--mesa-build',required=True)
p.add_argument('--defines',required=True);p.add_argument('--output',required=True);a=p.parse_args()
source=Path(a.source);mesa=Path(a.mesa_build);output=Path(a.output)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
files={str(path.relative_to(source)):sha(path) for path in source.rglob('*') if path.is_file() and not any(part in ('out','__pycache__') for part in path.relative_to(source).parts)}
names=re.search(r'LIBS="([^"]+)"',(source/'build.sh').read_text()).group(1).split()+['src/mapi/shared-glapi/libglapi.0.dylib']
binding={'frontend_sources':files,'Mesa_archives':{name:sha(mesa/name) for name in names},'Mesa_defines_sha256':sha(Path(a.defines)),'schema':1}
digest=hashlib.sha256(json.dumps(binding,sort_keys=True,separators=(',',':')).encode()).digest()
output.write_text('/* Generated from exact current input hashes. */\nstatic const unsigned char sweet_program_cache_build_id[32]={'+','.join(str(b) for b in digest)+'};\n',encoding='utf-8')
output.with_suffix('.json').write_text(json.dumps({'build_identity':digest.hex(),'inputs':binding},indent=2)+'\n',encoding='utf-8')
