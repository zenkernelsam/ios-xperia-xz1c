"""Install only the checked logging delta, preserving exact CRLF predecessor files."""
import json,shutil,subprocess,sys,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT,configuration,audit,sha
config=configuration(729);m=config['manifest'];tree=ROOT/'build/dxb/xnu'
proof=json.loads((ROOT/'build/console-snapshot-v729/host-checks/manifest.json').read_bytes())
assert proof['snapshot_boundaries_and_concurrency_ASan_UBSan_TSan_passed'] and proof['ARM64_compile_passed']
assert proof['header_sha256']==config['source_hashes']['bsd/dev/vermeer_log_ring.h']
audit(728)
backup=ROOT/'build/console-snapshot-v729'/('installed-from-'+time.strftime('%Y%m%d-%H%M%S'));backup.mkdir()
for path,before in m['base_sources'].items():
    file=tree/path;raw=file.read_bytes();assert sha(raw) in (before,m['prior_actual_sources'][path])
    saved=backup/path;saved.parent.mkdir(parents=True,exist_ok=True);saved.write_bytes(raw)
    normalized=raw.replace(b'\r\n',b'\n');assert sha(normalized)==before;file.write_bytes(normalized)
patch=ROOT/'xnu/lilac-console-snapshot.patch'
subprocess.run(['git','apply','--check',patch],cwd=tree,check=True)
subprocess.run(['git','apply',patch],cwd=tree,check=True)
stage=ROOT/'build/surface-diag-v729';stage.mkdir(exist_ok=True)
result=audit(729);(stage/'installation.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
print(json.dumps(result))
