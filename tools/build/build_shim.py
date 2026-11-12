"""Compile the canonical Lilac shim into a fresh build folder; never boot/flash or clean."""
import argparse,gzip,hashlib,json,os,re,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--dtb');p.add_argument('--output');a=p.parse_args()
out=Path(a.output).absolute() if a.output else root/'build'/('shim-'+time.strftime('%Y%m%d-%H%M%S'))
assert out.is_relative_to(root/'build') and not out.exists(),'Use a fresh directory under build/'
out.mkdir(parents=True)
source=root/'shim';flags=['-O2','-ffreestanding','-nostdlib','-fno-pic','-mgeneral-regs-only','-mstrict-align','-fno-stack-protector','-Wall','-Wextra']
files=['start.S','shim.c','console.c','mmu.c','fdt.c','loader.c','ufs.c','wdt.c','spmi.c','gpu.c']
objects=[]
for name in files:
    target=out/(Path(name).stem+'.o')
    subprocess.run(['aarch64-linux-gnu-gcc',*flags,'-c',str(source/name),'-o',str(target)],check=True)
    objects.append(target)
relocations=subprocess.check_output(['aarch64-linux-gnu-readelf','-r',*map(str,objects)],text=True)
# The Linux Image header stores image_size as _end at .text.head+0x10.
# It is a link-time size, not a runtime pointer. No other ABS64 is permitted.
for target in objects:
    rows=subprocess.check_output(['aarch64-linux-gnu-readelf','-r',str(target)],text=True)
    absolute=[line for line in rows.splitlines() if 'ABS64' in line]
    if target.stem=='start':
        assert len(absolute)==1 and re.search(r'^0+10\s+.*ABS64\s+0+\s+_end \+ 0$',absolute[0]),absolute
    else:assert not absolute,target
subprocess.run(['aarch64-linux-gnu-gcc',*flags,'-static','-T',str(source/'shim.ld'),'-Wl,--build-id=none','-Wl,-N',*map(str,objects),'-o',str(out/'shim.elf')],check=True)
subprocess.run(['aarch64-linux-gnu-objcopy','-O','binary',str(out/'shim.elf'),str(out/'Image')],check=True)
assert 'R_AARCH64_' not in subprocess.check_output(['aarch64-linux-gnu-readelf','-r',str(out/'shim.elf')],text=True)
symbols=subprocess.check_output(['aarch64-linux-gnu-nm',str(out/'shim.elf')],text=True)
match=re.search(r'^([0-9a-f]+)\s+\w\s+_end$',symbols,re.M);assert match
image=out/'Image';raw=image.read_bytes();size=int(match[1],16);assert len(raw)<=size<16<<20
image.write_bytes(raw+b'\0'*(size-len(raw)))
compressed=gzip.compress(image.read_bytes(),compresslevel=9,mtime=0)
(out/'Image.gz').write_bytes(compressed)
if a.dtb:
    dtb=Path(a.dtb).read_bytes();assert dtb[:4]==b'\xd0\x0d\xfe\xed'
    (out/'Image.gz-dtb').write_bytes(compressed+dtb)
manifest={'source_files':{n:hashlib.sha256((source/n).read_bytes()).hexdigest() for n in files+['shim.ld']},
          'compiler':subprocess.check_output(['aarch64-linux-gnu-gcc','--version'],text=True).splitlines()[0],
          'image_bytes':size,'image_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),
          'runtime_absolute_pointer_relocations':0,'linux_header_size_relocation':1,'no_device_operations':True,
          'rebuilt_binary_not_device_tested':True}
(out/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'output':str(out),**manifest},indent=2))
