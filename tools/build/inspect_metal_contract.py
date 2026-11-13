"""Compare retained target ObjC limits metadata with a referenced source projection."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def layout(fields):
    sizes={'I':4,'f':4,'Q':8};offset=0;alignment=1;rows=[]
    for name,type_ in fields:
        size=sizes[type_];alignment=max(alignment,size);offset=(offset+size-1)//size*size
        rows.append(dict(name=name,type=type_,offset=offset,bytes=size));offset+=size
    return rows,(offset+alignment-1)//alignment*alignment,alignment


def inspect(metadata,reference,struct_name='QcomMTLDeviceLimits'):
    # Scope to the real base class. An identically named ivar in another class
    # or a truncated encoding is not a layout observation.
    block=metadata.split('@interface _MTLDevice : NSObject {',1)[1].split('@end',1)[0]
    encoding=re.search(r'\{\?=(.*?)\} _limits;',block)[1]
    fields=re.findall(r'"([A-Za-z_][A-Za-z_0-9]*)"([IfQ])',encoding)
    assert ''.join('"'+name+'"'+type_ for name,type_ in fields)==encoding
    assert len(fields)==len(set(name for name,_ in fields)) and fields
    source=reference.split('struct '+struct_name+' {',1)[1].split('};',1)[0]
    source=re.sub(r'//[^\n]*','',source)
    expected=[];types={'uint32_t':'I','float':'f','uint64_t':'Q'}
    for type_,names in re.findall(r'\b(uint32_t|float|uint64_t)\s+([^;]+);',source):
        for name in names.split(','):
            name=name.strip();assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name)
            expected.append((name,types[type_]))
    rows,size,alignment=layout(fields)
    reference_rows,reference_size,reference_alignment=layout(expected)
    expected_by_name={f['name']:f for f in reference_rows}
    differences=[dict(name=f['name'],target_offset=f['offset'],reference_offset=expected_by_name[f['name']]['offset'])
        for f in rows if f['name'] in expected_by_name and f['offset']!=expected_by_name[f['name']]['offset']]
    return dict(fields=rows,limits_bytes=size,limits_alignment=alignment,
        referenced_projection_field_order_and_types_match=fields==expected,
        reference_limits_bytes=reference_size,reference_limits_alignment=reference_alignment,
        common_field_offset_differences=differences,
        reference_fields_absent_from_target=[name for name,_ in expected if name not in dict(fields)],
        target_base_class_present=True,target_initializer_name_present='-[_MTLDevice initWithAcceleratorPort:];' in block,
        target_SPI_protocol_present='@protocol MTLDeviceSPI' in metadata,
        runtime_method_type_encodings_not_yet_verified=True,
        Metal_device_driver_and_rendering_not_yet_provided=True,device_operations=0)


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--metadata',required=True)
    parser.add_argument('--reference-header',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();metadata=Path(args.metadata).read_bytes();reference=Path(args.reference_header).read_bytes()
    report=inspect(metadata.decode(),reference.decode())
    report.update(metadata_sha256=hashlib.sha256(metadata).hexdigest(),reference_sha256=hashlib.sha256(reference).hexdigest(),
        reference_repository='https://github.com/MC-XiaoXiao/ios_sagit',reference_revision='898998cf1b74ea103e0c16d6dc0016822d8d644b',
        reference_path='drivers/gpu/QcomMTL/NativeMetal.h')
    Path(args.output).write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:v for k,v in report.items() if k!='fields'}))
