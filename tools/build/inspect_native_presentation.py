"""Read exact original iOS16 presentation ABI and call anchors, without a phone.

Only original method metadata, source identities and hashes enter the report;
all Apple byte ranges and disassembly stay in the local build output.
"""
import argparse,hashlib,json,plistlib,re,shutil,struct,subprocess,sys
from pathlib import Path
from a2s_lookup import A2S
from inspect_poster_routing import A2S_SHA,CACHE_SHA,macho_string
ROOT=Path(__file__).resolve().parents[2]
TOOL_SHA='e90ac1e07e8dd74f533513e61088afc27c4e995c82902be80aab9618cb868c47'
IMAGES={'QuartzCore':('b136667d82490fcd9868ec96941b3397d61b03bbe58efce23ac8ec0df6ce7d0c','9e40b2eb260c3af1ae45a1a551a8c1b9'),
        'Metal':('4dabe5cbbe35f94fcf680ae682d18a278a037b744bbdb5a1273342c0a1c420dc','0f0b7b9690bf3a07b7e185e2fb6160e6')}
ANCHORS={
 'allocate-texture':(0x1884bc2cc,340,'ebe38249fdb0c40ec606eee530e7f4796d1c27b0632da3c5e2122deabe81c679'),
 'queue-present':(0x181a29fd4,384,'d8f9cc24a79c492a254042bd8ecbe748de1d8711430644fae73f502d1f3588d5'),
 'queue-present-block':(0x181a31874,120,'f58d095056656885e4ccdead210d393f41eed3107d75af2cb85ec8ba22dfe0f7'),
 'queue-options':(0x181b20db4,376,'09fb32c39f68297789f5e4ca8bd218e61bc8493c56d166f8a6491d2c894b5e12'),
 'queue-options-block':(0x181b20f88,104,'b4c301cdc1f516d43e36af0b891a649669fb4a468e1fdfb9ce30001f43396923'),
 'drawable-options':(0x1884bc174,176,'6f97daced3c3f9d663371de9d6709ec399ee9d71b0f2fc4e843e2d701b38a78c'),
 'canvas-present-commit':(0x1c1737740,32,'49a913f1c7f4324fe3d5f7827019913d7a938e7fc45d7611a191f05a20efd747'),
 'canvas-transaction-wait':(0x1c17379d4,68,'909ae4e274d19108156b120b20c18c311d4a6214398235e019570fd1e845d7f4'),
}
METHODS={
 'CAMetalDrawable':('QuartzCore',0x188558908,38,{'texture':'@16@0:8','present':'v16@0:8','presentAtTime:':'v24@0:8d16',
     'presentAfterMinimumDuration:':'v24@0:8d16','presentWithOptions:':'v24@0:8@16','addPresentScheduledHandler:':'v24@0:8@?16',
     'addPresentedHandler:':'v24@0:8@?16','presentedTime':'d16@0:8','newTextureWithResourceIndex:':'@24@0:8Q16'}),
 'MTLTextureDescriptorInternal':('Metal',0x181b3a6d8,66,{'isDrawable':'B16@0:8','framebufferOnly':'B16@0:8','rotation':'Q16@0:8',
     'protectionOptions':'Q16@0:8','setIsDrawable:':'v20@0:8B16','setFramebufferOnly:':'v20@0:8B16','swizzle':'{?=CCCC}16@0:8'}),
 '_MTLCommandBuffer':('Metal',0x181b42b90,81,{'presentDrawable:':'v24@0:8@16','presentDrawable:atTime:':'v32@0:8@16d24',
     'presentDrawable:afterMinimumDuration:':'v32@0:8@16d24','presentDrawable:options:':'v32@0:8@16@24'})
}
def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda:f.read(8<<20),b''):h.update(chunk)
    return h.hexdigest()
def vm_bytes(data,address,length):
    assert data[:4]==b'\xcf\xfa\xed\xfe';at=32
    for _ in range(struct.unpack_from('<I',data,16)[0]):
        command,size=struct.unpack_from('<II',data,at);assert size>=8 and at+size<=len(data)
        if command==0x19:
            _,vm,_,offset,span=struct.unpack_from('<16sQQQQ',data,at+8)
            if vm<=address and address+length<=vm+span:
                assert offset+span<=len(data);return data[offset+address-vm:offset+address-vm+length]
        at+=size
    raise ValueError('Original ABI range outside image')
def uuid(data):
    at=32;value=None
    for _ in range(struct.unpack_from('<I',data,16)[0]):
        command,size=struct.unpack_from('<II',data,at)
        if command==0x1b:assert value is None;value=data[at+8:at+24].hex()
        at+=size
    assert value;return value
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--symbols',type=Path,required=True)
    p.add_argument('--ipsw',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    assert sys.platform=='darwin';mounted=args.root.resolve(strict=True);symbols=args.symbols.resolve(strict=True);tool=args.ipsw.resolve(strict=True)
    cache=mounted/'System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64'
    assert sha(cache)==CACHE_SHA and sha(symbols)==A2S_SHA and sha(tool)==TOOL_SHA
    volume=plistlib.loads(subprocess.run(['diskutil','info','-plist',str(mounted)],capture_output=True,check=True,timeout=10).stdout)
    assert volume['MountPoint']==str(mounted) and not volume['WritableVolume'] and not volume['WritableMedia']
    out=args.output.resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    shutil.copyfile(symbols,out/'symbols.a2s')
    def run(name,arguments):
        result=subprocess.run([str(tool),*arguments,'--no-color'],capture_output=True,timeout=55);data=result.stdout+result.stderr
        assert len(data)<8<<20;(out/(name+'.log')).open('xb').write(data);assert result.returncode==0,data[-2500:].decode(errors='replace');return result.stdout.decode(errors='replace')
    anchors={};texts={}
    for name,(address,size,wanted) in ANCHORS.items():
        binary=out/(name+'.bin');run(name+'-dump',['dyld','dump',str(cache),hex(address),'--size',str(size),'--output',str(binary)])
        assert binary.stat().st_size==size and sha(binary)==wanted,name
        text=run(name,['dyld','disass',str(cache),'--vaddr',hex(address),'--count',str(size//4),'--cache',str(out/'symbols.a2s')]);texts[name]=text
        anchors[name]=dict(address=hex(address),bytes=size,sha256=wanted)
    allocation=texts['allocate-texture']
    for selector in ('setRotation:','setIsDrawable:','setFramebufferOnly:','setUsage:','newTextureWithDescriptor:iosurface:plane:'):
        assert '_objc_msgSend$'+selector in allocation,selector
    assert '_objc_msgSend$addScheduledHandler:' in texts['queue-present'] and '_objc_msgSend$present' in texts['queue-present-block']
    assert '_objc_msgSend$copy' in texts['queue-options'] and '_objc_msgSend$presentWithOptions:' in texts['queue-options-block']
    assert texts['canvas-present-commit'].index('_objc_msgSend$presentDrawable:')<texts['canvas-present-commit'].index('_objc_msgSend$commit')
    assert texts['canvas-transaction-wait'].index('_objc_msgSend$waitUntilScheduled')<texts['canvas-transaction-wait'].index('_objc_msgSend$present')
    images={}
    for image,(wanted,expected_uuid) in IMAGES.items():
        run(image+'-extract',['dyld','macho',str(cache),image,'--extract','--output',str(out/(image+'-extracted'))])
        path=out/(image+'-extracted')/image;assert sha(path)==wanted;data=path.read_bytes();assert uuid(data)==expected_uuid;images[image]=data
    mapping=A2S(symbols);methods=[]
    try:
        for owner,(image,address,count,wanted) in METHODS.items():
            table=out/(owner+'-methods.bin');run(owner+'-methods',['dyld','dump',str(cache),hex(address),'--size',str(8+count*12),'--output',str(table)])
            data=table.read_bytes();assert struct.unpack_from('<II',data)==(0xc000000f,count);seen=set()
            for index in range(count):
                entry=address+8+index*12;_,type_delta,imp_delta=struct.unpack_from('<iii',data,8+index*12);imp=entry+8+imp_delta
                info=mapping.lookup(imp)
                if not info or info['symbol_offset']!='0x0' or not info['symbol'].startswith('-['+owner+' '):continue
                selector=info['symbol'].split(' ',1)[1][:-1]
                if selector not in wanted:continue
                types=macho_string(images[image],entry+4+type_delta);assert types==wanted[selector] and selector not in seen;seen.add(selector)
                methods.append(dict(owner=owner,selector=selector,types=types,IMP=hex(imp),exact_A2S_IMP_start=True))
            assert seen==set(wanted)
    finally:mapping.close()
    mode=struct.unpack('<4Q',vm_bytes(images['QuartzCore'],0x1d5b5ec00,32));time=struct.unpack('<4Q',vm_bytes(images['QuartzCore'],0x1d5b5ec20,32))
    assert macho_string(images['QuartzCore'],mode[2])=='presentationMode' and macho_string(images['QuartzCore'],time[2])=='presentTimeInterval'
    assert sha(cache)==CACHE_SHA and sha(symbols)==A2S_SHA
    report=dict(system='iOS16.7.16/20H392',readonly_root_verified=True,cache_header_sha256=CACHE_SHA,A2S_sha256=A2S_SHA,ipsw_tool_sha256=TOOL_SHA,
        original_images={name:dict(sha256=values[0],uuid=values[1]) for name,values in IMAGES.items()},original_methods=methods,exact_call_anchors=anchors,
        CAMetalLayer_sets_original_drawable_and_framebuffer_flags=True,original_drawable_uses_original_IOSurface_texture=True,
        original_command_present_scheduled_handler_calls_native_present=True,original_options_are_copied_and_forwarded=True,
        original_ClockKitUI_has_presentDrawable_then_commit_and_transaction_waitUntilScheduled_then_present_paths=True,
        original_option_keys=['presentationMode','presentTimeInterval'],original_option_modes={'0':'present','1':'presentAtTime','2':'presentAfterMinimumDuration'},
        existing_backend_Scheduled_must_not_release_inputs_before_real_operations=True,static_metadata_is_not_runtime_or_optical_evidence=True,
        Apple_bytes_disassembly_not_for_Git=True,device_operations=0,tool_source_sha256=sha(Path(__file__)))
    (out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
