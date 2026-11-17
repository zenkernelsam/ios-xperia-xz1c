/* Reference only: author-owned images, original PNG API, no installed converter. */
#pragma once
#import <Foundation/Foundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <TargetConditionals.h>
#include <mach/mach_time.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
static id cif10_reference_image(CGImageRef image)
{ return [UIImage imageWithCGImage:image scale:2 orientation:UIImageOrientationUp]; }
static NSData *cif10_reference_png(id image)
{ return UIImagePNGRepresentation((UIImage *)image); }
#else
// Explicitly supplied by the Mac fixture, not an implementation of UIKit.
extern id cif10_reference_image(CGImageRef image);
extern NSData *cif10_reference_png(id image);
#endif
typedef void (*CIF10ReferenceEmit)(const char *,...);
static uint64_t cif10_reference_fnv(const void *pointer,size_t size)
{
    const uint8_t *bytes=pointer;uint64_t value=UINT64_C(14695981039346656037);
    for(size_t i=0;i<size;++i)value=(value^bytes[i])*UINT64_C(1099511628211);return value;
}
static uint64_t cif10_reference_wall(mach_timebase_info_data_t timebase)
{ return (uint64_t)(((__uint128_t)mach_absolute_time()*timebase.numer)/timebase.denom); }
static uint64_t cif10_reference_cpu(bool *valid)
{
    struct timespec value={0};*valid=clock_gettime(CLOCK_THREAD_CPUTIME_ID,&value)==0;
    return *valid?(uint64_t)value.tv_sec*1000000000+value.tv_nsec:0;
}
#include "CIF10VImageCompare.h"
static void cif10_reference_export(CIF10ReferenceEmit emit,unsigned index,const char *kind,CFDataRef data)
{
    const uint8_t *bytes=CFDataGetBytePtr(data);size_t length=(size_t)CFDataGetLength(data);
    for(size_t at=0;at<length;at+=48) {
        size_t count=length-at;if(count>48)count=48;char hex[97];
        static const char digits[]="0123456789abcdef";
        for(size_t i=0;i<count;++i){hex[i*2]=digits[bytes[at+i]>>4];hex[i*2+1]=digits[bytes[at+i]&15];}hex[count*2]=0;
        emit("SWEET CIF10 DATA case=%u kind=%s offset=%zu bytes=%zu hex=%s",index,kind,at,count,hex);
    }
}
static bool cif10_png_reference_probe(CIF10ReferenceEmit emit)
{
    mach_timebase_info_data_t timebase={0};
    int result=(!emit || mach_timebase_info(&timebase) || !timebase.numer || !timebase.denom)?1:0;
    if(!emit)return false;
    emit("SWEET CIF10 CONFIG revision=2 PNG_exports=1 vImage_modes=2 original_options=1 converter_installed=0");
    emit("SWEET CIF10 REFERENCE BEGIN cases=5 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0");
    unsigned completed=0;
    for(unsigned index=1;index<=5 && !result;++index) { @autoreleasepool {
        size_t width=index==5?750:1024,height=index==5?1624:1;
        size_t length=width*height*4;NSMutableData *input=[NSMutableData dataWithLength:length];
        uint8_t *raw=input.mutableBytes;
        uint32_t top=index==2?0:0xc0000000;
        for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x) {
            unsigned r,g,b,i=(unsigned)(y*width+x);
            if(index<=2)r=g=b=i&1023;
            else if(index==3){r=i&1023;g=b=384;}
            else if(index==4){r=i&1023;g=(i*17)&1023;b=(i*73)&1023;}
            else {r=(unsigned)(x*17+y*3)&1023;g=(unsigned)(x*7+y*29)&1023;b=(unsigned)(x*31+y*11)&1023;}
            uint32_t word=top|(r<<20)|(g<<10)|b;memcpy(raw+4*(y*width+x),&word,4);
        }
        uint64_t input_hash=cif10_reference_fnv(raw,length);
        CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);
        CGDataProviderRef provider=CGDataProviderCreateWithCFData((CFDataRef)input);
        CGBitmapInfo bitmap=(CGBitmapInfo)kCGImagePixelFormatRGBCIF10|(CGBitmapInfo)kCGImageByteOrder32Little;
        CGImageRef image=color && provider?CGImageCreate(width,height,10,32,width*4,color,bitmap,provider,NULL,false,kCGRenderingIntentDefault):NULL;
        CGImageSourceRef source=NULL;CGImageRef decoded=NULL;CFDataRef pixels=NULL,icc=NULL;
        @try {
            if(!image || CGImageGetBitsPerComponent(image)!=10 || CGImageGetBitsPerPixel(image)!=32 ||
               CGImageGetBytesPerRow(image)!=width*4 || CGImageGetBitmapInfo(image)!=0x42000){result=2;break;}
            emit("SWEET CIF10 INPUT case=%u width=%zu height=%zu bpc=10 bpp=32 pitch=%zu bitmap=0x%x top=0x%x input_fnv=%016llx scale=2 orientation=0",
                index,width,height,width*4,CGImageGetBitmapInfo(image),top,(unsigned long long)input_hash);
            id wrapper=cif10_reference_image(image);if(!wrapper){result=3;break;}
            bool cpu_valid=false,cpu_end_valid=false;uint64_t cpu=cif10_reference_cpu(&cpu_valid),begin=cif10_reference_wall(timebase);
            NSData *png=cif10_reference_png(wrapper);
            uint64_t elapsed=cif10_reference_wall(timebase)-begin,cpu_end=cif10_reference_cpu(&cpu_end_valid);
            cpu_valid=cpu_valid && cpu_end_valid && cpu_end>=cpu;
            if(!png || !png.length || png.length>(16u<<20)){result=4;break;}
            emit("SWEET CIF10 PNG case=%u bytes=%lu fnv=%016llx wall_ns=%llu cpu_ns=%llu cpu_valid=%u returned_nil=0 original_API=1",
                index,(unsigned long)png.length,(unsigned long long)cif10_reference_fnv(png.bytes,png.length),
                (unsigned long long)elapsed,(unsigned long long)(cpu_valid?cpu_end-cpu:0),cpu_valid);
            source=CGImageSourceCreateWithData((CFDataRef)png,NULL);if(!source){result=5;break;}
            decoded=CGImageSourceCreateImageAtIndex(source,0,NULL);if(!decoded){result=6;break;}
            if(CGImageGetWidth(decoded)!=width || CGImageGetHeight(decoded)!=height){result=7;break;}
            pixels=CGDataProviderCopyData(CGImageGetDataProvider(decoded));if(!pixels || CFDataGetLength(pixels)<=0 || CFDataGetLength(pixels)>(16u<<20)){result=8;break;}
            if((size_t)CFDataGetLength(pixels)!=CGImageGetBytesPerRow(decoded)*height){result=8;break;}
            icc=CGColorSpaceCopyICCData(CGImageGetColorSpace(decoded));
            if(icc && (CFDataGetLength(icc)<0 || CFDataGetLength(icc)>16384)){result=9;break;}
            CFStringRef name=CGColorSpaceCopyName(CGImageGetColorSpace(decoded));char color_name[64]="unnamed";
            if(name){if(!CFStringGetCString(name,color_name,sizeof(color_name),kCFStringEncodingUTF8))strcpy(color_name,"unnamed");CFRelease(name);}
            emit("SWEET CIF10 DECODE case=%u width=%zu height=%zu bpc=%zu bpp=%zu pitch=%zu bitmap=0x%x alpha=%u bytes=%ld fnv=%016llx",
                index,width,height,CGImageGetBitsPerComponent(decoded),CGImageGetBitsPerPixel(decoded),CGImageGetBytesPerRow(decoded),
                CGImageGetBitmapInfo(decoded),CGImageGetAlphaInfo(decoded),(long)CFDataGetLength(pixels),
                (unsigned long long)cif10_reference_fnv(CFDataGetBytePtr(pixels),(size_t)CFDataGetLength(pixels)));
            emit("SWEET CIF10 COLOR case=%u model=%d name=%.63s icc_bytes=%ld icc_fnv=%016llx",
                index,(int)CGColorSpaceGetModel(CGImageGetColorSpace(decoded)),color_name,icc?(long)CFDataGetLength(icc):0,
                (unsigned long long)(icc?cif10_reference_fnv(CFDataGetBytePtr(icc),(size_t)CFDataGetLength(icc)):0));
            if(!cif10_vimage_record(emit,index,image,decoded,pixels,timebase)){result=13;break;}
            if(index<=4) {
                if(CFDataGetLength(pixels)>32768 || png.length>32768){result=10;break;}
                cif10_reference_export(emit,index,"pixels",pixels);
                cif10_reference_export(emit,index,"png",(CFDataRef)png);
            }
            if(index==1 && icc)cif10_reference_export(emit,index,"icc",icc);
            if(cif10_reference_fnv(input.bytes,input.length)!=input_hash){result=11;break;}
            emit("SWEET CIF10 CASE END case=%u result=0 input_unchanged=1 API_forwarded=1 prediction_used=0",index);++completed;
        } @catch(NSException *exception) {
            (void)exception;result=12;emit("SWEET CIF10 exception case=%u original_UI_unmodified=1",index);
        } @finally {
            if(icc)CFRelease(icc);if(pixels)CFRelease(pixels);if(decoded)CGImageRelease(decoded);if(source)CFRelease(source);
            if(image)CGImageRelease(image);if(provider)CGDataProviderRelease(provider);if(color)CGColorSpaceRelease(color);
        }
    }}
    emit("SWEET CIF10 REFERENCE END result=%d cases=%u original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0",result,completed);
    return !result && completed==5;
}
