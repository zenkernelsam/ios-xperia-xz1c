/* Host forwarding fixture; real ImageIO, not a test of iOS's UIImage API. */
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/CIF10PNGReferenceProbe.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

@interface CIF10HostImage : NSObject { @public CGImageRef image; } @end
@implementation CIF10HostImage
- (void)dealloc {if(image)CGImageRelease(image);[super dealloc];}
@end
static unsigned originals,complete,records,comparisons;static bool fail_nil,fail_throw;
id cif10_reference_image(CGImageRef image)
{ CIF10HostImage *value=[CIF10HostImage new];value->image=CGImageRetain(image);return [value autorelease]; }
NSData *cif10_reference_png(id wrapper)
{
    ++originals;if(fail_throw)@throw [NSException exceptionWithName:@"PNGReferenceFixture" reason:@"kept as failed probe" userInfo:nil];
    if(fail_nil)return nil;
    CIF10HostImage *value=wrapper;NSMutableData *data=[NSMutableData data];
    CGImageDestinationRef destination=CGImageDestinationCreateWithData((CFMutableDataRef)data,CFSTR("public.png"),1,NULL);assert(destination);
    CGImageDestinationAddImage(destination,value->image,NULL);assert(CGImageDestinationFinalize(destination));CFRelease(destination);return data;
}
static void emit(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int size=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(size>0 && size<250);++records;if(strstr(line,"SWEET CIF10 CASE END "))++complete;
    if(strstr(line,"SWEET CIF10 VIMAGE case="))++comparisons;
    if(strstr(line,"SWEET CIF10 VIMAGE ") || strstr(line,"SWEET CIF10 REFERENCE END "))fprintf(stderr,"%s\n",line);
}
int main(void) { @autoreleasepool {
    assert(cif10_png_reference_probe(emit));assert(originals==5 && complete==5 && records>500 && comparisons==10);
    fail_nil=true;assert(!cif10_png_reference_probe(emit));assert(originals==6 && complete==5);fail_nil=false;
    fail_throw=true;assert(!cif10_png_reference_probe(emit));assert(originals==7 && complete==5);
    puts("CIF10 PNG reference PASS: bounded author buffers/exports, unchanged input, real ImageIO forward/decode, nil/throw failures kept; UIKit is a Mac fixture");
} return 0; }
