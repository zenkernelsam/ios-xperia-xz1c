"""v728: use XNU's full-width region wrapper; preserve the failed v727 input."""
import difflib,hashlib,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2];stage=root/'build/surface-readonly-v728'
source=stage/'before/SweetSurface.cpp';before=source.read_bytes();text=before.decode()
v727=json.loads((root/'reproducibility/surface-diag-v727.json').read_bytes())
path='iokit/Kernel/arm/SweetSurface.cpp'
assert hashlib.sha256(before).hexdigest()==v727['overrides'][path]
needle='struct sweet_caller_context { task_t task; IODirection direction; };'
assert text.count(needle)==1
text=text.replace(needle,'''/* Exact XNU mach_vm_server.h signature: direct kernel map query, full 64-bit address. */
extern "C" kern_return_t mach_vm_region_recurse(vm_map_t, mach_vm_address_t *, mach_vm_size_t *,
    natural_t *, vm_region_recurse_info_t, mach_msg_type_number_t *);
static_assert(sizeof(mach_vm_address_t) == 8 && sizeof(mach_vm_size_t) == 8, "Full-width VM query");
'''+needle)
needle='        vm_address_t address = cursor; vm_size_t bytes = 0; natural_t depth = 0;'
assert text.count(needle)==1
text=text.replace(needle,'        mach_vm_address_t address = cursor; mach_vm_size_t bytes = 0; natural_t depth = 0;')
needle='kr = vm_region_recurse_64(map, &address, &bytes, &depth, (vm_region_recurse_info_64_t)&info, &count);'
assert text.count(needle)==1
text=text.replace(needle,'kr = mach_vm_region_recurse(map, &address, &bytes, &depth, (vm_region_recurse_info_t)&info, &count);')
candidate=stage/'candidate';assert not candidate.exists();candidate.mkdir()
(candidate/'SweetSurface.cpp').write_bytes(text.encode())
delta=''.join(difflib.unified_diff(before.decode().splitlines(True),text.splitlines(True),fromfile='a/'+path,tofile='b/'+path)).encode()
patch=root/'xnu/lilac-surface-region64.patch';assert not patch.exists();patch.write_bytes(delta)
overrides=dict(v727['overrides']);overrides[path]=hashlib.sha256(text.encode()).hexdigest()
manifest=dict(version=728,base_version=727,source=path,before_sha256=hashlib.sha256(before).hexdigest(),
    after_sha256=overrides[path],patch_sha256=hashlib.sha256(delta).hexdigest(),overrides=overrides,
    changed_sources=[path],full_width_xnu_wrapper='mach_vm_region_recurse',
    v727_original_failure_preserved=True,source_permissions_unchanged=True,host_device_validation_pending=True)
(root/'reproducibility/surface-diag-v728.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
