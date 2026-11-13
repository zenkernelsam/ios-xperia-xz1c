"""Prepare the logging-only v729 delta from the retained exact v728 sources."""
import difflib,hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[2];stage=root/'build/console-snapshot-v729'
before=stage/'before';candidate=stage/'candidate';candidate.mkdir(exist_ok=True)
assert not any(candidate.iterdir()),'Retain prior generated outputs rather than overwrite them'
sha=lambda data:hashlib.sha256(data).hexdigest()
paths=['bsd/dev/vermeer_debug.c','bsd/dev/vermeer_log_ring.h'];changes={}
old=(before/'vermeer_debug.c').read_bytes();normalized=old.replace(b'\r\n',b'\n');text=normalized.decode()
start=text.index('unsigned vermeer_debug_copy(uint64_t *pos, char *out, unsigned max)\n{')
stop=text.index('uint64_t vermeer_debug_dropped(void)',start)
text=text[:start]+'''unsigned vermeer_debug_copy(uint64_t *pos, char *out, unsigned max)
{
    return vl_copy(&log_ring,pos,out,max);
}
uint64_t vermeer_debug_total(void)
{
    return atomic_load_explicit(&log_ring.total,memory_order_acquire);
}
'''+text[stop:]
assert text[:start].encode()==normalized[:start]
changes[paths[0]]=text.encode();changes[paths[1]]=(root/'xnu/support/vermeer_log_ring.h').read_bytes()
delta='';base_sources={};after_sources={};prior_actual_sources={}
for path in paths:
    actual=(before/Path(path).name).read_bytes();base=actual.replace(b'\r\n',b'\n');after=changes[path]
    (candidate/Path(path).name).write_bytes(after)
    base_sources[path]=sha(base);after_sources[path]=sha(after)
    prior_actual_sources[path]=sha(actual)
    delta+=''.join(difflib.unified_diff(base.decode().splitlines(True),after.decode().splitlines(True),fromfile='a/'+path,tofile='b/'+path))
patch=root/'xnu/lilac-console-snapshot.patch';assert not patch.exists();patch.write_bytes(delta.encode())
previous=json.loads((root/'reproducibility/surface-diag-v728.json').read_bytes());overrides=dict(previous['overrides']);overrides.update(after_sources)
manifest=dict(version=729,base_version=728,base_sources=base_sources,changed_sources=paths,overrides=overrides,
    prior_actual_sources=prior_actual_sources,base_sources_normalized_to_LF=True,
    patch_sha256=sha(delta.encode()),logging_only=True,writer_still_nonblocking=True,reader_does_not_hold_writer_lock=True,
    GPU_allocator_SCM_PON_RPC_VM_code_unchanged=True,hardware_acceptance_pending=True)
(root/'reproducibility/surface-diag-v729.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());print(json.dumps(manifest))
