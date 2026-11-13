"""Build a one-shot trial from exact tested baseline inputs, never from a legacy Patch path."""
import argparse,hashlib,json,secrets,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));import adt,mkpayload
from diagnostic_candidate import configuration
from ram_library import load as load_ram
from controller import Artifact,SHIM_SHA
def sha(data):return hashlib.sha256(data).hexdigest()
BASE_HASHES={'private':'b958bd29a457cae61be27a53e1237ccdc4c1247d4cbcff238afbe34d88615162',
             'ready':'2eb112563f920669e2e932799c56b6af03e79e09a1c8e73833855a4ac1f93af1',
             'gpu':'ee284994e2ac7e280f0a3b727ce1ebb4796e3813bbd98b524fa16e6c3d5f9082'}
BASE_KERNEL='da6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a'

def assemble_image(header,shim,payload):
    result=bytearray(header);struct.pack_into('<I',result,16,len(payload))
    result+=shim+b'\0'*(-len(shim)%4096);result+=payload+b'\0'*(-len(payload)%4096)
    return bytes(result)

def prove_payload(record):
    """Bind the entire embedded image to the compiled kernel and exact tested input blobs."""
    mode=record['trial_mode'];version=record['version']
    kernel=ROOT/f'build/surface-diag-v{version}/kernel.v{version}.macho'
    raw_kernel=kernel.read_bytes()
    assert sha(raw_kernel)==record['kernel_sha256'], 'Compiled kernel differs from image record'
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,_,reference,blobs,_,shim=baseline(selected,ROOT/'assets/baselines')
    if mode=='ramcpu':blobs.append(baseline('gpu',ROOT/'assets/baselines')[3][-1])
    if 'ram_library_binding' in record:
        assert mode in ('ramcpu','gpu')
        blob,_,_=load_ram(record['ram_package'],record['ram_library_binding'])
        blobs=[(n,blob if n=='GL-RAM-Z' else data) for n,data in blobs]
    expected_blobs={name:{'bytes':len(data),'sha256':sha(data)} for name,data in blobs}
    assert expected_blobs==record['blobs'], 'Trial firmware/RAM inputs differ from tested baseline'
    payload,_=mkpayload.build(raw_kernel,reference,record['cmdline'],blobs=blobs)
    image=assemble_image(raw[:4096],shim,payload)
    actual=Path(record['image']).read_bytes()
    assert image==actual and sha(image)==record['sha256'], 'Embedded kernel/payload differs from audited inputs'
    return {'image_sha256':sha(image),'kernel_sha256':sha(raw_kernel),'blob_count':len(blobs),
            'complete_payload_reconstructed_from_compiled_kernel':True}
def baseline(name,folder):
    image=folder/({'private':'v724-private.img','ready':'v724-ready.img','gpu':'v724-tested-panel.img'}[name])
    raw=image.read_bytes();assert sha(raw)==BASE_HASHES[name],image
    ks,rs=struct.unpack_from('<I',raw,8)[0],struct.unpack_from('<I',raw,16)[0]
    off=4096+(ks+4095)//4096*4096;payload=raw[off:off+rs]
    assert raw[:8]==b'ANDROID!' and sha(raw[4096:4096+ks])==SHIM_SHA
    assert payload[:8]==b'SWEETXNU' and len(payload)==rs
    vb,entry,ao,an,end,pb,mem=struct.unpack_from('<7Q',payload,16)
    count=struct.unpack_from('<I',payload,12)[0]
    segments=[struct.unpack_from('<4Q',payload,680+i*32) for i in range(count)]
    original_tree=payload[ao:ao+an];tree,parsed=adt.parse(original_tree);assert parsed==an
    chosen=next(c for c in tree.children if c.name=='chosen')
    mmap=next(c for c in chosen.children if c.name=='memory-map')
    names=['GPU-PM4','GPU-PFP','GPU-ZAP-MDT','GPU-ZAP-SEG','GPU-GPMU','ADSP-MDT','ADSP-IMAGE-Z']
    if name=='gpu':names+=['GL-RAM-Z']
    blob_data=[]
    for key in names:
        value=mmap.prop(key);assert value and len(value)==16,key
        pa,bytes=struct.unpack('<2Q',value)
        match=[s for s in segments if s[0]-vb+pb==pa and s[3]==bytes];assert len(match)==1,key
        s=match[0];data=payload[s[2]:s[2]+s[3]];assert len(data)==bytes
        blob_data.append((key,data))
    mmap.props=[v for v in mmap.props if v[0] not in names]
    reference=adt.serialize(tree)
    assert sha(reference)=='257076987411b50f037f70bf97524fae56ac2965e773c1bb8678d05d45480ba6'
    return raw,payload,reference,blob_data,payload[72:680].split(b'\0',1)[0].decode(),raw[4096:4096+ks]
def prepare(mode,kernel,folder,output,version=725,ram_package=None,owner_interval=None):
    config=configuration(version)
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,payload,reference,blobs,cmd,shim=baseline(selected,folder)
    files=list(blobs)
    # Prove extraction covers the complete tested baseline payload, not just the header.
    reproduced,_=mkpayload.build(str(folder/'kernel-v724-tested.macho'),reference,cmd,blobs=files)
    assert reproduced==payload,'Tested baseline reconstruction mismatch'
    token=secrets.token_hex(8);words=cmd.split()
    words=[('sweetloop='+token if w.startswith('sweetloop=') else 'sweetutc='+str(int(time.time())) if w.startswith('sweetutc=') else w) for w in words]
    if mode=='owner':
        words=[w.replace('sweetsample=-','sweetsample=bootdiag').replace('svmro=7','svmro=4') for w in words]
    watchdog=300;observe=240
    if owner_interval is not None:
        assert mode=='owner' and owner_interval==90, 'Only the reviewed owner timing comparison is supported'
        words=[('sweetsample_iv=90' if w.startswith('sweetsample_iv=') else 'sweetar=420' if w.startswith('sweetar=') else w) for w in words]
        watchdog=420;observe=300
    if mode=='ramcpu':
        _,_,_,ramblobs,_,_=baseline('gpu',folder)
        files.append(ramblobs[-1])
    library_binding=None
    if ram_package:
        assert mode in ('ramcpu','gpu')
        blob,library_binding,package=load_ram(ram_package)
        if version>=727:assert 'readonly_probe.c' in package['frontend_build_binding']['source_files']
        files=[(n,blob if n=='GL-RAM-Z' else data) for n,data in files]
        if mode=='gpu' and 'exception_observer' in package:
            # The removed zero-valued scan options have default zero in the
            # audited framebuffer source; TRACE is logging only. Keep cadence.
            words=[w for w in words if w not in ('sweetfb_direct=0','sweetfb_kick=0')]
            words=[w.replace(',SWEETGL_TRACE=1','')+';AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e' if w.startswith('sweetenv=') else w for w in words]
            # svmro7 owns/defer-starts GPU independently of sweetgpuasync.
            # Keep runtime/display/touch settings; make room for explicit tee.
            words=[w for w in words if w not in ('sweetgpuasync=1','-noprogress','keepsyms=1')]
            words.append('sweetnull=backboardd,AegirPoster')
    cmd=' '.join(words);assert len(cmd.encode())<=607
    candidate,info=mkpayload.build(str(kernel),reference,cmd,blobs=files)
    assert b'SWEET SURF CALLER DIAG phase=' in candidate
    if version>=726:
        assert b'SWEET SURF MEMREF ERROR address=' in candidate and b'SWEET SURF MEMREF INIT length=' in candidate
    result=assemble_image(raw[:4096],shim,candidate)
    image=output/'trial.img';assert not image.exists();image.write_bytes(result)
    m={'id':'surface-v'+str(version)+'-'+mode,'version':version,'image':str(image),'sha256':sha(result),
       'kernel_sha256':sha(kernel.read_bytes()),'shim_sha256':SHIM_SHA,'cmdline':cmd,'cmdline_bytes':len(cmd.encode()),
       'token':token,'watchdog_seconds':watchdog,'source_revision':config['binding']['manifest_sha256'],
       'diagnostic_binding':config['binding'],
       'boot_sender':'sony-boot-once','boot_sender_sha256':sha((ROOT/'tools/device/boot_once.py').read_bytes()),
       'return_evidence_sha256':sha((ROOT/'tools/device/return_evidence.py').read_bytes()),
       'trial_mode':mode,'baseline_full_payload_reconstructed':True,
       'blobs':{name:{'bytes':len(data),'sha256':sha(data)} for name,data in files}}
    Artifact(m).validate()
    if library_binding:
        m.update(ram_package=str(Path(ram_package).resolve()),ram_library_binding=library_binding)
        if 'exception_observer' in package:m['exception_observer']=package['exception_observer']
        if 'metal_resources' in package:m['metal_resources']=package['metal_resources']
    if owner_interval is not None:
        m['owner_observation_interval_seconds']=owner_interval
    (output/'artifacts.json').write_text(json.dumps({'target_serial':'BH905ND69G','pon_route':6,'artifacts':[m]},indent=2)+'\n')
    common=['SWEET LOOP LIVE proto=1 token='+token,'SWEET RMI4: F12 page=']
    if mode in ('private','ramcpu'):
        common+=['SWEET VM RO END snap=2 valid=1 .*writes=0 complete=1',
                 'SWEET SMEM STATE END snap=2 mode=2 attempted=9 .*writes=0 unowned_reads=0 complete=1']
        if mode=='ramcpu':
            library=library_binding or {'library_bytes':14675424,'library_fnv64':'bb763317000ee0d7'}
            common+=['SWEET GLRAM: library read=0 bytes='+str(library['library_bytes'])+' fnv='+library['library_fnv64']+' ro=1']
    elif mode=='owner':
        common+=['SWEET SG OWN END snap=1 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1',
                 'SWEET SG OWN END snap=2 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1']
    else:
        common+=['SWEET SCATTER READY END result=0 context=1 identity=1 idle=1 input_bits=36 output_bits=36 native_probe=0']
        if mode=='gpu':
            common+=['sweetgl: full selftest -> PASS','sweetgl: full surface selftest -> PASS',
                     'sweetgl: caller selftest -> PASS','sweetgl: copy refresh selftest -> PASS',
                     'sweetgl: copy context selftest -> PASS',
                     'sweetgl: bridge state selftest -> PASS','sweetgl: program life selftest -> PASS',
                     'SWEET SURF CALLER DIAG phase=.*status=(0x[0-9a-f]+|0)']
            if version>=727:
                assert library_binding,'Read-only trial needs the tested-probe frontend'
                common+=['sweetgl: readonly selftest -> PASS phase=held_sampler']
            if 'exception_observer' in m:
                common+=['SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1',
                         'SWEET EXCEPTION begin call=1 class=.*name=',
                         'SWEET EXCEPTION end call=1 frames=[1-9].*original_chain=1']
                if 'renderer_observations_max' in m['exception_observer']:
                    common+=['SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=[Qq] device_object=1 original_forwarded=1',
                        'SWEET METAL uniform call=1 index=[0-2] device_nil=[01].*metadata_only=1 original_called=1']
                if 'runtime_Metal_ABI_methods' in m['exception_observer']:
                    common+=['SWEET METAL ABI begin class=_MTLDevice present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1',
                        'SWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0']
                if m['exception_observer'].get('metal_resource_probe'):
                    common+=['SWEET METAL RESOURCE END result=0 buffers=3 bytes=33201 span=65536 final_allocated=0 gpu_submit=0']
                if m['exception_observer'].get('metal_queue_probe'):
                    common+=['SWEET METAL QUEUE END result=0 final_allocated=0 copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1']
    plan={'target_serial':'BH905ND69G','pon_route':6,'max_cycles':1,'budget_seconds':480,'camera_required':mode=='gpu',
          'steps':[{'artifact_id':m['id'],'observe_seconds':observe,'stable_seconds':30,'return_seconds':120,'goals':common,
                    'return_evidence':'final-drain','on_pass':'stop','on_failure':'stop'}]}
    (output/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    (output/'package-proof.json').write_text(json.dumps({'baseline':selected,'baseline_sha256':BASE_HASHES[selected],
       'baseline_payload_exact':True,'kernel_sha256':m['kernel_sha256'],'image_sha256':m['sha256'],'cmdline_bytes':len(cmd),
       'blob_count':len(files),'device_operations':0},indent=2)+'\n')
    return m
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--mode',choices=['private','owner','ready','ramcpu','gpu'],required=True)
    p.add_argument('--kernel',required=True);p.add_argument('--baselines',default=str(ROOT/'assets/baselines'));p.add_argument('--output',required=True)
    p.add_argument('--version',type=int,choices=[725,726,727,728,729],default=725)
    p.add_argument('--ram-package')
    p.add_argument('--owner-interval',type=int,choices=[90])
    a=p.parse_args();out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    m=prepare(a.mode,Path(a.kernel),Path(a.baselines),out,a.version,a.ram_package,a.owner_interval)
    print(json.dumps({k:m[k] for k in ('trial_mode','token','kernel_sha256','sha256','cmdline_bytes')}))
