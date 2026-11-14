/* Original Metal registry/default entry points; no method replacement. */
#pragma once
static bool metal_discovery_probe(MetalABIEmit emit)
{
    emit("SWEET METAL DISCOVERY begin process=AegirPoster original_MTLAddDevice=1 original_default_device=1 native_GPU=1");
    NSError *error=nil;id<MTLDevice> device=SweetMTLCreateRegisteredDevice(&error);
    uint64_t registry=device?device.registryID:0;bool valid=device && !error && registry &&
        [(id)device isKindOfClass:objc_lookUpClass("SweetMTLDevice")] && [(id)device conformsToProtocol:objc_getProtocol("MTLDeviceSPI")];
    emit("SWEET METAL DISCOVERY device_nil=%u device_class=%s registry=%016llx SPI=%u real_factory=1",
        !device,device?object_getClassName(device):"nil",(unsigned long long)registry,device && [(id)device conformsToProtocol:objc_getProtocol("MTLDeviceSPI")]);
    id<MTLDevice> again=valid?MTLCreateSystemDefaultDevice():nil;valid=valid && again==device;[again release];
    emit("SWEET METAL DISCOVERY END result=%u same_original_default=%u original_API=1 GPU_work_preverified=1",valid?0:1,valid);
    if(!valid && error)emit("SWEET METAL DISCOVERY failure reason=%s",error.localizedDescription.UTF8String);
    [device release];return valid;
}
