/* Shared lossless PNG writer. Color conversion stays in the original ImageIO. */
#pragma once
#import <Foundation/Foundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <math.h>
#include <stdint.h>

static NSData *sweet_png_imageio_encode(CGImageRef image,CGFloat scale,uint32_t filter,int *status)
{
    int result=0;NSData *answer=nil;
    if(!image || !isfinite((double)scale) || scale<=0 || scale>4 ||
       (filter && filter!=IMAGEIO_PNG_FILTER_NONE && filter!=IMAGEIO_PNG_FILTER_UP))result=1;
    size_t width=image?CGImageGetWidth(image):0,height=image?CGImageGetHeight(image):0;
    if(!width || !height || width>4096 || height>4096 || width>(4u<<20)/height)result=1;
    if(result){if(status)*status=result;return nil;}
    NSMutableData *output=[NSMutableData data];CGImageDestinationRef destination=NULL;
    @try {
        CGFloat dpi=72*scale;
        NSMutableDictionary *properties=[NSMutableDictionary dictionaryWithDictionary:@{(id)kCGImagePropertyDPIWidth:@(dpi),(id)kCGImagePropertyDPIHeight:@(dpi)}];
        if(filter)properties[(id)kCGImagePropertyPNGDictionary]=@{(id)kCGImagePropertyPNGCompressionFilter:@(filter)};
        destination=CGImageDestinationCreateWithData((CFMutableDataRef)output,CFSTR("public.png"),1,NULL);
        if(!destination)result=2;
        else {
            CGImageDestinationAddImage(destination,image,(CFDictionaryRef)properties);
            if(!CGImageDestinationFinalize(destination))result=3;
        }
        if(!result && (!output.length || output.length>(16u<<20)))result=4;
        if(!result)answer=[[output copy] autorelease];
    } @finally {if(destination)CFRelease(destination);}
    if(status)*status=result;return answer;
}

static bool sweet_png_cif10_contract(CGImageRef image,CGSize points,CGFloat scale,NSInteger orientation)
{
    if(!image || scale!=2 || orientation!=0 || CGImageIsMask(image) || CGImageGetDecode(image)!=NULL ||
       CGImageGetBitsPerComponent(image)!=10 || CGImageGetBitsPerPixel(image)!=32 ||
       CGImageGetBitmapInfo(image)!=0x42000 || CGImageGetAlphaInfo(image)!=kCGImageAlphaNone ||
       CGImageGetRenderingIntent(image)!=kCGRenderingIntentDefault)return false;
    size_t width=CGImageGetWidth(image),height=CGImageGetHeight(image),pitch=CGImageGetBytesPerRow(image);
    if(!width || !height || width>4096 || height>4096 || width>(4u<<20)/height ||
       pitch<width*4 || pitch>SIZE_MAX/height || points.width!=(CGFloat)width/2 || points.height!=(CGFloat)height/2 ||
       CGImageGetUTType(image)!=NULL)return false;
    CGColorSpaceRef color=CGImageGetColorSpace(image);if(!color || CGColorSpaceGetModel(color)!=kCGColorSpaceModelRGB)return false;
    CFStringRef name=CGColorSpaceCopyName(color);bool valid=name && CFEqual(name,kCGColorSpaceExtendedSRGB);
    if(name)CFRelease(name);return valid;
}
