/* iOS-only forwarding fixture plus real CoreGraphics metadata/ImageIO bytes. */
#define LILAC_POSTDRAW_TEST 1
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/PostDrawObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

@interface PNGInputFixture : NSObject <SweetPNGInputGetters> {
@public
    CGImageRef cg;
    CGSize points;
    CGFloat imageScale;
    NSInteger orientation;
    unsigned getterCalls;
}
@end
@implementation PNGInputFixture
- (CGImageRef)CGImage { ++getterCalls; return cg; }
- (CGSize)size { ++getterCalls; return points; }
- (CGFloat)scale { ++getterCalls; return imageScale; }
- (NSInteger)imageOrientation { ++getterCalls; return orientation; }
@end
static NSException *getterException;
@interface PNGThrowingGetter : PNGInputFixture @end
@implementation PNGThrowingGetter
- (CGImageRef)CGImage { ++getterCalls; errno=155; @throw getterException; }
@end
// An ABI mismatch must be rejected before invoking any metadata getter.
@interface PNGBadABI : NSObject
- (CGImageRef)CGImage;
- (CGSize)size;
- (int)scale;
- (NSInteger)imageOrientation;
@end
@implementation PNGBadABI
- (CGImageRef)CGImage { abort(); }
- (CGSize)size { abort(); }
- (int)scale { abort(); }
- (NSInteger)imageOrientation { abort(); }
@end

typedef struct { uint8_t *bytes; size_t length; unsigned reads; } Provider;
static const void *provider_pointer(void *info)
{ Provider *value=info; ++value->reads; return value->bytes; }
static size_t provider_bytes(void *info, void *buffer, off_t offset, size_t count)
{
    Provider *value=info; ++value->reads;
    if (offset<0 || (uint64_t)offset>value->length) return 0;
    size_t available=value->length-(size_t)offset; if(count>available)count=available;
    memcpy(buffer,value->bytes+(size_t)offset,count); return count;
}
static CGImageRef make_image(Provider *provider, unsigned bits, CFStringRef name)
{
    size_t width=37,height=19,stride=width*4*(bits/8)+16;
    provider->length=stride*height; provider->bytes=malloc(provider->length);
    assert(provider->bytes); memset(provider->bytes,0xa7,provider->length);
    for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x) {
        uint8_t *pixel=provider->bytes+y*stride+x*4*(bits/8);
        if(bits==8) {pixel[0]=(uint8_t)(x*7);pixel[1]=(uint8_t)(y*13);pixel[2]=(uint8_t)(x^y);pixel[3]=255;}
        else if(bits==16) {uint16_t half[4]={0x3400,0x3800,0x3a00,0x3c00};memcpy(pixel,half,sizeof(half));}
        else {float fp[4]={-0.125f,(float)x/36,1.125f,1.0f};memcpy(pixel,fp,sizeof(fp));}
    }
    CGDataProviderDirectCallbacks callbacks={0,provider_pointer,NULL,provider_bytes,NULL};
    CGDataProviderRef data=CGDataProviderCreateDirect(provider,provider->length,&callbacks);
    CGColorSpaceRef color=CGColorSpaceCreateWithName(name);
    assert(data && color);
    CGBitmapInfo bitmap=(CGBitmapInfo)kCGImageAlphaLast;
    if(bits==8)bitmap|=kCGBitmapByteOrder32Big;
    else bitmap|=kCGBitmapFloatComponents|(bits==16?kCGBitmapByteOrder16Little:kCGBitmapByteOrder32Little);
    CGImageRef image=CGImageCreate(width,height,bits,bits*4,stride,color,bitmap,data,NULL,false,kCGRenderingIntentDefault);
    CGDataProviderRelease(data); CGColorSpaceRelease(color); assert(image); return image;
}
static NSData *encode_original(CGImageRef image)
{
    NSMutableData *data=[NSMutableData data];
    CGImageDestinationRef destination=CGImageDestinationCreateWithData((CFMutableDataRef)data,CFSTR("public.png"),1,NULL);
    assert(destination); CGImageDestinationAddImage(destination,image,NULL);
    assert(CGImageDestinationFinalize(destination)); CFRelease(destination); assert(data.length); return data;
}
static id expectedImage;
static NSData *answer;
static NSException *originalException;
static bool encodeImage,throwOriginal;
static unsigned originalCalls,inputRecords,uiRecords,cgRecords,colorRecords;
NSData *UIImagePNGRepresentation(id image)
{
    assert(image==expectedImage && errno==73); ++originalCalls;
    errno=91;
    if(throwOriginal)@throw originalException;
    NSData *result=encodeImage?encode_original(((PNGInputFixture *)image)->cg):answer;
    errno=91; return result;
}
static void emit(const char *format,...)
{
    char line[512]; va_list args;va_start(args,format);
    int length=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(length>0 && length<250);
    inputRecords+=strstr(line,"SWEET POSTDRAW PNG_INPUT ")!=NULL;
    uiRecords+=strstr(line,"SWEET POSTDRAW PNG_UI ")!=NULL;
    cgRecords+=strstr(line,"SWEET POSTDRAW PNG_CG ")!=NULL;
    colorRecords+=strstr(line,"SWEET POSTDRAW PNG_COLOR ")!=NULL;
    errno=117;
}
static void compare_png(NSData *left,NSData *right)
{
    assert([left isEqualToData:right]);
    CGImageSourceRef a=CGImageSourceCreateWithData((CFDataRef)left,NULL),b=CGImageSourceCreateWithData((CFDataRef)right,NULL);
    assert(a && b);
    CFDictionaryRef pa=CGImageSourceCopyPropertiesAtIndex(a,0,NULL),pb=CGImageSourceCopyPropertiesAtIndex(b,0,NULL);
    assert(pa && pb && CFEqual(pa,pb));CFRelease(pa);CFRelease(pb);
    CGImageRef ia=CGImageSourceCreateImageAtIndex(a,0,NULL),ib=CGImageSourceCreateImageAtIndex(b,0,NULL);
    assert(ia && ib && CGImageGetWidth(ia)==37 && CGImageGetHeight(ia)==19);
    CFDataRef ca=CGColorSpaceCopyICCData(CGImageGetColorSpace(ia)),cb=CGColorSpaceCopyICCData(CGImageGetColorSpace(ib));
    assert(ca && cb && CFEqual(ca,cb));CFRelease(ca);CFRelease(cb);
    uint8_t aPixels[37*19*4],bPixels[37*19*4];
    CGColorSpaceRef rgb=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef ac=CGBitmapContextCreate(aPixels,37,19,8,37*4,rgb,(CGBitmapInfo)kCGImageAlphaPremultipliedLast);
    CGContextRef bc=CGBitmapContextCreate(bPixels,37,19,8,37*4,rgb,(CGBitmapInfo)kCGImageAlphaPremultipliedLast);
    assert(ac && bc);CGContextDrawImage(ac,CGRectMake(0,0,37,19),ia);CGContextDrawImage(bc,CGRectMake(0,0,37,19),ib);
    assert(!memcmp(aPixels,bPixels,sizeof(aPixels)));
    CGContextRelease(ac);CGContextRelease(bc);CGColorSpaceRelease(rgb);
    CGImageRelease(ia);CGImageRelease(ib);CFRelease(a);CFRelease(b);
}
int main(void) { @autoreleasepool {
    answer=[NSData dataWithBytes:"original identity" length:17];
    expectedImage=[NSObject new];errno=73;assert(postdraw_png(expectedImage)==answer && errno==91 && originalCalls==1 && !inputRecords);
    assert(postdraw_observe(emit));
    const unsigned bits[]={8,8,16,32};
    CFStringRef names[]={kCGColorSpaceSRGB,kCGColorSpaceDisplayP3,kCGColorSpaceExtendedLinearSRGB,kCGColorSpaceExtendedLinearDisplayP3};
    [expectedImage release];
    for(unsigned pass=0;pass<4;++pass) {
        Provider provider={0};CGImageRef image=make_image(&provider,bits[pass],names[pass]);
        PNGInputFixture *fixture=[PNGInputFixture new];fixture->cg=image;fixture->points=CGSizeMake(18.5,9.5);fixture->imageScale=2;fixture->orientation=3;
        unsigned reads=provider.reads;errno=73;SweetPNGInput input=postdraw_read_png_input(fixture);
        assert(errno==73 && provider.reads==reads && fixture->getterCalls==4);
        assert(input.getters_available && !input.getter_error && !input.cgimage_nil && !input.color_nil);
        assert(input.width==37 && input.height==19 && input.bpc==bits[pass] && input.bpp==bits[pass]*4);
        assert(input.pitch==37*4*(bits[pass]/8)+16 && input.color_model==kCGColorSpaceModelRGB && input.components==3);
        assert(input.points.width==18.5 && input.points.height==9.5 && input.scale==2 && input.orientation==3);
        assert(input.named && !input.name_sanitized && input.extended==(pass>=2));
        uint8_t *copy=malloc(provider.length);assert(copy);memcpy(copy,provider.bytes,provider.length);
        NSData *baseline=[[encode_original(image) copy] autorelease];expectedImage=fixture;encodeImage=true;
        unsigned calls=originalCalls;errno=73;NSData *observed=postdraw_png(fixture);
        assert(errno==91 && originalCalls==calls+1 && fixture->getterCalls==8);
        compare_png(baseline,observed);assert(!memcmp(copy,provider.bytes,provider.length));free(copy);
        [fixture release];CGImageRelease(image);free(provider.bytes);encodeImage=false;
    }
    expectedImage=nil;answer=nil;unsigned calls=originalCalls;errno=73;
    assert(postdraw_png(nil)==nil && errno==91 && originalCalls==calls+1);
    PNGBadABI *bad=[PNGBadABI new];expectedImage=bad;answer=[NSData dataWithBytes:"same" length:4];errno=73;
    assert(!postdraw_read_png_input(bad).getters_available && errno==73);
    assert(postdraw_png(bad)==answer && errno==91);[bad release];
    getterException=[NSException exceptionWithName:@"DiagnosticGetter" reason:@"must not replace PNG" userInfo:nil];
    PNGThrowingGetter *throwing=[PNGThrowingGetter new];expectedImage=throwing;errno=73;
    assert(postdraw_read_png_input(throwing).getter_error && errno==73);
    assert(postdraw_png(throwing)==answer && errno==91);
    originalException=[NSException exceptionWithName:@"OriginalPNG" reason:@"preserved" userInfo:nil];throwOriginal=true;
    bool caught=false;errno=73;@try{(void)postdraw_png(throwing);}@catch(NSException *value){assert(value==originalException);caught=true;}
    assert(caught && errno==91 && !postdraw_depth && !postdraw_root);[throwing release];
    assert(inputRecords==8 && uiRecords==8 && cgRecords==8 && colorRecords==8);
    puts("PNG input observer PASS: typed getter rejection/nil/throw/errno, no pixel-provider reads, real 8/16/32-bit sRGB/P3 PNG bytes/pixels/ICC and original input preserved; UIKit forwarding is a host fixture");
} return 0; }
