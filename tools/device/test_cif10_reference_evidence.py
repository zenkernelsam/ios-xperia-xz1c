"""Reject cross-token, reordered, truncated and changed author reference bytes."""
from cif10_reference_evidence import inspect,authored_input,fnv,BEGIN,END,png_container
import struct,zlib

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
