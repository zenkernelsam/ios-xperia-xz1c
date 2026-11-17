/* Original UIKit is an explicit host fixture; ImageIO/pixels/ICC are real. */
#import <Foundation/Foundation.h>
#include "../../gpu/compat/PNGImageDispatch.h"
#include <assert.h>

@interface PNGDispatchFixture : NSObject <SweetPNGDispatchImage> {
@public CGImageRef image;CGFloat imageScale;NSInteger orientation;NSArray *frames;
}
@end
@implementation PNGDispatchFixture
- (CGImageRef)CGImage {errno=155;return image;}
- (CGSize)size {return CGSizeMake(CGImageGetWidth(image)/imageScale,CGImageGetHeight(image)/imageScale);}
- (CGFloat)scale {return imageScale;}
- (NSInteger)imageOrientation {return orientation;}
- (NSArray *)images {return frames;}
@end
static id expectedImage;static NSData *answer;static unsigned originals;static bool originalThrows;
static NSException *expectedException;
static NSData *original(id image)
{ assert(image==expectedImage && errno==73);++originals;errno=91;if(originalThrows)@throw expectedException;return answer; }
static NSData *failing_encoder(CGImageRef image,CGFloat scale,uint32_t filter,int *status)
{assert(image && scale==2 && filter==IMAGEIO_PNG_FILTER_UP);errno=177;*status=3;return nil;}
static NSData *throwing_encoder(CGImageRef image,CGFloat scale,uint32_t filter,int *status)
{(void)image;(void)scale;(void)filter;(void)status;errno=177;@throw [NSException exceptionWithName:@"EncoderDiagnostic" reason:@"fallback" userInfo:nil];}
static void compare(NSData *left,NSData *right)
{
    CGImageSourceRef a=CGImageSourceCreateWithData((CFDataRef)left,NULL),b=CGImageSourceCreateWithData((CFDataRef)right,NULL);assert(a && b);
    CGImageRef ai=CGImageSourceCreateImageAtIndex(a,0,NULL),bi=CGImageSourceCreateImageAtIndex(b,0,NULL);assert(ai && bi);
    CFDictionaryRef ap=CGImageSourceCopyPropertiesAtIndex(a,0,NULL),bp=CGImageSourceCopyPropertiesAtIndex(b,0,NULL);assert(ap && bp && CFEqual(ap,bp));
    CFDataRef ab=CGDataProviderCopyData(CGImageGetDataProvider(ai)),bb=CGDataProviderCopyData(CGImageGetDataProvider(bi));assert(ab && bb && CFEqual(ab,bb));
    CFDataRef ac=CGColorSpaceCopyICCData(CGImageGetColorSpace(ai)),bc=CGColorSpaceCopyICCData(CGImageGetColorSpace(bi));assert(ac && bc && CFEqual(ac,bc));
    assert(CGImageGetBitsPerComponent(ai)==16 && CGImageGetBitsPerComponent(bi)==16);
    CFRelease(ap);CFRelease(bp);CFRelease(ab);CFRelease(bb);CFRelease(ac);CFRelease(bc);CGImageRelease(ai);CGImageRelease(bi);CFRelease(a);CFRelease(b);
}
int main(void) { @autoreleasepool {
    const size_t width=37,height=19;NSMutableData *input=[NSMutableData dataWithLength:width*height*4];
    for(size_t i=0;i<width*height;++i){uint32_t r=384+i%512,g=384+(i*17)%512,b=384+(i*73)%512;uint32_t v=0xc0000000|(r<<20)|(g<<10)|b;memcpy((uint8_t *)input.mutableBytes+4*i,&v,4);}
    NSData *copy=[[input copy] autorelease];CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);CGDataProviderRef provider=CGDataProviderCreateWithCFData((CFDataRef)input);
    CGImageRef image=CGImageCreate(width,height,10,32,width*4,color,(CGBitmapInfo)kCGImagePixelFormatRGBCIF10|(CGBitmapInfo)kCGImageByteOrder32Little,provider,NULL,false,kCGRenderingIntentDefault);assert(image);
    PNGDispatchFixture *fixture=[PNGDispatchFixture new];fixture->image=image;fixture->imageScale=2;expectedImage=fixture;
    CGImageRef cg=NULL;CGFloat scale=0;errno=73;assert(sweet_png_image_eligible(fixture,[PNGDispatchFixture class],&cg,&scale) && cg==image && scale==2 && errno==73);
    int status=0;answer=sweet_png_imageio_encode(image,2,IMAGEIO_PNG_FILTER_NONE,&status);assert(answer && !status);
    int route=-1;errno=73;NSData *png=sweet_png_dispatch(fixture,original,sweet_png_imageio_encode,true,cg,scale,&route);
    assert(png && route==SWEET_PNG_UP && !originals && errno==73);compare(answer,png);assert([input isEqualToData:copy]);
    errno=73;assert(sweet_png_dispatch(fixture,original,failing_encoder,true,cg,scale,&route)==answer && route==SWEET_PNG_FAILED_FALLBACK && originals==1 && errno==91);
    errno=73;assert(sweet_png_dispatch(fixture,original,throwing_encoder,true,cg,scale,&route)==answer && route==SWEET_PNG_THROW_FALLBACK && originals==2 && errno==91);
    expectedImage=nil;answer=nil;errno=73;assert(sweet_png_dispatch(nil,original,sweet_png_imageio_encode,false,NULL,0,&route)==nil && route==SWEET_PNG_ORIGINAL && originals==3 && errno==91);
    expectedException=[NSException exceptionWithName:@"OriginalPNG" reason:@"identity" userInfo:nil];originalThrows=true;bool caught=false;errno=73;
    @try{(void)sweet_png_dispatch(nil,original,sweet_png_imageio_encode,false,NULL,0,&route);}@catch(NSException *value){assert(value==expectedException);caught=true;}
    assert(caught && originals==4 && errno==91);originalThrows=false;expectedImage=fixture;
    fixture->orientation=1;errno=73;assert(!sweet_png_image_eligible(fixture,[PNGDispatchFixture class],&cg,&scale) && errno==73);fixture->orientation=0;
    fixture->imageScale=1;errno=73;assert(!sweet_png_image_eligible(fixture,[PNGDispatchFixture class],&cg,&scale) && errno==73);fixture->imageScale=2;
    fixture->frames=@[fixture];errno=73;assert(!sweet_png_image_eligible(fixture,[PNGDispatchFixture class],&cg,&scale) && errno==73);fixture->frames=nil;
    errno=73;assert(!sweet_png_image_eligible(fixture,[NSObject class],&cg,&scale) && errno==73);
    status=0;assert(!sweet_png_imageio_encode(image,0,IMAGEIO_PNG_FILTER_UP,&status) && status);status=0;assert(!sweet_png_imageio_encode(image,2,123,&status) && status);
    assert([input isEqualToData:copy]);[fixture release];CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(color);
    puts("PNG image dispatch PASS: real RGB16 PNG/pixels/ICC/metadata, exact nil/throw/errno fallback, source immutability and unsupported class/orientation/scale/animation rejection; UIKit is a host fixture");
} return 0; }
