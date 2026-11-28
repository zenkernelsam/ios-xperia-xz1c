"""Build the real readonly MCM reader; keep fixtures for native-bin recycling."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/device'))
from poster_container_evidence import inspect_scope
META='.com.apple.mobile_container_manager.metadata.plist'


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--fixtures',required=True)
    args=parser.parse_args();out=Path(args.output).resolve();fixtures=Path(args.fixtures).resolve()
    if os.name=='nt':
        def linux(p):return '/mnt/'+p.drive[0].lower()+p.as_posix()[2:]
        command=['wsl','-d','Ubuntu-22.04','--exec','python3','-B',linux(Path(__file__).resolve()),
                 '--output',linux(out),'--fixtures',linux(fixtures)]
        r=subprocess.run(command,capture_output=True);sys.stdout.buffer.write(r.stdout);sys.stderr.buffer.write(r.stderr)
        return r.returncode
    assert out.is_relative_to(ROOT/'build') and not out.exists() and not fixtures.exists()
    out.mkdir(parents=True);fixtures.mkdir(parents=True)
    sources=['gpu/diagnostics/PosterContainerSnapshot.h','gpu/diagnostics/PosterConfigSnapshot.h',
             'gpu/sweetgl/ProgramCacheCodec.h','gpu/sweetgl/MetalShaderAPI.h',
             'tools/build/test_poster_container_snapshot.c','tools/build/check_poster_container_snapshot.py',
             'tools/device/poster_container_evidence.py']
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    before={p:sha(ROOT/p) for p in sources}
    def run(cmd,name):
        r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=30)
        (out/(name+'.log')).write_bytes(r.stdout+r.stderr)
        assert r.returncode==0,(r.stdout+r.stderr).decode('utf8','replace')[-4000:]
        return r.stdout.decode('utf8')
    compiler=['clang'] if sys.platform=='darwin' else ['gcc']
    link=[] if sys.platform=='darwin' else ['-lcrypto']
    run(compiler+['-O1','-g','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations',
        '-fsanitize=address,undefined','-fno-sanitize-recover=all',ROOT/'tools/build/test_poster_container_snapshot.c',
        '-o',out/'test-reader']+link,'compile')
    def roots(name):
        base=fixtures/name;base.mkdir();r=[base/str(i) for i in range(4)]
        for p in r:p.mkdir()
        return r
    def put(root,name,identifier='com.apple.PosterBoard',size=None):
        p=root/name;p.mkdir()
        data=plistlib.dumps({'MCMMetadataIdentifier':identifier,'MCMMetadataUUID':name},fmt=plistlib.FMT_BINARY)
        (p/META).write_bytes(data if size is None else data+bytes(size-len(data)))
        return p
    def capture(name,r,fault=0):
        snapshot={str(p):sha(p) for base in r if base.exists() for p in base.rglob('*') if not p.is_symlink() and p.is_file()}
        text=run([out/'test-reader',fault,*r],name)
        assert snapshot=={p:sha(Path(p)) for p in snapshot}
        result=inspect_scope(text,list(map(str,r)))
        (out/(name+'.json')).write_bytes((json.dumps(result,indent=2)+'\n').encode())
        return text,result
    normal=roots('normal')
    target=put(normal[0],'uuid-poster')
    put(normal[0],'uuid-other','com.apple.Preferences')
    put(normal[3],'systemgroup.com.apple.posterboard','systemgroup.com.apple.posterboard')
    (target/'configuration.plist').write_bytes(plistlib.dumps({'selected':'must-not-read'}))
    (target/'opaque.png').write_bytes(b'not-an-image-read')
    (normal[1]/'missing-metadata').mkdir()
    (normal[1]/'outside-link').symlink_to(target,target_is_directory=True)
    text,a=capture('normal',normal)
    assert a['summary']['result']==0 and a['summary']['files']==3 and not a['summary']['errors']
    assert a['exact_PosterBoard_identity_unique'] and a['exact_PosterBoard_containers'][0]['container_path']==str(target)
    assert all(p['metadata_path'].endswith('/'+META) for p in a['containers'])
    _,a=capture('EINTR',normal,1);assert a['summary']['files']==3 and a['summary']['errors']==0
    _,a=capture('read-EIO',normal,2);assert a['summary']['files']==0 and a['summary']['errors']==3
    _,a=capture('FIFO-open-race',normal,3);assert a['summary']['files']==0 and a['summary']['errors']==3
    linked=roots('linked-metadata');linked_target=put(linked[0],'poster')
    (linked[1]/'linked').mkdir();(linked[1]/'linked'/META).symlink_to(linked_target/META)
    _,a=capture('linked-metadata',linked)
    assert a['summary']['files']==1 and a['summary']['errors']==1 and not a['exact_PosterBoard_identity_unique']
    signature=roots('bad-signature');(signature[0]/'unknown').mkdir()
    (signature[0]/'unknown'/META).write_bytes(b'unknown format')
    (signature[1]/'json').mkdir();(signature[1]/'json'/META).write_bytes(b'{"MCMMetadataIdentifier":"com.apple.PosterBoard"}')
    _,a=capture('bad-signature',signature)
    assert a['summary']['files']==0 and a['summary']['errors']==2 and not a['catalog_enumeration_complete']
    oversized=roots('oversized');put(oversized[0],'too-big',size=65537)
    _,a=capture('oversized',oversized);assert a['summary']['files']==0 and a['summary']['errors']==1
    count=roots('file-limit')
    for i in range(129):put(count[0],'uuid-%03d'%i,'com.apple.Other' if i else 'com.apple.PosterBoard')
    _,a=capture('file-limit',count)
    assert a['summary']['result']<0 and a['summary']['files']==128 and not a['exact_PosterBoard_identity_unique']
    assert not a['catalog_enumeration_complete']
    entries=roots('entry-limit')
    for i in range(257):(entries[0]/('entry-%03d'%i)).mkdir()
    _,a=capture('entry-limit',entries);assert a['summary']['result']<0 and a['summary']['examined']==256
    newline=roots('newline-path');put(newline[0],'uuid-\n-safe')
    _,a=capture('newline-path',newline);assert a['exact_PosterBoard_identity_unique']
    duplicate=roots('duplicate-identity');put(duplicate[0],'one');put(duplicate[2],'two')
    _,a=capture('duplicate-identity',duplicate);assert len(a['exact_PosterBoard_containers'])==2 and not a['exact_PosterBoard_identity_unique']
    missing=roots('missing-root');put(missing[0],'poster');missing[2]=missing[2].parent/'absent'
    _,a=capture('missing-root',missing);assert a['summary']['opened']==3 and a['summary']['errors']==0
    budget=roots('byte-limit')
    for i in range(7):
        p=budget[0]/('uuid-%d'%i);p.mkdir()
        (p/META).write_bytes(plistlib.dumps({'MCMMetadataIdentifier':'com.apple.Other','padding':bytes(40000)},fmt=plistlib.FMT_BINARY))
    _,a=capture('byte-limit',budget)
    assert a['summary']['result']<0 and a['summary']['files']==6 and a['summary']['bytes']<=262144
    rejected={}
    data=re.search(r'SWEET PCON DATA[^\n]*',text)[0]
    corruptions=dict(missing_data=text.replace(data+'\n','',1),duplicate_data=text.replace(data,data+'\n'+data,1),
        changed_data=text.replace(data,data[:-1]+('0' if data[-1]!='0' else '1'),1),
        missing_END=re.sub(r'SWEET PCON END[^\n]*','',text),
        wrong_count=text.replace('examined=7','examined=8'),
        wrong_root=text.replace('ROOT index=0','ROOT index=2',1),
        unknown_record=text+'SWEET PCON UNKNOWN\n',
        late_record=text+data+'\n')
    # Alter the actual END count rather than relying on a fixture directory order.
    corruptions['wrong_count']=re.sub(r'(END result=0 roots=4 opened=4 examined=)\d+',r'\g<1>999',text)
    for name,value in corruptions.items():
        try:inspect_scope(value,list(map(str,normal)))
        except ValueError as error:rejected[name]=str(error)
        else:raise AssertionError('Accepted corruption '+name)
    assert before=={p:sha(ROOT/p) for p in sources}
    report=dict(source_files=before,host_ASan_UBSan_passed=True,actual_filesystem_cases=13,
        corruption_rejections=rejected,metadata_only_readonly_nofollow_FIFO_and_limits_checked=True,
        original_configuration_files_and_images_not_captured=True,fixtures=str(fixtures),
        device_operations=0,ARM64_compilation_not_done=True,observer_integration_not_done=True,
        current_phone_container_not_captured=True,selected_wallpaper_not_inferred=True,permanent_deletions=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report,indent=2));return 0


if __name__=='__main__':sys.exit(main())
