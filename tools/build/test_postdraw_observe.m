/* Real ImageIO bytes/profile preservation and iOS-only API forwarding fixture. */
#define LILAC_POSTDRAW_TEST 1
#import <Foundation/Foundation.h>
#include <ColorSync/ColorSync.h>
#include "../../gpu/diagnostics/PostDrawObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static id expected_image;
static NSData *answer;
static NSException *original_exception;
static bool throw_png, nested_png;
static atomic_uint original_calls;
static unsigned begins, ends, images;
static CGImageRef nested_image;
static NSData *encode(CGImageRef image, bool observe)
{
    NSMutableData *data = [NSMutableData data];
    CGImageDestinationRef destination = CGImageDestinationCreateWithData((CFMutableDataRef)data, CFSTR("public.png"), 1, NULL);
    assert(destination);
    if (observe) postdraw_add_image(destination, image, NULL);
    else CGImageDestinationAddImage(destination, image, NULL);
    bool valid = observe ? postdraw_finalize(destination) : CGImageDestinationFinalize(destination);
    CFRelease(destination);
    assert(valid && data.length);
    return data;
}
NSData *UIImagePNGRepresentation(id image)
{
    assert(image == expected_image && errno == 73);
    atomic_fetch_add(&original_calls, 1);
    errno = 91;
    if (throw_png) @throw original_exception;
    if (nested_png) {
        NSData *result = encode(nested_image, true);
        errno = 91;
        return result;
    }
    return answer;
}
static void emit(const char *format, ...)
{
    char line[512]; va_list args; va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args); va_end(args);
    assert(length > 0 && length < 250);
    if (strstr(line, "SWEET POSTDRAW BEGIN ")) ++begins;
    if (strstr(line, "SWEET POSTDRAW END ")) ++ends;
    if (strstr(line, "SWEET POSTDRAW IMAGE ")) ++images;
    puts(line);
    // The instrumentation must not leak its own errno into either API side.
    errno = 117;
}
static CGImageRef make_image(size_t width, size_t height, CFStringRef color_name, NSMutableData **original)
{
    size_t stride = width*4, bytes = stride*height;
    NSMutableData *data = [NSMutableData dataWithLength:bytes];
    uint8_t *pixels = data.mutableBytes;
    for (size_t y=0; y<height; ++y) for (size_t x=0; x<width; ++x) {
        size_t at = y*stride+x*4;
        pixels[at]=(uint8_t)((x*11+y*7)&255); pixels[at+1]=(uint8_t)((x*3+y*17)&255);
        pixels[at+2]=(uint8_t)((x^y)&255); pixels[at+3]=255;
    }
    CGColorSpaceRef color = CGColorSpaceCreateWithName(color_name);
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((CFDataRef)data);
    CGImageRef image = CGImageCreate(width, height, 8, 32, stride, color,
        kCGBitmapByteOrder32Big | kCGImageAlphaLast, provider, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(provider); CGColorSpaceRelease(color);
    assert(image); *original=data; return image;
}
static void compare_decode(NSData *left, NSData *right, size_t width, size_t height)
{
    assert([left isEqualToData:right]);
    CGImageSourceRef a=CGImageSourceCreateWithData((CFDataRef)left,NULL), b=CGImageSourceCreateWithData((CFDataRef)right,NULL);
    assert(a && b);
    CFDictionaryRef pa=CGImageSourceCopyPropertiesAtIndex(a,0,NULL), pb=CGImageSourceCopyPropertiesAtIndex(b,0,NULL);
    assert(pa && pb && CFEqual(pa,pb)); CFRelease(pa); CFRelease(pb);
    CGImageRef ia=CGImageSourceCreateImageAtIndex(a,0,NULL), ib=CGImageSourceCreateImageAtIndex(b,0,NULL);
    assert(ia && ib && CGImageGetWidth(ia)==width && CGImageGetHeight(ia)==height);
    CFDataRef ca=CGColorSpaceCopyICCData(CGImageGetColorSpace(ia)), cb=CGColorSpaceCopyICCData(CGImageGetColorSpace(ib));
    assert(ca && cb && CFEqual(ca,cb)); CFRelease(ca); CFRelease(cb);
    size_t size=width*height*4;
    uint8_t *rawa=malloc(size+64), *rawb=malloc(size+64); assert(rawa && rawb);
    memset(rawa,0xa7,size+64); memset(rawb,0xa7,size+64);
    CGContextRef xa=CGBitmapContextCreate(rawa+32,width,height,8,width*4,CGImageGetColorSpace(ia),kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);
    CGContextRef xb=CGBitmapContextCreate(rawb+32,width,height,8,width*4,CGImageGetColorSpace(ib),kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);
    assert(xa && xb); CGContextDrawImage(xa,CGRectMake(0,0,width,height),ia); CGContextDrawImage(xb,CGRectMake(0,0,width,height),ib);
    assert(!memcmp(rawa,rawb,size+64));
    for(unsigned i=0;i<32;++i) assert(rawa[i]==0xa7 && rawa[size+32+i]==0xa7);
    CGContextRelease(xa); CGContextRelease(xb); free(rawa); free(rawb);
    CGImageRelease(ia); CGImageRelease(ib); CFRelease(a); CFRelease(b);
}
int main(void) { @autoreleasepool {
    expected_image=[NSObject new]; answer=[NSData dataWithBytes:"PNG-return-identity" length:19];
    errno=73; assert(postdraw_png(expected_image)==answer && errno==91 && !begins);
    assert(postdraw_observe(emit));
    errno=73; assert(postdraw_png(expected_image)==answer && errno==91);
    answer=nil; errno=73; assert(postdraw_png(expected_image)==nil && errno==91);
    throw_png=true; original_exception=[NSException exceptionWithName:@"OriginalPNGThrow" reason:@"kept" userInfo:nil];
    bool caught=false; errno=73;
    @try { (void)postdraw_png(expected_image); } @catch(NSException *value) { assert(value==original_exception); caught=true; }
    assert(caught && errno==91 && !postdraw_depth && !postdraw_root); throw_png=false;
    for(unsigned pass=0; pass<3; ++pass) {
        size_t width=pass==2?750:64, height=pass==2?1624:48;
        NSMutableData *pixels=nil; CGImageRef image=make_image(width,height,pass==1?kCGColorSpaceDisplayP3:kCGColorSpaceSRGB,&pixels);
        NSData *input_copy=[[pixels copy] autorelease];
        NSData *baseline=[[encode(image,false) copy] autorelease], *observed=[[encode(image,true) copy] autorelease];
        compare_decode(baseline,observed,width,height); assert([pixels isEqualToData:input_copy]);
        if(pass==0) { nested_image=image; nested_png=true; errno=73;
            NSData *png=postdraw_png(expected_image); assert(errno==91 && [png isEqualToData:baseline]);
            assert(!postdraw_depth && !postdraw_root); nested_png=false; }
        CGImageRelease(image);
    }
    NSMutableData *empty=[NSMutableData data];
    CGImageDestinationRef bad=CGImageDestinationCreateWithData((CFMutableDataRef)empty,CFSTR("public.png"),1,NULL);
    assert(bad && !postdraw_finalize(bad)); CFRelease(bad);
    assert(postdraw_colorsync(NULL,NULL)==NULL);
    ColorSyncProfileRef srgb=ColorSyncProfileCreateWithName(kColorSyncSRGBProfile);
    assert(srgb);
    NSArray *sequence=@[@{(id)kColorSyncProfile:(id)srgb,(id)kColorSyncRenderingIntent:(id)kColorSyncRenderingIntentPerceptual,(id)kColorSyncTransformTag:(id)kColorSyncTransformDeviceToPCS},
        @{(id)kColorSyncProfile:(id)srgb,(id)kColorSyncRenderingIntent:(id)kColorSyncRenderingIntentPerceptual,(id)kColorSyncTransformTag:(id)kColorSyncTransformPCSToDevice}];
    ColorSyncTransformRef transform=postdraw_colorsync((CFArrayRef)sequence,NULL); assert(transform); CFRelease(transform); CFRelease(srgb);
    answer=[NSData dataWithBytes:"same" length:4];
    for(unsigned i=0;i<80;++i) { errno=73; assert(postdraw_png(expected_image)==answer && errno==91); }
    assert(begins==64 && ends==64 && images==4 && atomic_load(&postdraw_calls)>64);
    assert(atomic_load(&original_calls)==85 && !postdraw_depth && !postdraw_root);
    [expected_image release];
    puts("postdraw observer PASS: exact return/nil/throw/errno, nesting, 64-record bound, real PNG bytes/decoded pixels/ICC and original input preserved");
} return 0; }
