"""Prepare and sanitize v726 metadata logs; default leaves the live XNU source intact."""
import argparse
import difflib
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'iokit/Kernel/IOMemoryDescriptor.cpp'
BEFORE = '9963ef1a82cfe5e1bfca797c17c686c54eeea6e631ed349babfc80ac8c824379'
ENTRY = r'''
					/* v726: bounded failure metadata, original memory-entry request unchanged. */
					if ((_flags & kIOMemoryPersistent) && _task && _task != kernel_task) {
						static volatile SInt32 lilac_entry_failures;
						SInt32 count = OSIncrementAtomic(&lilac_entry_failures) + 1;
						if (count <= 24) IOLog("SWEET SURF MEMREF ERROR address=%#llx requested=%llu actual=%llu prot=%#x page_mask=%#llx kr=%d count=%d\n",
						    (unsigned long long)entryAddr, (unsigned long long)entrySize,
						    (unsigned long long)actualSize, prot,
						    map ? (unsigned long long)vm_map_page_mask(map) : 0, err, count);
					}
'''
INIT = r'''
					/* v726: retain the true creation error hidden by withAddressRange(NULL). */
					if ((_flags & kIOMemoryPersistent) && _task && _task != kernel_task) {
						static volatile SInt32 lilac_init_failures;
						SInt32 count = OSIncrementAtomic(&lilac_init_failures) + 1;
						if (count <= 24) IOLog("SWEET SURF MEMREF INIT length=%llu flags=%#x kr=%#x count=%d\n",
						    (unsigned long long)_length, (uint32_t)_flags, err, count);
					}
'''


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    stage = ROOT / 'build/surface-diag-v726'
    assert not stage.exists(), 'Preserve an existing candidate; do not overwrite it'
    tree = ROOT / 'build/dxb/xnu'
    source = tree / SOURCE
    before = source.read_bytes()
    assert sha(before) == BEFORE, 'IOMemoryDescriptor must match the pinned Apple input'
    lock = json.loads((ROOT / 'reproducibility/source-lock.json').read_text())
    old = json.loads((ROOT / 'reproducibility/surface-diag-v725.json').read_text())
    for path, digest in lock['audited_xnu_files'].items():
        assert sha((tree / path).read_bytes()) == (old['after_sha256'] if path == old['source'] else digest), path
    text = before.decode()
    needle = '\t\t\t\tif (KERN_SUCCESS != err) {\n\t\t\t\t\tDEBUG4K_ERROR("make_memory_entry'
    assert text.count(needle) == 1
    text = text.replace(needle, '\t\t\t\tif (KERN_SUCCESS != err) {' + ENTRY + '\t\t\t\t\tDEBUG4K_ERROR("make_memory_entry')
    needle = '\t\t\t\tif (kIOReturnSuccess != err) {\n\t\t\t\t\treturn false;\n\t\t\t\t}\n\t\t\t}\n\n\t\t\tif ((_flags & kIOMemoryAutoPrepare)'
    assert text.count(needle) == 1
    text = text.replace(needle, '\t\t\t\tif (kIOReturnSuccess != err) {' + INIT + '\t\t\t\t\treturn false;\n\t\t\t\t}\n\t\t\t}\n\n\t\t\tif ((_flags & kIOMemoryAutoPrepare)')
    assert text.replace(ENTRY, '\n').replace(INIT, '\n').encode() == before
    after = text.encode()
    stage.mkdir()
    (stage / 'IOMemoryDescriptor.before.cpp').write_bytes(before)
    (stage / 'IOMemoryDescriptor.candidate.cpp').write_bytes(after)
    delta = ''.join(difflib.unified_diff(before.decode().splitlines(True), text.splitlines(True),
                  fromfile='a/' + SOURCE, tofile='b/' + SOURCE)).encode()
    patch = ROOT / 'xnu/lilac-surface-memref-diagnostics.patch'
    if patch.exists():
        assert patch.read_bytes() == delta
    else:
        patch.write_bytes(delta)
    # Compile the exact inserted blocks, including their atomic counter and NULL-map guards.
    harness = r'''
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
using SInt32=int32_t;
constexpr uint32_t kIOMemoryPersistent=1;
void *kernel_task=(void*)2;
std::atomic<unsigned> logs{0},metadata{0};
SInt32 OSIncrementAtomic(volatile SInt32*p){return __atomic_fetch_add(p,1,__ATOMIC_SEQ_CST);}
uint64_t vm_map_page_mask(void*p){assert(p);++metadata;return 4095;}
void IOLog(const char*,...){++logs;}
void entry_log(int err,uint32_t _flags,void *_task,void *map){
 uint64_t entryAddr=0x107348000,entrySize=32768,actualSize=19044;uint32_t prot=3;
 if(err){
''' + ENTRY + r'''
 }
}
void init_log(int err,uint32_t _flags,void *_task){uint64_t _length=32768;
 if(err){
''' + INIT + r'''
 }
}
int main(){
 for(int i=0;i<30;++i){entry_log(0,1,(void*)1,(void*)1);init_log(0,1,(void*)1);
  entry_log(1,0,(void*)1,(void*)1);init_log(1,0,(void*)1);
  entry_log(1,1,nullptr,(void*)1);init_log(1,1,nullptr);
  entry_log(1,1,kernel_task,(void*)1);init_log(1,1,kernel_task);}
 assert(logs==0&&metadata==0);
 entry_log(1,1,(void*)1,nullptr);assert(logs==1&&metadata==0);
 for(int i=0;i<30;++i)entry_log(2,1,(void*)1,(void*)1);
 assert(logs==24&&metadata==23);
 for(int i=0;i<30;++i)init_log(2,1,(void*)1);
 assert(logs==48&&metadata==23);
 puts("actual diagnostic blocks: success/flags/task/NULL-map/24-per-site bounds PASS");
}
'''
    fixture = stage / 'test_diagnostics.cpp'
    fixture.write_bytes(harness.encode())
    binary = stage / 'test_diagnostics'
    compile_result = subprocess.run(['clang++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                    str(fixture), '-o', str(binary)], capture_output=True, text=True)
    (stage / 'host-compile.log').write_text(compile_result.stdout + compile_result.stderr)
    assert compile_result.returncode == 0, compile_result.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    (stage / 'host-test.log').write_text(run.stdout + run.stderr)
    assert run.returncode == 0, run.stdout + run.stderr
    candidate = {'version': 726, 'source': SOURCE, 'before_sha256': BEFORE, 'after_sha256': sha(after),
                 'patch_sha256': sha(delta), 'overrides': {old['source']: old['after_sha256'], SOURCE: sha(after)},
                 'failure_messages_per_site': 24, 'original_source_after_removing_logs_identical': True,
                 'actual_inserted_blocks_asan_ubsan_passed': True,
                 'memory_content_reads': 0, 'mmio_writes': 0, 'device_operations': 0,
                 'installed': args.install}
    if args.install:
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=tree, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=tree, check=True)
        assert sha(source.read_bytes()) == sha(after)
    else:
        assert source.read_bytes() == before
    (stage / 'candidate.json').write_bytes((json.dumps(candidate, indent=2) + '\n').encode())
    print(json.dumps(candidate, indent=2))


if __name__ == '__main__':
    main()
