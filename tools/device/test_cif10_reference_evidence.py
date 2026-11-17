"""Reject cross-token, reordered, truncated and changed author reference bytes."""
from cif10_reference_evidence import inspect,authored_input,fnv,BEGIN,END

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
