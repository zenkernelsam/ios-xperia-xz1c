"""Generate v727 read-only shared-backing changes from audited v726 files; no install."""
import difflib,hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[2];stage=root/'build/surface-readonly-v727'
before=stage/'before';after=stage/'candidate';assert not after.exists();after.mkdir()
lock=json.loads((root/'reproducibility/source-lock.json').read_bytes())
v726=json.loads((root/'reproducibility/surface-diag-v726.json').read_bytes())
def sha(raw):return hashlib.sha256(raw).hexdigest()
files={name:(before/name).read_text(encoding='utf-8') for name in ('SweetSurface.cpp','SweetSurface.h','SGBufferLease.h','SGSharedMapper.cpp','SweetGPUScatter.cpp')}
original=dict(files)
for name,text in original.items():
    path='iokit/Kernel/arm/'+name
    assert sha(text.encode())==v726['overrides'].get(path,lock['audited_xnu_files'][path]),name

s=files['SweetSurface.cpp']
s=s.replace('#include <sys/proc.h>','#include <sys/proc.h>\n#include <vm/vm_map.h>\n#include <kern/task.h>')
# Two modes use the exact same descriptor lifetime; only access direction differs.
s=s.replace('surface->callerBacking.state == SCA_PREPARED ? kIOMapUnique : 0);',
            'surface->callerBacking.state == SCA_PREPARED ? kIOMapUnique : 0) |\n        (surface->callerReadOnly ? kIOMapReadOnly : 0);')
needle='static void *sweet_caller_allocate(void *task, uint64_t start, uint64_t bytes)'
assert s.count(needle)==1
helper='''struct sweet_caller_context { task_t task; IODirection direction; };
static IOReturn sweet_caller_direction(task_t task, const sca_range *range, IODirection *direction)
{
    if (!task || !range || !direction) return kIOReturnBadArgument;
    vm_map_t map = get_task_map(task);
    if (!map) return kIOReturnBadArgument;
    vm_map_offset_t cursor = range->storage_start, end = cursor + range->storage_bytes;
    bool writable = true;
    for (unsigned regions = 0; cursor < end && regions < 64; ++regions) {
        vm_address_t address = cursor; vm_size_t bytes = 0; natural_t depth = 0;
        vm_region_submap_info_data_64_t info = {}; kern_return_t kr = KERN_FAILURE;
        bool metadata_valid = false;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
            kr = vm_region_recurse_64(map, &address, &bytes, &depth, (vm_region_recurse_info_64_t)&info, &count);
            metadata_valid = !kr && count >= VM_REGION_SUBMAP_INFO_COUNT_64;
            if (kr || count < VM_REGION_SUBMAP_INFO_COUNT_64 || !info.is_submap) break;
            ++depth;
        }
        if (!metadata_valid || info.is_submap || address > cursor || !bytes || bytes > UINT64_MAX-address ||
            address+bytes <= cursor || !(info.protection & VM_PROT_READ)) return kIOReturnNotReadable;
        writable = writable && (info.protection & VM_PROT_WRITE) && (info.max_protection & VM_PROT_WRITE);
        cursor = address+bytes < end ? address+bytes : end;
    }
    if (cursor != end) return kIOReturnBadArgument;
    *direction = writable ? kIODirectionInOut : kIODirectionOut;
    return kIOReturnSuccess;
}
'''
s=s.replace(needle,helper+'static void *sweet_caller_allocate(void *opaque, uint64_t start, uint64_t bytes)')
s=s.replace('''    IOMemoryDescriptor *descriptor = IOMemoryDescriptor::withAddressRange(start, bytes,
        kIODirectionInOut | kIOMemoryMapperNone | kIOMemoryThreadSafe | kIOMemoryPersistent,
        (task_t)task);''','''    auto *context = (sweet_caller_context *)opaque;
    IOMemoryDescriptor *descriptor = IOMemoryDescriptor::withAddressRange(start, bytes,
        context->direction | kIOMemoryMapperNone | kIOMemoryThreadSafe | kIOMemoryPersistent,
        context->task);''')
s=s.replace('memory->prepare(kIODirectionInOut)','memory->prepare(memory->getDirection())')
s=s.replace('->complete(kIODirectionInOut)','->complete(((IOMemoryDescriptor *)descriptor)->getDirection())')
s=s.replace('static struct sca_ops sweet_caller_ops(task_t task)\n{ return {(void *)task,',
            'static struct sca_ops sweet_caller_ops(sweet_caller_context *context)\n{ return {(void *)context,')
s=s.replace('auto ops = sweet_caller_ops(TASK_NULL);','auto ops = sweet_caller_ops(nullptr);')
s=s.replace('kernelMap = memory->map(kIOMapDefaultCache);',
            'kernelMap = memory->map(kIOMapDefaultCache | (callerReadOnly ? kIOMapReadOnly : 0));')
needle='    if (!sca_range_plan(address, rawBytes, end, page_size, &range)) return kIOReturnBadArgument;'
assert s.count(needle)==1
s=s.replace(needle,needle+'''
    sweet_caller_context context = { task, kIODirectionInOut };
    IOReturn access = sweet_caller_direction(task, &range, &context.direction);
    if (access) return access;
    surface->callerReadOnly = context.direction == kIODirectionOut;
''')
s=s.replace('auto ops = sweet_caller_ops(task);','auto ops = sweet_caller_ops(&context);')
needle='    return kIOReturnSuccess;\n}\n\n/* Per-client mapping'
assert s.count(needle)==1
s=s.replace(needle,'''    if (surface->callerReadOnly) IOLog("SWEET SURF READONLY id=%u width=%u height=%u bytes=%llu source=%#llx direction=out cpu_write=0 gpu_write=0\\n",
        surface->id, surface->width, surface->height, (unsigned long long)rawBytes, (unsigned long long)address);
'''+needle)
s=s.replace('trapLock(void *id, __unused void *options,', 'trapLock(void *id, void *options,')
needle='\tIOReturn ret = sweet_surface_seed_out(s, seedOut, false);'
assert s.count(needle)==1
s=s.replace(needle,'\tIOReturn ret = s->callerReadOnly && !((uintptr_t)options & kIOSurfaceLockReadOnly) ?\n\t    kIOReturnNotWritable : sweet_surface_seed_out(s, seedOut, false);')
needle='\tIOReturn ret = sweet_surface_seed_out(s, seedOut,\n\t    !((uintptr_t)options & kIOSurfaceLockReadOnly));'
assert s.count(needle)==1
s=s.replace(needle,'\tIOReturn ret = s->callerReadOnly && !((uintptr_t)options & kIOSurfaceLockReadOnly) ?\n\t    kIOReturnNotWritable : sweet_surface_seed_out(s, seedOut,\n\t    !((uintptr_t)options & kIOSurfaceLockReadOnly));')
files['SweetSurface.cpp']=s
s=files['SweetSurface.h'];s=s.replace('struct sca_backing callerBacking;','struct sca_backing callerBacking;\n\tbool callerReadOnly;')
s=s.replace('callerBacking = {}; return','callerBacking = {}; callerReadOnly = false; return');files['SweetSurface.h']=s
s=files['SGBufferLease.h'];s=s.replace('unsigned pinned;','unsigned pinned;\n    unsigned read_only; /* Immutable mapped permission for this lease lifetime. */');files['SGBufferLease.h']=s
s=files['SGSharedMapper.cpp'];needle='static int valid_ops';s=s.replace(needle,'''static uint64_t mapping_attributes(const sg_shared_record *record) {
    return attributes | (record->data->read_only ? (UINT64_C(1)<<7) : 0);
}
'''+needle,1)
s=s.replace('!data->descriptor||!data->pinned||data->generation!=generation',
            '!data->descriptor||!data->pinned||data->read_only>1||data->generation!=generation')
assert s.count('|attributes;')==4
s=s.replace('(other->physical[leaf/4]+(leaf%4)*4096)|attributes','(other->physical[leaf/4]+(leaf%4)*4096)|mapping_attributes(other)')
s=s.replace('(physical[leaf/4]+(leaf%4)*4096)|attributes','(physical[leaf/4]+(leaf%4)*4096)|mapping_attributes(r)')
s=s.replace('(r->physical[leaf/4]+(leaf%4)*4096)|attributes','(r->physical[leaf/4]+(leaf%4)*4096)|mapping_attributes(r)')
files['SGSharedMapper.cpp']=s
s=files['SweetGPUScatter.cpp']
s=s.replace('->prepare(kIODirectionInOut)','->prepare(static_cast<IOMemoryDescriptor *>(d)->getDirection())')
s=s.replace('->complete(kIODirectionInOut)','->complete(static_cast<IOMemoryDescriptor *>(d)->getDirection())')
s=s.replace('auto ops=buffer_ops(b);sg_lease_init(&b->lease,b->generation);sg_status result;',
'''auto ops=buffer_ops(b);sg_lease_init(&b->lease,b->generation);sg_status result;
    b->lease.read_only=surface && surface->callerReadOnly;''')
files['SweetGPUScatter.cpp']=s

delta='';overrides=dict(v726['overrides'])
for name,text in files.items():
    assert text!=original[name],name
    (after/name).write_bytes(text.encode())
    path='iokit/Kernel/arm/'+name
    delta+=''.join(difflib.unified_diff(original[name].splitlines(True),text.splitlines(True),fromfile='a/'+path,tofile='b/'+path))
    overrides[path]=sha(text.encode())
patch=root/'xnu/lilac-surface-readonly.patch';assert not patch.exists();patch.write_bytes(delta.encode())
manifest={'version':727,'base_version':726,'patch_sha256':sha(delta.encode()),'overrides':overrides,
          'changed_sources':['iokit/Kernel/arm/'+name for name in files],
          'read_only_descriptor_direction':'kIODirectionOut','cpu_map_read_only':True,'gpu_leaf_ap_bit7_read_only':True,
          'existing_readwrite_path_retained':True,'source_permissions_unchanged':True,
          'host_and_device_validation_pending':True}
(root/'reproducibility/surface-diag-v727.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
