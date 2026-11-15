"""Complete original pipeline factory metadata, bounded errors and forwarding."""
import re
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert 'SWEET METAL PIPELINE observer installed=1 methods=3 metadata_only=1 original_returns_errors=1' in scope
    starts=list(re.finditer(r'SWEET METAL PIPELINE begin call=(\d+) format=(\d+) samples=(\d+) depth=(\d+) stencil=(\d+) raster=([01]) original_forwarded=1',scope));assert starts
    records=[];incomplete=0
    for index,start in enumerate(starts):
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)];call=int(start[1]);assert 1<=call<=32
        end=re.search(r'SWEET METAL PIPELINE END call='+str(call)+r' returned_nil=([01]) error_pointer=([01]) error_code=(-?\d+) original_return=1',block)
        if not end:incomplete+=1;continue
        vertex=re.search(r'SWEET METAL PIPELINE function call='+str(call)+r' stage=vertex name=([^\r\n]*)',block[:end.start()])
        fragment=re.search(r'SWEET METAL PIPELINE function call='+str(call)+r' stage=fragment name=([^\r\n]*)',block[:end.start()])
        states=re.search(r'SWEET METAL PIPELINE states call='+str(call)+r' vertex_class=(\S+) fragment_class=(\S+) blend=([01]) mask=(\d+) alphaCoverage=([01]) alphaOne=([01])',block[:end.start()])
        if not vertex or not fragment or not states:incomplete+=1;continue
        error=re.search(r'SWEET METAL PIPELINE error call='+str(call)+r' reason="([^"]*)"',block[end.end():])
        if int(end[3]) and not error:incomplete+=1;continue
        colors=[dict(index=int(i),format=int(f),blend_enabled=bool(int(b)),write_mask=int(w),source_rgb=int(sr),destination_rgb=int(dr),operation_rgb=int(opr),source_alpha=int(sa),destination_alpha=int(da),operation_alpha=int(opa))
            for i,f,b,w,sr,dr,opr,sa,da,opa in re.findall(r'SWEET METAL PIPELINE color call='+str(call)+r' index=(\d+) format=(\d+) blend=([01]) mask=(\d+) srcRGB=(\d+) dstRGB=(\d+) opRGB=(\d+) srcAlpha=(\d+) dstAlpha=(\d+) opAlpha=(\d+)',block[:end.start()])]
        if colors:
            assert [p['index'] for p in colors]==sorted(set(p['index'] for p in colors)) and all(p['index']<8 for p in colors)
            assert colors[0]['index']==0 and colors[0]['format']==int(start[2]) and colors[0]['blend_enabled']==bool(int(states[3])) and colors[0]['write_mask']==int(states[4])
        records.append(dict(call=call,format=int(start[2]),sample_count=int(start[3]),depth_format=int(start[4]),stencil_format=int(start[5]),raster_enabled=bool(int(start[6])),
            vertex_function=vertex[1],fragment_function=fragment[1],vertex_class=states[1],fragment_class=states[2],blend_enabled=bool(int(states[3])),write_mask=int(states[4]),
            returned_nil=bool(int(end[1])),error_pointer=bool(int(end[2])),error_code=int(end[3]),original_error=error[1] if error else None,
            actual_color_attachments=colors,
            vertex_layouts=[dict(index=int(i),stride=int(s),step=int(t),rate=int(r)) for i,s,t,r in re.findall(r'SWEET METAL PIPELINE layout call='+str(call)+r' index=(\d+) stride=(\d+) step=(\d+) rate=(\d+)',block[:end.start()])],
            vertex_attributes=[dict(index=int(i),format=int(f),offset=int(o),buffer=int(b)) for i,f,o,b in re.findall(r'SWEET METAL PIPELINE attribute call='+str(call)+r' index=(\d+) format=(\d+) offset=(\d+) buffer=(\d+)',block[:end.start()])]))
    assert records,'No complete original pipeline factory return/error record'
    return dict(actual_original_pipeline_factory_metadata_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        observer_does_not_repair_shader_or_original_UI=True)
