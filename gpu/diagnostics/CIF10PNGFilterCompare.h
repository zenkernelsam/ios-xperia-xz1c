/* Lossless ImageIO filter experiment on author-owned images only. */
#pragma once
#include "../compat/PNGImageIOEncoder.h"
static bool cif10_png_filter_record(CIF10ReferenceEmit emit,unsigned index,CGImageRef input,
                                    NSData *original_png,CGImageRef original_decoded,CFDataRef original_pixels,
                                    CFDataRef original_icc,mach_timebase_info_data_t timebase)
{
    CGImageSourceRef original_source=CGImageSourceCreateWithData((CFDataRef)original_png,NULL);
    if(!original_source)return false;
    CFDictionaryRef original_metadata=CGImageSourceCopyPropertiesAtIndex(original_source,0,NULL);
    if(!original_metadata){CFRelease(original_source);return false;}
    bool safe=true;
    for(unsigned policy=0;policy<3;++policy) { @autoreleasepool {
        NSData *output=nil;
        CGImageSourceRef source=NULL;CGImageRef decoded=NULL;CFDataRef pixels=NULL,icc=NULL;CFDictionaryRef metadata=NULL;
        int result=0;uint64_t elapsed=0,cpu_ns=0;bool cpu_valid=false,end_cpu_valid=false;
        bool pixels_equal=false,icc_equal=false,metadata_equal=false,png_equal=false;
        @try {
            uint64_t cpu=cif10_reference_cpu(&cpu_valid),begin=cif10_reference_wall(timebase);
            output=sweet_png_imageio_encode(input,2,policy==0?0:policy==1?IMAGEIO_PNG_FILTER_NONE:IMAGEIO_PNG_FILTER_UP,&result);
            elapsed=cif10_reference_wall(timebase)-begin;uint64_t cpu_end=cif10_reference_cpu(&end_cpu_valid);
            cpu_valid=cpu_valid && end_cpu_valid && cpu_end>=cpu;cpu_ns=cpu_valid?cpu_end-cpu:0;
            if(!result && (!output.length || output.length>(16u<<20)))result=3;
            if(!result){source=CGImageSourceCreateWithData((CFDataRef)output,NULL);if(!source)result=4;}
            if(!result){decoded=CGImageSourceCreateImageAtIndex(source,0,NULL);metadata=CGImageSourceCopyPropertiesAtIndex(source,0,NULL);if(!decoded || !metadata)result=5;}
            if(!result) {
                pixels=CGDataProviderCopyData(CGImageGetDataProvider(decoded));icc=CGColorSpaceCopyICCData(CGImageGetColorSpace(decoded));
                if(!pixels || CFDataGetLength(pixels)>(16u<<20))result=6;
                else {
                    bool format=CGImageGetWidth(decoded)==CGImageGetWidth(original_decoded) && CGImageGetHeight(decoded)==CGImageGetHeight(original_decoded) &&
                        CGImageGetBitsPerComponent(decoded)==CGImageGetBitsPerComponent(original_decoded) && CGImageGetBitsPerPixel(decoded)==CGImageGetBitsPerPixel(original_decoded) &&
                        CGImageGetBytesPerRow(decoded)==CGImageGetBytesPerRow(original_decoded) && CGImageGetBitmapInfo(decoded)==CGImageGetBitmapInfo(original_decoded);
                    pixels_equal=format && CFEqual(pixels,original_pixels);
                    icc_equal=icc && original_icc?CFEqual(icc,original_icc):icc==original_icc;
                    metadata_equal=CFEqual(metadata,original_metadata);png_equal=[output isEqualToData:original_png];
                }
            }
        } @catch(NSException *exception){(void)exception;result=7;}
        emit("SWEET CIF10 FILTER case=%u policy=%s result=%d bytes=%lu fnv=%016llx pixels_equal=%u ICC_equal=%u metadata_equal=%u PNG_equal=%u",
            index,policy==0?"baseline":policy==1?"none":"up",result,(unsigned long)output.length,
            (unsigned long long)cif10_reference_fnv(output.bytes,output.length),pixels_equal,icc_equal,metadata_equal,png_equal);
        emit("SWEET CIF10 FILTER TIME case=%u policy=%s wall_ns=%llu cpu_ns=%llu cpu_valid=%u",
            index,policy==0?"baseline":policy==1?"none":"up",(unsigned long long)elapsed,(unsigned long long)cpu_ns,cpu_valid);
        if(index==1 && !result && output.length<=32768) {
            const char *kind=policy==0?"baseline_png":policy==1?"none_png":"up_png";
            if(!cif10_reference_export(emit,index,kind,(CFDataRef)output))safe=false;
        }
        if(icc)CFRelease(icc);if(pixels)CFRelease(pixels);if(decoded)CGImageRelease(decoded);if(metadata)CFRelease(metadata);
        if(source)CFRelease(source);
        if(!safe)break;
        // Failures or unequal pixels remain rejected candidates, never an
        // installed encoder. The original reference is still mandatory.
    }}
    CFRelease(original_metadata);CFRelease(original_source);return safe;
}
