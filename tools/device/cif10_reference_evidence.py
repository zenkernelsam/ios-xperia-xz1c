"""Read a current-token, author-owned PNG reference; never admit an optimizer."""
import hashlib
import re
import struct

BEGIN='SWEET CIF10 REFERENCE BEGIN cases=5 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0'
END='SWEET CIF10 REFERENCE END result=0 cases=5 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0'
INPUT=re.compile(r'SWEET CIF10 INPUT case=(\d+) width=(\d+) height=(\d+) bpc=10 bpp=32 pitch=(\d+) bitmap=(0x[0-9a-f]+) top=(0x[0-9a-f]+) input_fnv=([0-9a-f]{16}) scale=2 orientation=0')
PNG=re.compile(r'SWEET CIF10 PNG case=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) returned_nil=0 original_API=1')
DECODE=re.compile(r'SWEET CIF10 DECODE case=(\d+) width=(\d+) height=(\d+) bpc=(\d+) bpp=(\d+) pitch=(\d+) bitmap=(0x[0-9a-f]+) alpha=(\d+) bytes=(\d+) fnv=([0-9a-f]{16})')
COLOR=re.compile(r'SWEET CIF10 COLOR case=(\d+) model=(-?\d+) name=(\S+) icc_bytes=(\d+) icc_fnv=([0-9a-f]{16})')
DATA=re.compile(r'SWEET CIF10 DATA case=(\d+) kind=(pixels|icc) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
CASE_END=re.compile(r'SWEET CIF10 CASE END case=(\d+) result=0 input_unchanged=1 API_forwarded=1 prediction_used=0')


def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'


def authored_input(index):
    width,height=(750,1624) if index==5 else (1024,1)
    data=bytearray(width*height*4)
    top=0 if index==2 else 0xc0000000
    for y in range(height):
        for x in range(width):
            i=y*width+x
            if index<=2:r=g=b=i&1023
            elif index==3:r,g,b=i&1023,384,384
            elif index==4:r,g,b=i&1023,(i*17)&1023,(i*73)&1023
            else:r,g,b=(x*17+y*3)&1023,(x*7+y*29)&1023,(x*31+y*11)&1023
            struct.pack_into('<I',data,i*4,top|(r<<20)|(g<<10)|b)
    return data


def parse(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    start,finish=text.index(live),text.index(ack);assert start<finish
    scope=text[start:finish]
    assert scope.count(BEGIN)==scope.count(END)==1
    begin,end=scope.index(BEGIN),scope.index(END);assert begin<end
    assert 'SWEET AIR STATIC HALF END result=0' in scope[:begin]
    assert 'SWEET METAL PROBES END process=backboardd result=0' in scope[end:]
    rows={};payloads={};current=None
    events=sorted((m.start(),kind,m) for kind,pattern in (('input',INPUT),('png',PNG),('decode',DECODE),('color',COLOR),('data',DATA),('end',CASE_END)) for m in pattern.finditer(scope))
    for _,kind,m in events:
        index=int(m[1]);assert 1<=index<=5
        if kind=='input':
            assert current is None and index==len(rows)+1
            width,height,pitch=map(int,m.groups()[1:4]);bitmap,top,digest=m.groups()[4:]
            assert (width,height)==((750,1624) if index==5 else (1024,1)) and pitch==width*4
            assert bitmap=='0x42000' and int(top,16)==(0 if index==2 else 0xc0000000)
            assert fnv(authored_input(index))==digest
            rows[index]=dict(case=index,width=width,height=height,input_fnv=digest,input_bitmap=bitmap,top_bits=top,
                             input_author_generated=True,original_UI_pixels_read=False,complete=False)
            current=index
        else:
            assert current==index;row=rows[index]
            if kind=='png':
                assert 'PNG' not in row
                size,wall,cpu=int(m[2]),int(m[4]),int(m[5]);assert 0<size<=16<<20
                assert m[6]=='1' or cpu==0
                row['PNG']=dict(bytes=size,fnv=m[3],wall_ns=wall,thread_cpu_ns=cpu,cpu_valid=m[6]=='1',original_API=True)
            elif kind=='decode':
                assert 'PNG' in row and 'decoded' not in row
                width,height,bpc,bpp,pitch=map(int,m.groups()[1:6]);bitmap,alpha,size,digest=m.groups()[6:]
                assert width==row['width'] and height==row['height'] and 0<bpc<=32 and 0<bpp<=128 and bpp%8==0
                assert int(size)==pitch*height<=16<<20 and pitch>=width*bpp//8
                row['decoded']=dict(bits_per_component=bpc,bits_per_pixel=bpp,pitch=pitch,bitmap_info=bitmap,alpha=int(alpha),bytes=int(size),fnv=digest)
            elif kind=='color':
                assert 'decoded' in row and 'color' not in row and 0<=int(m[4])<=16384
                if int(m[4])==0:assert m[5]=='0000000000000000'
                row['color']=dict(model=int(m[2]),name=m[3],icc_bytes=int(m[4]),icc_fnv=m[5])
            elif kind=='data':
                assert 'color' in row
                label,offset,count,encoded=m.groups()[1:];offset,count=int(offset),int(count)
                assert 0<count<=48 and len(encoded)==count*2
                key=(index,label);data=payloads.setdefault(key,bytearray());assert offset==len(data)
                maximum=row['decoded']['bytes'] if label=='pixels' else row['color']['icc_bytes']
                assert (label=='pixels' and index<=4 and maximum<=32768) or (label=='icc' and index==1)
                data.extend(bytes.fromhex(encoded));assert len(data)<=maximum
            else:
                assert 'color' in row and not row['complete']
                for label,size,digest in (('pixels',row['decoded']['bytes'],row['decoded']['fnv']),('icc',row['color']['icc_bytes'],row['color']['icc_fnv'])):
                    if (label=='pixels' and index<=4) or (label=='icc' and index==1 and size):
                        data=payloads[(index,label)];assert len(data)==size and fnv(data)==digest
                        row[label+'_sha256']=hashlib.sha256(data).hexdigest()
                row['complete']=True;current=None
    assert list(rows)==[1,2,3,4,5] and current is None and all(row['complete'] for row in rows.values())
    for label,pattern in (('INPUT',INPUT),('PNG',PNG),('DECODE',DECODE),('COLOR',COLOR),('DATA',DATA),('CASE END',CASE_END)):
        assert scope.count('SWEET CIF10 '+label+' ')==len(list(pattern.finditer(scope)))
    for index in range(1,5):
        row=rows[index];data=payloads[(index,'pixels')];pixel=row['decoded']['bits_per_pixel']//8
        row['decoded_samples']=[dict(column=x,raw_hex=data[x*pixel:(x+1)*pixel].hex()) for x in (0,1,383,384,385,509,510,511,893,894,895,1023)]
    return dict(token=token,cases=list(rows.values()),original_UIImage_API_referenced=True,author_generated_inputs_only=True,
                installed_converter=False,does_not_admit_lossless_optimization=True,does_not_complete_original_UI=True,
                post_ACK_exit_period_excluded=True),payloads


def inspect(text,token):
    return parse(text,token)[0]


if __name__=='__main__':
    import argparse,json,os
    from pathlib import Path
    from return_evidence import inspect_return
    p=argparse.ArgumentParser();p.add_argument('--directory',required=True);p.add_argument('--output',required=True);p.add_argument('--decoded-directory');a=p.parse_args()
    folder=Path(a.directory);row=json.loads((folder/'results.json').read_bytes())[-1];artifact=row['artifact']
    assert not row['simulated'] and artifact['exception_observer']['cif10_PNG_reference']
    raw=(folder/'cycle-01/serial.raw.log').read_bytes();assert hashlib.sha256(raw).hexdigest()==row['log_sha256']
    returned=inspect_return(raw,artifact['token'],row['checkpoint_log_bytes'],require_drain=True)
    assert returned['complete'] and row['fastboot_returned'] and row['reset_to_fastboot_seconds']>=30
    report,payloads=parse(raw.decode('ascii','replace'),artifact['token'])
    report.update(raw_sha256=row['log_sha256'],source=str(folder.resolve()),original_result=row['result'],strict_return=returned)
    if a.decoded_directory:
        target=Path(a.decoded_directory).resolve();assert target.is_relative_to(Path(os.environ['TEMP']).resolve()) and not target.exists();target.mkdir()
        for (index,label),data in payloads.items():(target/f'case-{index}.{label}').write_bytes(data)
        report['decoded_payload_directory']=str(target)
    out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print(json.dumps(dict(token=artifact['token'],reference_cases=len(report['cases']),original_result=row['result'],optimization_not_installed=True)))
