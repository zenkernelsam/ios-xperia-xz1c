"""Read a current-token, author-owned PNG reference; never admit an optimizer."""
import hashlib
import re
import struct
import zlib

BEGIN='SWEET CIF10 REFERENCE BEGIN cases=5 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0'
END='SWEET CIF10 REFERENCE END result=0 cases=5 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0'
INPUT=re.compile(r'SWEET CIF10 INPUT case=(\d+) width=(\d+) height=(\d+) bpc=10 bpp=32 pitch=(\d+) bitmap=(0x[0-9a-f]+) top=(0x[0-9a-f]+) input_fnv=([0-9a-f]{16}) scale=2 orientation=0')
PNG=re.compile(r'SWEET CIF10 PNG case=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) returned_nil=0 original_API=1')
DECODE=re.compile(r'SWEET CIF10 DECODE case=(\d+) width=(\d+) height=(\d+) bpc=(\d+) bpp=(\d+) pitch=(\d+) bitmap=(0x[0-9a-f]+) alpha=(\d+) bytes=(\d+) fnv=([0-9a-f]{16})')
COLOR=re.compile(r'SWEET CIF10 COLOR case=(\d+) model=(-?\d+) name=(\S+) icc_bytes=(\d+) icc_fnv=([0-9a-f]{16})')
DATA=re.compile(r'SWEET CIF10 DATA case=(\d+) kind=(pixels|icc|png|baseline_png|none_png|up_png) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
CASE_END=re.compile(r'SWEET CIF10 CASE END case=(\d+) result=0 input_unchanged=1 API_forwarded=1 prediction_used=0')
CONFIG='SWEET CIF10 CONFIG revision=2 PNG_exports=1 vImage_modes=2 original_options=1 converter_installed=0'
CONFIG3='SWEET CIF10 CONFIG revision=3 PNG_exports=1 vImage_modes=2 filter_policies=3 original_options=1 converter_installed=0'
VIMAGE=re.compile(r'SWEET CIF10 VIMAGE case=(\d+) mode=(default|direct) status=(-?\d+) bytes=(\d+) fnv=([0-9a-f]{16}) (?:mismatches|differences)=(\d+) guard=([01]) buffer=([01])')
VIMAGE_TIME=re.compile(r'SWEET CIF10 VIMAGE TIME case=(\d+) mode=(default|direct) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01])')
FILTER=re.compile(r'SWEET CIF10 FILTER case=(\d+) policy=(baseline|none|up) result=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) pixels_equal=([01]) ICC_equal=([01]) metadata_equal=([01]) PNG_equal=([01])')
FILTER_TIME=re.compile(r'SWEET CIF10 FILTER TIME case=(\d+) policy=(baseline|none|up) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01])')


def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'


def authored_input(index):
    width,height=(750,1624) if index>=5 else (1024,1)
    data=bytearray(width*height*4)
    top=0 if index==2 else 0xc0000000
    for y in range(height):
        for x in range(width):
            i=y*width+x
            if index<=2:r=g=b=i&1023
            elif index==3:r,g,b=i&1023,384,384
            elif index==4:r,g,b=i&1023,(i*17)&1023,(i*73)&1023
            elif index==5:r,g,b=(x*17+y*3)&1023,(x*7+y*29)&1023,(x*31+y*11)&1023
            else:r,g,b=384+x*511//(width-1),384+y*511//(height-1),384+(x+y)*511//(width+height-2)
            struct.pack_into('<I',data,i*4,top|(r<<20)|(g<<10)|b)
    return data


def png_container(data,reference,row):
    assert data[:8]==b'\x89PNG\r\n\x1a\n'
    at=8;chunks=[];packed=bytearray();ihdr=None;metadata=bytearray()
    while at<len(data):
        assert at+12<=len(data);size=struct.unpack_from('>I',data,at)[0];kind=data[at+4:at+8]
        assert size<=32768 and at+12+size<=len(data);body=data[at+8:at+8+size];crc=struct.unpack_from('>I',data,at+8+size)[0]
        assert zlib.crc32(kind+body)&0xffffffff==crc
        chunks.append(dict(type=kind.decode('ascii'),bytes=size,payload_sha256=hashlib.sha256(body).hexdigest()))
        if kind==b'IHDR':assert ihdr is None and at==8 and size==13;ihdr=struct.unpack('>IIBBBBB',body)
        elif kind==b'IDAT':packed.extend(body)
        elif kind==b'IEND':assert size==0 and at+12==len(data)
        else:metadata.extend(kind+struct.pack('>I',size)+body)
        at+=12+size
    assert chunks[-1]['type']=='IEND' and ihdr==(row['width'],row['height'],16,2,0,0,0)
    width,height=row['width'],row['height'];pitch=width*6;size=height*(pitch+1);decoder=zlib.decompressobj()
    raw=decoder.decompress(packed,size+1);assert len(raw)==size and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    decoded=bytearray();previous=bytes(pitch)
    for y in range(height):
        off=y*(pitch+1);filter_type=raw[off];assert 0<=filter_type<=4;current=bytearray(raw[off+1:off+pitch+1])
        for i in range(pitch):
            left=current[i-6] if i>=6 else 0;above=previous[i];upper_left=previous[i-6] if i>=6 else 0
            if filter_type==1:predictor=left
            elif filter_type==2:predictor=above
            elif filter_type==3:predictor=(left+above)//2
            elif filter_type==4:
                p=left+above-upper_left;a,b,c=abs(p-left),abs(p-above),abs(p-upper_left)
                predictor=left if a<=b and a<=c else above if b<=c else upper_left
            else:predictor=0
            current[i]=(current[i]+predictor)&255
        decoded.extend(current);previous=current
    for i in range(0,len(decoded),2):decoded[i],decoded[i+1]=decoded[i+1],decoded[i]
    assert row['decoded']['bits_per_component']==16 and row['decoded']['bits_per_pixel']==48 and row['decoded']['bitmap_info']=='0x1000'
    if reference is not None:assert all(decoded[y*pitch:(y+1)*pitch]==reference[y*row['decoded']['pitch']:y*row['decoded']['pitch']+pitch] for y in range(height))
    return dict(chunks=chunks,metadata_sha256=hashlib.sha256(metadata).hexdigest(),IDAT_defiltered_pixels_match_original_CG_decode=reference is not None)


def parse(text,token,revision=1):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    start,finish=text.index(live),text.index(ack);assert start<finish
    scope=text[start:finish]
    assert revision in (1,2,3) and scope.count(CONFIG)==(1 if revision==2 else 0) and scope.count(CONFIG3)==(1 if revision==3 else 0)
    case_count=6 if revision==3 else 5;start_marker=BEGIN.replace('cases=5',f'cases={case_count}');end_marker=END.replace('cases=5',f'cases={case_count}')
    assert scope.count(start_marker)==scope.count(end_marker)==1
    begin,end=scope.index(start_marker),scope.index(end_marker);assert begin<end
    assert 'SWEET AIR STATIC HALF END result=0' in scope[:begin]
    assert 'SWEET METAL PROBES END process=backboardd result=0' in scope[end:]
    rows={};payloads={};current=None
    events=sorted((m.start(),kind,m) for kind,pattern in (('input',INPUT),('png',PNG),('decode',DECODE),('color',COLOR),('vimage',VIMAGE),('vimage_time',VIMAGE_TIME),('filter',FILTER),('filter_time',FILTER_TIME),('data',DATA),('end',CASE_END)) for m in pattern.finditer(scope))
    for _,kind,m in events:
        index=int(m[1]);assert 1<=index<=case_count
        if kind=='input':
            assert current is None and index==len(rows)+1
            width,height,pitch=map(int,m.groups()[1:4]);bitmap,top,digest=m.groups()[4:]
            assert (width,height)==((750,1624) if index>=5 else (1024,1)) and pitch==width*4
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
            elif kind=='vimage':
                assert revision>=2 and 'color' in row;mode=m[2];values=row.setdefault('vImage',{});assert mode not in values
                assert (' differences=' if revision==3 else ' mismatches=') in m[0]
                status,size,diffs=int(m[3]),int(m[4]),int(m[6]);assert size==row['decoded']['bytes'] and 0<=diffs<=size and m[7]=='1'
                if status:assert m[5]=='0000000000000000' and diffs==0
                else:
                    assert m[8]=='1'
                    if not diffs:assert m[5]==row['decoded']['fnv']
                values[mode]=dict(status=status,bytes=size,fnv=m[5],byte_mismatches=diffs,guard_valid=True,buffer_identity=m[8]=='1',pixels_match=not status and not diffs)
            elif kind=='vimage_time':
                assert revision>=2;value=row['vImage'][m[2]];assert 'wall_ns' not in value
                assert m[5]=='1' or int(m[4])==0
                value.update(wall_ns=int(m[3]),thread_cpu_ns=int(m[4]),cpu_valid=m[5]=='1')
            elif kind=='filter':
                assert revision==3 and 'color' in row;policy=m[2];values=row.setdefault('filters',{});assert policy not in values
                status,size=int(m[3]),int(m[4]);assert 0<=size<=16<<20
                values[policy]=dict(result=status,bytes=size,fnv=m[5],pixels_equal=m[6]=='1',ICC_equal=m[7]=='1',metadata_equal=m[8]=='1',PNG_equal=m[9]=='1')
                if status:assert m[6]==m[7]==m[8]==m[9]=='0'
            elif kind=='filter_time':
                assert revision==3;value=row['filters'][m[2]];assert 'wall_ns' not in value and (m[5]=='1' or int(m[4])==0)
                value.update(wall_ns=int(m[3]),thread_cpu_ns=int(m[4]),cpu_valid=m[5]=='1')
            elif kind=='data':
                assert 'color' in row
                label,offset,count,encoded=m.groups()[1:];offset,count=int(offset),int(count)
                assert 0<count<=48 and len(encoded)==count*2
                key=(index,label);data=payloads.setdefault(key,bytearray());assert offset==len(data)
                maximum=row['decoded']['bytes'] if label=='pixels' else row['PNG']['bytes'] if label=='png' else row['filters'][label[:-4]]['bytes'] if label.endswith('_png') else row['color']['icc_bytes']
                assert (label=='pixels' and index<=4 and maximum<=32768) or (label=='icc' and index==1) or (label=='png' and revision>=2 and index<=4 and maximum<=32768) or (label.endswith('_png') and revision==3 and index==1 and maximum<=32768)
                data.extend(bytes.fromhex(encoded));assert len(data)<=maximum
            else:
                assert 'color' in row and not row['complete']
                for label,size,digest in (('pixels',row['decoded']['bytes'],row['decoded']['fnv']),('icc',row['color']['icc_bytes'],row['color']['icc_fnv'])):
                    if (label=='pixels' and index<=4) or (label=='icc' and index==1 and size):
                        data=payloads[(index,label)];assert len(data)==size and fnv(data)==digest
                        row[label+'_sha256']=hashlib.sha256(data).hexdigest()
                row['complete']=True;current=None
                if revision>=2:
                    assert set(row['vImage'])=={'default','direct'} and all('wall_ns' in v for v in row['vImage'].values())
                    if index<=4:
                        data=payloads[(index,'png')];assert len(data)==row['PNG']['bytes'] and fnv(data)==row['PNG']['fnv']
                        row['PNG']['sha256']=hashlib.sha256(data).hexdigest();row['PNG_container']=png_container(data,payloads[(index,'pixels')],row)
                if revision==3:
                    assert set(row['filters'])=={'baseline','none','up'} and all('wall_ns' in v for v in row['filters'].values())
                    if index==1:
                        for policy,value in row['filters'].items():
                            if not value['result'] and value['bytes']<=32768:
                                data=payloads[(index,policy+'_png')];assert len(data)==value['bytes'] and fnv(data)==value['fnv']
                                value['sha256']=hashlib.sha256(data).hexdigest();value['container']=png_container(data,payloads[(index,'pixels')] if value['pixels_equal'] else None,row)
    assert list(rows)==list(range(1,case_count+1)) and current is None and all(row['complete'] for row in rows.values())
    for label,pattern in (('INPUT',INPUT),('PNG',PNG),('DECODE',DECODE),('COLOR',COLOR),('DATA',DATA),('CASE END',CASE_END),('VIMAGE case=',VIMAGE),('VIMAGE TIME',VIMAGE_TIME),('FILTER case=',FILTER),('FILTER TIME',FILTER_TIME)):
        prefix='SWEET CIF10 '+label+('' if label in ('VIMAGE case=','FILTER case=') else ' ')
        assert scope.count(prefix)==len(list(pattern.finditer(scope)))
    for index in range(1,5):
        row=rows[index];data=payloads[(index,'pixels')];pixel=row['decoded']['bits_per_pixel']//8
        row['decoded_samples']=[dict(column=x,raw_hex=data[x*pixel:(x+1)*pixel].hex()) for x in (0,1,383,384,385,509,510,511,893,894,895,1023)]
    return dict(token=token,reference_revision=revision,cases=list(rows.values()),original_UIImage_API_referenced=True,author_generated_inputs_only=True,
                vImage_all_pixel_stages_match=revision>=2 and all(v['pixels_match'] for row in rows.values() for v in row['vImage'].values()),
                filter_policies_match_original=({p:all(not r['filters'][p]['result'] and r['filters'][p]['pixels_equal'] and r['filters'][p]['ICC_equal'] and r['filters'][p]['metadata_equal'] for r in rows.values()) for p in ('baseline','none','up')} if revision==3 else {}),
                installed_converter=False,does_not_admit_lossless_optimization=True,does_not_complete_original_UI=True,
                post_ACK_exit_period_excluded=True),payloads


def inspect(text,token,revision=1):
    return parse(text,token,revision)[0]


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
    report,payloads=parse(raw.decode('ascii','replace'),artifact['token'],artifact['exception_observer']['cif10_reference_revision'])
    report.update(raw_sha256=row['log_sha256'],source=str(folder.resolve()),original_result=row['result'],strict_return=returned)
    if a.decoded_directory:
        target=Path(a.decoded_directory).resolve();assert target.is_relative_to(Path(os.environ['TEMP']).resolve()) and not target.exists();target.mkdir()
        for (index,label),data in payloads.items():(target/f'case-{index}.{label}').write_bytes(data)
        report['decoded_payload_directory']=str(target)
    out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print(json.dumps(dict(token=artifact['token'],reference_cases=len(report['cases']),original_result=row['result'],optimization_not_installed=True)))
