/* Encoder/timing composition with an explicit UIKit host fixture. */
#define LILAC_POSTDRAW_TEST 1
#define SWEET_PNG_FILTER_ENCODER 1
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/PostDrawObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

@interface UIImage : NSObject <SweetPNGDispatchImage> { @public CGImageRef cg;CGFloat imageScale;NSInteger orientation; } @end
@implementation UIImage
- (CGImageRef)CGImage {return cg;}
- (CGSize)size {return CGSizeMake(CGImageGetWidth(cg)/imageScale,CGImageGetHeight(cg)/imageScale);}
- (CGFloat)scale {return imageScale;}
- (NSInteger)imageOrientation {return orientation;}
- (NSArray *)images {return nil;}
@end
static id expected;static NSData *originalAnswer;static unsigned originals,optimized,forwarded;static bool originalThrows;
static NSException *originalException;
NSData *UIImagePNGRepresentation(id image)
{assert(image==expected && errno==73);++originals;errno=91;if(originalThrows)@throw originalException;return originalAnswer;}
static void emit(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int size=vsnprintf(line,sizeof(line),format,args);va_end(args);assert(size>0 && size<250);
    if(strstr(line,"PNG_ROUTE") && strstr(line,"route=ImageIO_UP"))++optimized;
    if(strstr(line,"POSTDRAW BEGIN") && strstr(line,"api=png") && strstr(line,"original_forwarded=1"))++forwarded;
    errno=117;
}
int main(void) { @autoreleasepool {
    NSMutableData *raw=[NSMutableData dataWithLength:64*16*4];uint32_t value=0xc0000000|(640u<<20)|(510u<<10)|384u;
    for(size_t i=0;i<64*16;++i)memcpy((uint8_t *)raw.mutableBytes+i*4,&value,4);
    CGDataProviderRef provider=CGDataProviderCreateWithCFData((CFDataRef)raw);CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);
    CGImageRef image=CGImageCreate(64,16,10,32,256,color,(CGBitmapInfo)kCGImagePixelFormatRGBCIF10|(CGBitmapInfo)kCGImageByteOrder32Little,provider,NULL,false,kCGRenderingIntentDefault);assert(image);
    UIImage *fixture=[UIImage new];fixture->cg=image;fixture->imageScale=2;expected=fixture;originalAnswer=[NSData dataWithBytes:"original" length:8];
    errno=73;assert(postdraw_observe(emit));errno=73;NSData *png=postdraw_png(fixture);
    assert(png.length && !originals && optimized==1 && !postdraw_depth && !postdraw_root && errno==73);
    fixture->orientation=1;errno=73;assert(postdraw_png(fixture)==originalAnswer && originals==1 && forwarded==1 && errno==91);
    expected=nil;originalAnswer=nil;errno=73;assert(postdraw_png(nil)==nil && originals==2 && forwarded==2 && errno==91);
    originalException=[NSException exceptionWithName:@"OriginalPNGThrow" reason:@"identity" userInfo:nil];originalThrows=true;bool caught=false;errno=73;
    @try{(void)postdraw_png(nil);}@catch(NSException *exception){assert(exception==originalException);caught=true;}
    assert(caught && originals==3 && errno==91 && !postdraw_depth && !postdraw_root);
    [fixture release];CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(color);
    puts("PNG postdraw encoder composition PASS: real encoded PNG, truthful optimized/original routes, untouched nil/throw/errno and bounded nesting; UIKit is a host fixture");
} return 0; }
