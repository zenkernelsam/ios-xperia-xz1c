"""Byte-bound read-only RAM frontend packages, independent of the boot transport."""
import hashlib,json,struct,zlib,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
API='a8f1e5a6693c82fbf34c438868723bcd61cc30f1829e57f17519277dba252bb3'
HELPER='e65bdc7b70d098725918c637cc8cb1c25a8a544186d0f4468b399fda52024898'
BASELINE={'library_sha256':'9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571',
          'library_bytes':14675424,'library_fnv64':'bb763317000ee0d7'}
def sha(data):return hashlib.sha256(data).hexdigest()
def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'
def load(folder,expected=None):
    folder=Path(folder).resolve();assert folder.is_relative_to(ROOT/'build')
    manifest_bytes=(folder/'manifest.json').read_bytes();m=json.loads(manifest_bytes)
    assert m['read_only_verified'] and m['aliases_m_g_verified']
    assert m['api_sha256']==API and m['helper_sha256']==HELPER
    blob=(folder/'glram.zblob').read_bytes()
    assert len(blob)==m['blob_bytes'] and sha(blob)==m['blob_sha256'] and len(blob)<=8<<20
    magic,version,header,raw_bytes,packed_bytes,raw_fnv=struct.unpack_from('<8sIIQQQ',blob)
    assert (magic,version,header)==(b'SGLZ0001',1,40)
    assert raw_bytes==m['raw_bytes']==32<<20 and packed_bytes==len(blob)-40
    decoder=zlib.decompressobj();raw=decoder.decompress(blob[40:],raw_bytes+1)
    assert len(raw)==raw_bytes and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert raw[1024:1026]==b'H+' and sha(raw)==m['raw_sha256']
    assert f'{raw_fnv:016x}'==m['raw_fnv64'] and fnv(raw)==m['raw_fnv64']
    names={'sweetgl.dylib','libglapi.0.dylib','georead.dylib'}
    if 'exception_observer' in m:
        observer=m['exception_observer'];names.add('exception_observe.dylib')
        assert observer['target_process']=='AegirPoster' and observer['observations_max']==16
        assert observer['native_original_chain_and_bound_test_passed'] and observer['host_asan_ubsan_passed']
        assert m['observer_alias_e_verified']
        assert observer['source']=='gpu/diagnostics/exception_observe.m'
        if 'renderer_observations_max' in observer:
            assert observer['renderer_observations_max']==32 and observer['renderer_abi_and_device_observer_test_passed']
            assert observer['metadata_only_no_buffer_allocation'] and observer['original_renderer_return_and_exception_preserved']
        if 'runtime_Metal_ABI_methods' in observer:
            assert observer['runtime_Metal_ABI_methods']==22 and observer['metadata_observer_no_device_calls_test_passed']
            sources={'gpu/diagnostics/exception_observe.m','gpu/diagnostics/MetalRuntimeObserve.h'}
            if observer.get('metal_resource_probe'):sources|={'gpu/diagnostics/MetalResourceProbe.h','gpu/metal/MetalResources.h'}
            if observer.get('metal_queue_probe'):
                sources.add('gpu/diagnostics/MetalQueueProbe.h')
                assert observer['queue_probe_submits_actual_GPU_commands'] and observer['metal_resources']['native_blit_command_queue']
            assert set(observer['source_files'])==sources
            assert observer['source_files'][observer['source']]==observer['source_sha256']
    if 'metal_resources' in m:
        names.add('MetalResources.dylib');resources=m['metal_resources']
        assert m['exception_observer']['metal_resource_probe'] and m['exception_observer']['metal_resources']==resources
        assert resources['only_explicit_resource_factory'] and resources['system_Metal_plugin_not_advertised']
        assert m['files']['MetalResources.dylib']['sha256']==resources['library_sha256']
        assert m['library_sha256']==resources['gles_library_sha256']
        assert m['files']['exception_observe.dylib']['sha256']==observer['library_sha256']
    assert set(m['files'])==names
    for name,info in m['files'].items():
        offset=info['raw_offset'];size=info['bytes']
        assert type(offset) is int and type(size) is int and offset>=0 and size>0 and offset+size<=len(raw)
        data=(folder/name).read_bytes()
        assert len(data)==size and sha(data)==info['sha256'] and raw[offset:offset+size]==data
    library=(folder/'sweetgl.dylib').read_bytes()
    assert sha(library)==m['library_sha256'] and len(library)==m['library_bytes'] and fnv(library)==m['library_fnv64']
    assert m['files']['libglapi.0.dylib']['sha256']==API and m['files']['georead.dylib']['sha256']==HELPER
    source=m['frontend_build_binding']['source_files']
    for path,digest in source.items():
        path=Path(path);assert not path.is_absolute() and '..' not in path.parts
        assert re.fullmatch('[0-9a-f]{64}',digest)
    binding={key:m[key] for key in ('library_sha256','library_bytes','library_fnv64','blob_sha256','raw_sha256')}
    binding['manifest_sha256']=sha(manifest_bytes)
    if expected is not None:assert binding==expected,'RAM library binding differs'
    return blob,binding,m
def record_library(record):return record.get('ram_library_binding',BASELINE)
def require_same_library(record,proof):
    assert record_library(record)==proof.get('ram_library_binding',BASELINE),'RAM CPU prerequisite library differs'
