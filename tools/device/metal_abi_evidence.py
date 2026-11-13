"""Validate complete current-boot Metal runtime metadata; never claim a driver."""
import re

SELECTORS=[('device_base',s) for s in ('init','initWithAcceleratorPort:','initLimits','initFeatureQueries','featureProfile')]+[
    ('device','newBufferWithLength:options:'),('device','newBufferWithBytes:length:options:'),('device','newCommandQueue'),
    ('device','newLibraryWithData:error:'),('device','newTextureWithDescriptor:'),
    ('device','newRenderPipelineStateWithDescriptor:error:'),('device','newComputePipelineStateWithFunction:error:'),
    ('buffer','contents'),('buffer','length'),('buffer','gpuAddress'),('queue','commandBuffer'),
    ('queue','commandBufferWithUnretainedReferences'),('commands','commit'),('commands','waitUntilCompleted'),
    ('commands','addCompletedHandler:'),('commands','status'),('commands','error')]

def verify(text,token):
    start=text.index('SWEET LOOP LIVE proto=1 token='+token);stop=text.index('SWEET LOOP ACK FASTBOOT token='+token)
    scope=text[start:stop];begins=list(re.finditer(r'SWEET METAL ABI begin class=_MTLDevice present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1 device_protocol=1 buffer_protocol=1 queue_protocol=1 command_protocol=1 metadata_only=1',scope))
    assert begins,'Actual target Metal base/limits/protocols absent'
    records=[];incomplete=0
    for i,begin in enumerate(begins):
        block=scope[begin.end():begins[i+1].start() if i+1<len(begins) else len(scope)]
        end=re.search(r'SWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0',block)
        if not end:incomplete+=1;continue
        methods=[]
        for m in re.finditer(r'SWEET METAL ABI method index=(\d+) owner=(\S+) selector=(\S+) present=([01]) types_hex=([0-9a-f]+|none) metadata_only=1',block[:end.start()]):
            index=int(m[1]);assert index<len(SELECTORS) and (m[2],m[3])==SELECTORS[index]
            if m[4]=='1':
                assert m[5]!='none' and len(m[5])<=256
                types=bytes.fromhex(m[5]).decode('ascii');assert types and '\x00' not in types
            else:assert m[5]=='none';types=None
            methods.append(dict(index=index,owner=m[2],selector=m[3],types=types))
        if len(methods)<22:incomplete+=1;continue
        assert [m['index'] for m in methods]==list(range(22))
        assert re.fullmatch(r'@(?:20|24)@0:8I16',methods[1]['types'] or ''),'Metal accelerator initializer differs from uint32 ABI'
        assert methods[0]['types']=='@16@0:8' and methods[2]['types']=='v16@0:8'
        assert methods[3]['types']=='v16@0:8' and methods[4]['types'] in (None,'Q16@0:8')
        # featureProfile belongs to the native SPI, but the real iOS16
        # abstract base has no implementation. Keep that observed absence.
        assert all(m['types'] for m in methods if m['selector'] not in ('gpuAddress','featureProfile'))
        records.append(methods)
    assert records,'No complete actual Metal ABI sample'
    assert all(record==records[0] for record in records),'Metal ABI changed between actual processes'
    return dict(actual_Metal_base_and_protocols_verified=True,limits_bytes=264,limits_alignment=8,
        initializer_uses_uint32_accelerator_port=True,methods=records[0],complete_samples=len(records),incomplete_samples=incomplete,
        base_featureProfile_implementation_missing=records[0][4]['types'] is None,
        metadata_only=True,Metal_instances_created=0,GPU_operations=0,device_driver_and_rendering_not_yet_provided=True)
