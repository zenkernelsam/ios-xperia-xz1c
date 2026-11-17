/* Scoped UIImage PNG compatibility. Unsupported cases keep the original API. */
#pragma once
#include "PNGImageIOEncoder.h"
#include <objc/runtime.h>
#include <errno.h>
#include <string.h>

@protocol SweetPNGDispatchImage
- (CGImageRef)CGImage;
- (CGSize)size;
- (CGFloat)scale;
- (NSInteger)imageOrientation;
- (NSArray *)images;
@end
typedef NSData *(*SweetPNGOriginal)(id);
typedef NSData *(*SweetPNGEncoder)(CGImageRef,CGFloat,uint32_t,int *);
enum { SWEET_PNG_ORIGINAL=0,SWEET_PNG_UP=1,SWEET_PNG_FAILED_FALLBACK=2,SWEET_PNG_THROW_FALLBACK=3 };
static bool sweet_png_getter(Class type,SEL selector,const char *encoding)
{
    Method method=type?class_getInstanceMethod(type,selector):NULL;char actual[96]={0};
    if(method)method_getReturnType(method,actual,sizeof(actual));
    return method && method_getNumberOfArguments(method)==2 && !strcmp(actual,encoding);
}
static bool sweet_png_image_eligible(id image,Class expected,CGImageRef *cg,CGFloat *scale)
{
    int saved_errno=errno;bool valid=false;
    if(cg)*cg=NULL;if(scale)*scale=0;
    Class type=image?object_getClass(image):Nil;
    if(!type || type!=expected || !cg || !scale){errno=saved_errno;return false;}
    if(!sweet_png_getter(type,@selector(CGImage),@encode(CGImageRef)) ||
       !sweet_png_getter(type,@selector(size),@encode(CGSize)) ||
       !sweet_png_getter(type,@selector(scale),@encode(CGFloat)) ||
       !sweet_png_getter(type,@selector(imageOrientation),@encode(NSInteger)) ||
       !sweet_png_getter(type,@selector(images),@encode(id))){errno=saved_errno;return false;}
    @try {
        id<SweetPNGDispatchImage> original=(id<SweetPNGDispatchImage>)image;
        CGImageRef value=[original CGImage];CGFloat value_scale=[original scale];
        valid=[original images]==nil && sweet_png_cif10_contract(value,[original size],value_scale,[original imageOrientation]);
        if(valid){*cg=value;*scale=value_scale;}
    } @catch(NSException *exception){(void)exception;valid=false;}
    errno=saved_errno;return valid;
}
static NSData *sweet_png_dispatch(id image,SweetPNGOriginal original,SweetPNGEncoder encoder,
                                  bool eligible,CGImageRef cg,CGFloat scale,int *route)
{
    int incoming_errno=errno;int path=SWEET_PNG_ORIGINAL;NSData *result=nil;
    if(eligible) {
        @try {
            int status=0;result=encoder(cg,scale,IMAGEIO_PNG_FILTER_UP,&status);
            if(!status && result && result.length){path=SWEET_PNG_UP;if(route)*route=path;errno=incoming_errno;return result;}
            path=SWEET_PNG_FAILED_FALLBACK;
        } @catch(NSException *exception){(void)exception;path=SWEET_PNG_THROW_FALLBACK;}
    }
    if(route)*route=path;
    errno=incoming_errno;return original(image);
}
