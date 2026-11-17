/* Author-image experiment: the public converter must match original PNG pixels. */
#pragma once
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
#include <errno.h>

_Static_assert(sizeof(vImage_CGImageFormat)==40 && sizeof(vImage_Buffer)==32,"Unexpected vImage arm64/host ABI");
typedef struct {
    long status;
    size_t bytes,mismatches;
    uint64_t fnv,wall_ns,cpu_ns;
    bool guard,buffer,cpu_valid;
} CIF10VImageResult;

static CIF10VImageResult cif10_vimage_compare(CGImageRef input,CGImageRef expected,CFDataRef expected_pixels,
                                              bool direct,mach_timebase_info_data_t timebase)
{
    CIF10VImageResult out={.status=-EINVAL,.guard=true,.buffer=true};
    if(!input || !expected || !expected_pixels)return out;
    size_t width=CGImageGetWidth(expected),height=CGImageGetHeight(expected),row=CGImageGetBytesPerRow(expected);
    size_t bytes=(size_t)CFDataGetLength(expected_pixels);
    if(!width || !height || !row || height>(16u<<20)/row || bytes!=row*height || bytes>(16u<<20))return out;
    uint8_t *allocation=malloc(bytes+128);if(!allocation){out.status=-ENOMEM;return out;}
    memset(allocation,0xa7,bytes+128);uint8_t *pixels=allocation+64;
    vImage_Buffer buffer={.data=pixels,.height=height,.width=width,.rowBytes=row};
    vImage_CGImageFormat format={.bitsPerComponent=(uint32_t)CGImageGetBitsPerComponent(expected),
        .bitsPerPixel=(uint32_t)CGImageGetBitsPerPixel(expected),.colorSpace=CGImageGetColorSpace(expected),
        .bitmapInfo=CGImageGetBitmapInfo(expected),.version=0,.decode=CGImageGetDecode(expected),
        .renderingIntent=CGImageGetRenderingIntent(input)};
    bool cpu_end_valid=false;uint64_t cpu=cif10_reference_cpu(&out.cpu_valid),begin=cif10_reference_wall(timebase);
    if(!direct)out.status=vImageBuffer_InitWithCGImage(&buffer,&format,NULL,input,kvImageNoAllocate);
    else {
        vImage_CGImageFormat source_format={.bitsPerComponent=(uint32_t)CGImageGetBitsPerComponent(input),
            .bitsPerPixel=(uint32_t)CGImageGetBitsPerPixel(input),.colorSpace=CGImageGetColorSpace(input),
            .bitmapInfo=CGImageGetBitmapInfo(input),.version=0,.decode=CGImageGetDecode(input),
            .renderingIntent=CGImageGetRenderingIntent(input)};
        vImage_Error error=0;
        vImageConverterRef converter=vImageConverter_CreateWithCGImageFormat(&source_format,&format,NULL,kvImageDoNotTile,&error);
        if(!converter)out.status=error?error:-EINVAL;
        else {
            CFDataRef source_data=CGDataProviderCopyData(CGImageGetDataProvider(input));
            size_t input_row=CGImageGetBytesPerRow(input);
            if(!source_data || (size_t)CFDataGetLength(source_data)!=input_row*height)out.status=-EINVAL;
            else if(vImageConverter_GetNumberOfSourceBuffers(converter)!=1 || vImageConverter_GetNumberOfDestinationBuffers(converter)!=1)out.status=-EINVAL;
            else {
                vImage_Buffer source_buffer={.data=(void *)CFDataGetBytePtr(source_data),.height=height,.width=width,.rowBytes=input_row};
                out.status=vImageConvert_AnyToAny(converter,&source_buffer,&buffer,NULL,kvImageDoNotTile);
            }
            if(source_data)CFRelease(source_data);vImageConverter_Release(converter);
        }
    }
    out.wall_ns=cif10_reference_wall(timebase)-begin;uint64_t cpu_end=cif10_reference_cpu(&cpu_end_valid);
    out.cpu_valid=out.cpu_valid && cpu_end_valid && cpu_end>=cpu;out.cpu_ns=out.cpu_valid?cpu_end-cpu:0;
    for(unsigned i=0;i<64;++i)if(allocation[i]!=0xa7 || allocation[bytes+64+i]!=0xa7)out.guard=false;
    out.buffer=buffer.data==pixels && buffer.width==width && buffer.height==height && buffer.rowBytes==row;
    out.bytes=bytes;
    if(!out.status && out.guard && out.buffer) {
        const uint8_t *reference=CFDataGetBytePtr(expected_pixels);
        for(size_t i=0;i<bytes;++i)out.mismatches+=pixels[i]!=reference[i];
        out.fnv=cif10_reference_fnv(pixels,bytes);
    }
    free(allocation);return out;
}
static bool cif10_vimage_record(CIF10ReferenceEmit emit,unsigned index,CGImageRef input,CGImageRef expected,
                                 CFDataRef pixels,mach_timebase_info_data_t timebase)
{
    bool safe=true;
    for(unsigned direct=0;direct<2;++direct) {
        CIF10VImageResult result=cif10_vimage_compare(input,expected,pixels,direct!=0,timebase);
        emit("SWEET CIF10 VIMAGE case=%u mode=%s status=%ld bytes=%zu fnv=%016llx differences=%zu guard=%u buffer=%u",
            index,direct?"direct":"default",result.status,result.bytes,(unsigned long long)result.fnv,result.mismatches,result.guard,result.buffer);
        emit("SWEET CIF10 VIMAGE TIME case=%u mode=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u",
            index,direct?"direct":"default",(unsigned long long)result.wall_ns,(unsigned long long)result.cpu_ns,result.cpu_valid);
        safe=safe && result.guard && (result.status || result.buffer);
    }
    // A numerical mismatch is retained as a rejected candidate; corruption is
    // a failed reference run. No candidate is installed by this probe.
    return safe;
}
