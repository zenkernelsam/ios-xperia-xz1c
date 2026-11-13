"""Make a new bounded read-only HFS RAM-library image from the canonical build outputs."""
import argparse,hashlib,json,os,shutil,struct,subprocess,time,zlib
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output');p.add_argument('--frontend');a=p.parse_args()
out=Path(a.output).absolute() if a.output else root/'build'/('glram-'+time.strftime('%Y%m%d-%H%M%S'))
assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
frontend=Path(a.frontend).absolute() if a.frontend else root/'build/sweetgl'
assert frontend.is_relative_to(root/'build')
inputs={'sweetgl.dylib':frontend/'sweetgl.dylib','libglapi.0.dylib':frontend/'libglapi.0.dylib',
        'georead.dylib':root/'build/geocompat/georead.dylib'}
def run(args):subprocess.run(list(map(str,args)),check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for name,path in inputs.items():
    assert path.is_file();shutil.copy2(path,out/name)
run(['install_name_tool','-id','/dev/sglram/sweetgl.dylib','-change','/usr/lib/libglapi.0.dylib','/dev/sglram/libglapi.0.dylib',out/'sweetgl.dylib'])
run(['install_name_tool','-id','/dev/sglram/libglapi.0.dylib',out/'libglapi.0.dylib'])
for name in inputs:run(['codesign','-f','-s','-',out/name]);run(['codesign','--verify',out/name])
image=out/'glram.dmg';mount=out/'mount';mount.mkdir()
run(['hdiutil','create','-size','32m','-fs','HFS+','-volname','SGLRAM','-layout','NONE',image])
run(['hdiutil','attach','-nobrowse','-owners','on','-mountpoint',mount,image])
try:
    for name in inputs:shutil.copy2(out/name,mount/name)
    os.link(mount/'sweetgl.dylib',mount/'m');os.link(mount/'georead.dylib',mount/'g')
    run(['sync'])
finally:run(['hdiutil','detach',mount])
raw=image.read_bytes()[:32<<20];assert len(raw)==32<<20 and raw[1024:1026]==b'H+'
(out/'glram.raw').write_bytes(raw)
run(['hdiutil','attach','-imagekey','diskimage-class=CRawDiskImage','-readonly','-nobrowse','-owners','on','-mountpoint',mount,out/'glram.raw'])
try:
    assert os.statvfs(mount).f_flag&os.ST_RDONLY
    for name in inputs:assert sha(mount/name)==sha(out/name)
    assert (mount/'m').stat().st_ino==(mount/'sweetgl.dylib').stat().st_ino
    assert (mount/'g').stat().st_ino==(mount/'georead.dylib').stat().st_ino
finally:run(['hdiutil','detach',mount])
def fnv(data):
    n=14695981039346656037
    for byte in data:n=((n^byte)*1099511628211)&0xffffffffffffffff
    return n
packed=zlib.compress(raw,6);blob=struct.pack('<8sIIQQQ',b'SGLZ0001',1,40,len(raw),len(packed),fnv(raw))+packed
assert len(blob)<=8<<20;(out/'glram.zblob').write_bytes(blob)
manifest={'raw_bytes':len(raw),'raw_sha256':sha(out/'glram.raw'),'blob_bytes':len(blob),'blob_sha256':sha(out/'glram.zblob'),
          'raw_fnv64':f'{fnv(raw):016x}','library_sha256':sha(out/'sweetgl.dylib'),'library_bytes':(out/'sweetgl.dylib').stat().st_size,
          'library_fnv64':f'{fnv((out/"sweetgl.dylib").read_bytes()):016x}','helper_sha256':sha(out/'georead.dylib'),
          'read_only_verified':True,'aliases_m_g_verified':True,'new_package_device_tested':False,'no_device_operations':True}
manifest['api_sha256']=sha(out/'libglapi.0.dylib')
manifest['files']={name:{'bytes':(out/name).stat().st_size,'sha256':sha(out/name),'raw_offset':raw.find((out/name).read_bytes())} for name in inputs}
assert all(row['raw_offset']>=0 for row in manifest['files'].values()), 'New HFS image must expose exact contiguous file bytes for independent audit'
binding=frontend/'build-binding.json'
if binding.is_file():
    manifest['frontend_build_binding']=json.loads(binding.read_bytes())
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'output':str(out),**manifest},indent=2))
