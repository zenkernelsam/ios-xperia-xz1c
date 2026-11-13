"""Install only the checked single-source v728 wrapper correction."""
import json,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT,configuration,audit
config=configuration(728);stage=ROOT/'build/surface-readonly-v728'
proof=json.loads((stage/'host-checks.json').read_bytes())
assert proof['actual_old_failure_reproduced'] and proof['actual_new_direction_and_wrapper_asan_ubsan_passed']
assert proof['patch_sha256']==config['manifest']['patch_sha256']
tree=ROOT/'build/dxb/xnu';patch=ROOT/'xnu/lilac-surface-region64.patch'
reverse=subprocess.run(['git','apply','--reverse','--check',str(patch)],cwd=tree,capture_output=True)
if reverse.returncode:
    audit(727)
    subprocess.run(['git','apply','--check',str(patch)],cwd=tree,check=True)
    subprocess.run(['git','apply',str(patch)],cwd=tree,check=True)
result=audit(728);output=ROOT/'build/surface-diag-v728';output.mkdir(exist_ok=True)
(output/'installation.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
print(json.dumps(result,indent=2))
