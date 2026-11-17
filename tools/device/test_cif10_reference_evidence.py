"""Reject cross-token, reordered, truncated and changed author reference bytes."""
from cif10_reference_evidence import inspect,authored_input,fnv,BEGIN,END,png_container
import struct,zlib,re

TOKEN='123456789abcdef0'
lines=[f'SWEET LOOP LIVE proto=1 token={TOKEN}','SWEET AIR STATIC HALF END result=0',BEGIN]
for i in range(1,6):
    w,h=(750,1624) if i==5 else (1024,1);top=0 if i==2 else 0xc0000000
    lines.append(f'SWEET CIF10 INPUT case={i} width={w} height={h} bpc=10 bpp=32 pitch={w*4} bitmap=0x42000 top={hex(top)} input_fnv={fnv(authored_input(i))} scale=2 orientation=0')
    lines.append(f'SWEET CIF10 PNG case={i} bytes=100 fnv=0000000000000001 wall_ns=200 cpu_ns=100 cpu_valid=1 returned_nil=0 original_API=1')
    data=bytes((n*13)&255 for n in range(w*h*6));digest=fnv(data)
    lines.append(f'SWEET CIF10 DECODE case={i} width={w} height={h} bpc=16 bpp=48 pitch={w*6} bitmap=0x1000 alpha=0 bytes={len(data)} fnv={digest}')
    lines.append(f'SWEET CIF10 COLOR case={i} model=1 name=kCGColorSpaceSRGB icc_bytes=0 icc_fnv=0000000000000000')
    if i<=4:
        for at in range(0,len(data),48):
            part=data[at:at+48];lines.append(f'SWEET CIF10 DATA case={i} kind=pixels offset={at} bytes={len(part)} hex={part.hex()}')
    lines.append(f'SWEET CIF10 CASE END case={i} result=0 input_unchanged=1 API_forwarded=1 prediction_used=0')
lines.extend((END,'SWEET METAL PROBES END process=backboardd result=0',f'SWEET LOOP ACK FASTBOOT token={TOKEN}'))
text='\n'.join(lines)+'\n';report=inspect(text,TOKEN);assert len(report['cases'])==5 and report['does_not_admit_lossless_optimization']
for broken in (text.replace('DATA case=1 kind=pixels offset=48','DATA case=1 kind=pixels offset=49'),
               text.replace('bytes=48 hex=000d','bytes=48 hex=010d',1),text.replace('bitmap=0x42000','bitmap=0x32000',1),
               text.replace('cpu_valid=1','cpu_valid=0',1),text.replace('API_forwarded=1','API_forwarded=0',1),
               text.replace('hex=000d','hex=\n000d',1),text.replace('top=0x0','top=0xc0000000',1)):
    try:inspect(broken,TOKEN)
    except (AssertionError,KeyError,ValueError):continue
    raise AssertionError('Malformed CIF10 reference was accepted')
try:inspect(text,'fedcba9876543210')
except AssertionError:pass
else:raise AssertionError('Mixed token accepted')
print('CIF10 reference evidence PASS: exact author input/metadata/export FNV, strict ordering/truncation/CPU-state and token rejection')

def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
new=[]
for line in lines:
    if line==BEGIN:new.append('SWEET CIF10 CONFIG revision=2 PNG_exports=1 vImage_modes=2 original_options=1 converter_installed=0')
    if line.startswith('SWEET CIF10 PNG '):
        i=int(line.split('case=')[1].split()[0])
        if i<=4:
            data=bytes((n*13)&255 for n in range(1024*6));network=bytes(data[n^1] for n in range(len(data)))
            png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1024,1,16,2,0,0,0))+chunk(b'IDAT',zlib.compress(b'\0'+network))+chunk(b'IEND',b'')
            line=f'SWEET CIF10 PNG case={i} bytes={len(png)} fnv={fnv(png)} wall_ns=200 cpu_ns=100 cpu_valid=1 returned_nil=0 original_API=1'
    new.append(line)
    if line.startswith('SWEET CIF10 COLOR '):
        i=int(line.split('case=')[1].split()[0]);w,h=(750,1624) if i==5 else (1024,1);data=bytes((n*13)&255 for n in range(w*h*6))
        new.extend((f'SWEET CIF10 VIMAGE case={i} mode=default status=0 bytes={len(data)} fnv={fnv(data)} mismatches=0 guard=1 buffer=1',
                    f'SWEET CIF10 VIMAGE TIME case={i} mode=default wall_ns=50 cpu_ns=40 cpu_valid=1',
                    f'SWEET CIF10 VIMAGE case={i} mode=direct status=-21771 bytes={len(data)} fnv=0000000000000000 mismatches=0 guard=1 buffer=0',
                    f'SWEET CIF10 VIMAGE TIME case={i} mode=direct wall_ns=50 cpu_ns=40 cpu_valid=1'))
        if i<=4:
            for at in range(0,len(png),48):
                part=png[at:at+48];new.append(f'SWEET CIF10 DATA case={i} kind=png offset={at} bytes={len(part)} hex={part.hex()}')
text2='\n'.join(new)+'\n';report=inspect(text2,TOKEN,2)
assert not report['vImage_all_pixel_stages_match'] and report['cases'][0]['vImage']['default']['pixels_match']
assert report['cases'][0]['PNG_container']['IDAT_defiltered_pixels_match_original_CG_decode']
for broken in (text2.replace('mode=default status=0','mode=default status=-1',1),text2.replace('guard=1 buffer=1','guard=0 buffer=1',1),
               text2.replace('kind=png offset=48','kind=png offset=49',1),text2.replace('hex=89504e47','hex=88504e47',1),
               text2.replace('mode=direct wall_ns=50 cpu_ns=40 cpu_valid=1','mode=direct wall_ns=50 cpu_ns=40 cpu_valid=0',1)):
    try:inspect(broken,TOKEN,2)
    except (AssertionError,KeyError,ValueError):continue
    raise AssertionError('Malformed vImage/PNG export accepted')
try:inspect(text2,TOKEN,1)
except AssertionError:pass
else:raise AssertionError('Undeclared revision accepted')
print('CIF10 v2 evidence PASS: conversion/error preservation, PNG CRC/decompression/endian pixel equality and export/revision rejection')

# Independent literal PNG filter vectors for a 2x5 RGB16 raster containing
# network-order bytes 0..59. They cover left/above/Paeth paths, not just None.
filtered=(b'\0'+bytes(range(12))+b'\1'+bytes(range(12,18))+bytes([6])*6+
          b'\2'+bytes([12])*12+b'\3'+bytes([24,25,25,26,26,27])+bytes([9])*6+
          b'\4'+bytes([12])*6+bytes([6])*6)
filters_png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',2,5,16,2,0,0,0))+chunk(b'IDAT',zlib.compress(filtered))+chunk(b'IEND',b'')
expected=bytes(n^1 for n in range(60))
assert png_container(filters_png,expected,dict(width=2,height=5,decoded=dict(bits_per_component=16,bits_per_pixel=48,bitmap_info='0x1000',pitch=12)))['IDAT_defiltered_pixels_match_original_CG_decode']
print('PNG RGB16 filter vectors PASS: all five filters, exact network/host endian bytes')

records3=[]
for line in new:
    if line.startswith('SWEET CIF10 CONFIG '):line='SWEET CIF10 CONFIG revision=3 PNG_exports=1 vImage_modes=2 filter_policies=3 original_options=1 converter_installed=0'
    if line==BEGIN:line=BEGIN.replace('cases=5','cases=6')
    if line==END:
        # A sixth, author-owned smooth fullscreen reference with diagnostic
        # fields only. It deliberately is not a PNG optimizer admission.
        data=bytes((n*13)&255 for n in range(750*1624*6))
        records3.extend((f'SWEET CIF10 INPUT case=6 width=750 height=1624 bpc=10 bpp=32 pitch=3000 bitmap=0x42000 top=0xc0000000 input_fnv={fnv(authored_input(6))} scale=2 orientation=0',
                         'SWEET CIF10 PNG case=6 bytes=100 fnv=0000000000000001 wall_ns=200 cpu_ns=100 cpu_valid=1 returned_nil=0 original_API=1',
                         f'SWEET CIF10 DECODE case=6 width=750 height=1624 bpc=16 bpp=48 pitch=4500 bitmap=0x1000 alpha=0 bytes={len(data)} fnv={fnv(data)}',
                         'SWEET CIF10 COLOR case=6 model=1 name=kCGColorSpaceSRGB icc_bytes=0 icc_fnv=0000000000000000',
                         f'SWEET CIF10 VIMAGE case=6 mode=default status=0 bytes={len(data)} fnv={fnv(data)} differences=0 guard=1 buffer=1',
                         'SWEET CIF10 VIMAGE TIME case=6 mode=default wall_ns=50 cpu_ns=40 cpu_valid=1',
                         f'SWEET CIF10 VIMAGE case=6 mode=direct status=0 bytes={len(data)} fnv=0000000000000001 differences=1 guard=1 buffer=1',
                         'SWEET CIF10 VIMAGE TIME case=6 mode=direct wall_ns=50 cpu_ns=40 cpu_valid=1'))
        for policy in ('baseline','none','up'):
            records3.extend((f'SWEET CIF10 FILTER case=6 policy={policy} result=0 bytes=100 fnv=0000000000000001 pixels_equal=1 ICC_equal=1 metadata_equal=1 PNG_equal=1',
                             f'SWEET CIF10 FILTER TIME case=6 policy={policy} wall_ns=50 cpu_ns=40 cpu_valid=1'))
        records3.append('SWEET CIF10 CASE END case=6 result=0 input_unchanged=1 API_forwarded=1 prediction_used=0')
        line=END.replace('cases=5','cases=6')
    records3.append(line.replace(' mismatches=',' differences='))
    if line.startswith('SWEET CIF10 COLOR '):
        i=int(line.split('case=')[1].split()[0]);small=i<=4
        for policy in ('baseline','none','up'):
            size=len(png) if small else 100;digest=fnv(png) if small else '0000000000000001'
            records3.extend((f'SWEET CIF10 FILTER case={i} policy={policy} result=0 bytes={size} fnv={digest} pixels_equal=1 ICC_equal=1 metadata_equal=1 PNG_equal=1',
                             f'SWEET CIF10 FILTER TIME case={i} policy={policy} wall_ns=50 cpu_ns=40 cpu_valid=1'))
            if i==1:
                for at in range(0,len(png),48):
                    part=png[at:at+48];records3.append(f'SWEET CIF10 DATA case=1 kind={policy}_png offset={at} bytes={len(part)} hex={part.hex()}')
text3='\n'.join(records3)+'\n';assert not re.search(r'mismatches=[1-9]',text3)
report=inspect(text3,TOKEN,3);assert len(report['cases'])==6 and all(report['filter_policies_match_original'].values())
assert not report['vImage_all_pixel_stages_match'] and report['does_not_admit_lossless_optimization']
for bad in (text3.replace(' differences=1 ',' mismatches=1 ',1),text3.replace('policy=none result=0','policy=none result=1',1),
            text3.replace('kind=none_png offset=48','kind=none_png offset=49',1)):
    try:inspect(bad,TOKEN,3)
    except (AssertionError,KeyError,ValueError):continue
    raise AssertionError('Malformed filter candidate or ambiguous telemetry accepted')
print('CIF10 v3 evidence PASS: six references/filter pixel/profile/metadata scope, ambiguous telemetry and altered exports rejected')

source='a'*64
bound=text3.replace(BEGIN.replace('cases=5','cases=6'),f'SWEET PNG ENCODER REFERENCE source_sha={source} backend=ImageIO_UP installed=0\n'+BEGIN.replace('cases=5','cases=6'))
assert inspect(bound,TOKEN,3,source)['encoder_source_sha256']==source
for expected in (None,'b'*64):
    try:inspect(bound,TOKEN,3,expected)
    except AssertionError:continue
    raise AssertionError('Unbound shared encoder reference accepted')
print('Shared PNG encoder reference PASS: exact native source identity required')
