"""Original system registration/default device with a real kernel registry ID."""
import re
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    starts=list(re.finditer(r'SWEET METAL DISCOVERY begin process=AegirPoster original_MTLAddDevice=1 original_default_device=1 native_GPU=1',scope));assert starts
    records=[];incomplete=0
    for index,start in enumerate(starts):
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)]
        end=re.search(r'SWEET METAL DISCOVERY END result=(\d+) same_original_default=(\d+) original_API=1 GPU_work_preverified=(\d+)',block)
        if not end:incomplete+=1;continue
        assert (end[1],end[2],end[3])==('0','1','1'),'Original system Metal registry failed'
        row=re.search(r'SWEET METAL DISCOVERY device_nil=0 device_class=SweetMTLDevice registry=([0-9a-f]{16}) SPI=1 real_factory=1',block[:end.start()]);assert row and int(row[1],16)>0
        records.append(dict(kernel_registry_id=row[1],original_default_same_device=True,conforms_native_device_SPI=True))
    assert records,'No complete original Metal registration sample'
    return dict(original_Metal_default_device_registration_verified=True,complete_samples=len(records),incomplete_samples=incomplete,
        process='AegirPoster',framework_API_and_registry_properties_not_replaced=True,records=records,full_UI_and_shader_coverage_pending=True)
