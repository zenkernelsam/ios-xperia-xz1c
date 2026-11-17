/* Author-generated CIF10 inputs: real ImageIO timings and lossless comparisons.
 * No UIKit/phone APIs, image substitutions or compression options are installed.
 */
#import <Foundation/Foundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <CommonCrypto/CommonDigest.h>
#include <mach/mach_time.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mach_timebase_info_data_t timebase;
static uint64_t wall_ns(void)
{ return (uint64_t)(((__uint128_t)mach_absolute_time()*timebase.numer)/timebase.denom); }
static uint64_t cpu_ns(void)
{ struct timespec value={0};assert(!clock_gettime(CLOCK_THREAD_CPUTIME_ID,&value));return (uint64_t)value.tv_sec*1000000000+value.tv_nsec; }
static NSString *sha(NSData *data)
{
    if(!data)return @"none";
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(data.bytes,(CC_LONG)data.length,digest);
    char hex[65];for(unsigned i=0;i<32;++i)sprintf(hex+i*2,"%02x",digest[i]);return [NSString stringWithUTF8String:hex];
}
typedef struct { uint64_t create,add,finalize,cpu; } Timing;
static NSData *encode(CGImageRef image,Timing *timing)
{
    NSMutableData *png=[NSMutableData data];uint64_t cpu=cpu_ns(),begin=wall_ns();
    CGImageDestinationRef destination=CGImageDestinationCreateWithData((CFMutableDataRef)png,CFSTR("public.png"),1,NULL);
    assert(destination);timing->create=wall_ns()-begin;begin=wall_ns();
    CGImageDestinationAddImage(destination,image,NULL);timing->add=wall_ns()-begin;begin=wall_ns();
    assert(CGImageDestinationFinalize(destination));timing->finalize=wall_ns()-begin;timing->cpu=cpu_ns()-cpu;
    CFRelease(destination);assert(png.length);return png;
}
static NSDictionary *timing_dict(Timing timing)
{ return @{@"create_ns":@(timing.create),@"add_image_ns":@(timing.add),@"finalize_ns":@(timing.finalize),@"thread_cpu_ns":@(timing.cpu)}; }
static CGImageRef create(NSData *pixels,size_t width,size_t height,unsigned bits)
{
    bool cif10=bits==10;assert(cif10 || bits==8 || bits==16);
    CGColorSpaceRef color=CGColorSpaceCreateWithName(cif10?kCGColorSpaceExtendedSRGB:kCGColorSpaceSRGB);
    CGDataProviderRef provider=CGDataProviderCreateWithCFData((CFDataRef)pixels);assert(color && provider);
    CGBitmapInfo bitmap=cif10?((CGBitmapInfo)kCGImagePixelFormatRGBCIF10|(CGBitmapInfo)kCGImageByteOrder32Little):
        ((CGBitmapInfo)kCGImageAlphaNone|(bits==16?(CGBitmapInfo)kCGImageByteOrder16Little:0));
    CGImageRef image=CGImageCreate(width,height,bits,cif10?32:bits*3,width*(cif10?4:3*(bits/8)),color,bitmap,provider,NULL,false,kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);CGColorSpaceRelease(color);assert(image);return image;
}
static uint16_t prediction(uint32_t code,unsigned bits)
{
    // This is a hypothesis, never an installed conversion. Every byte/profile
    // must match real ImageIO before it can inform a lossless implementation.
    if(code<=384)return 0;if(code>=894)return bits==8?255:65535;
    unsigned product=(code-384)*(bits==8?1:257),value=product>>1;return (uint16_t)(value+((product&1u)&&(value&1u)));
}
typedef struct {
    CGImageSourceRef source;
    CGImageRef image;
    CFDictionaryRef properties;
    CFDataRef icc,pixels;
} Decoded;
static Decoded decode(NSData *png)
{
    Decoded value={0};value.source=CGImageSourceCreateWithData((CFDataRef)png,NULL);assert(value.source);
    value.properties=CGImageSourceCopyPropertiesAtIndex(value.source,0,NULL);assert(value.properties);
    value.image=CGImageSourceCreateImageAtIndex(value.source,0,NULL);assert(value.image);
    value.icc=CGColorSpaceCopyICCData(CGImageGetColorSpace(value.image));
    value.pixels=CGDataProviderCopyData(CGImageGetDataProvider(value.image));assert(value.pixels);return value;
}
static void release_decoded(Decoded value)
{ if(value.icc)CFRelease(value.icc);CFRelease(value.pixels);CGImageRelease(value.image);CFRelease(value.properties);CFRelease(value.source); }
static NSDictionary *image_dict(CGImageRef image)
{
    CFStringRef color=CGColorSpaceCopyName(CGImageGetColorSpace(image));
    NSDictionary *result=@{@"width":@(CGImageGetWidth(image)),@"height":@(CGImageGetHeight(image)),
        @"bpc":@(CGImageGetBitsPerComponent(image)),@"bpp":@(CGImageGetBitsPerPixel(image)),@"pitch":@(CGImageGetBytesPerRow(image)),
        @"bitmap_info":@(CGImageGetBitmapInfo(image)),@"alpha":@(CGImageGetAlphaInfo(image)),@"color_name":color?(id)color:@"unnamed"};
    if(color)CFRelease(color);return result;
}
static NSArray *bytes_prefix(CFDataRef data)
{
    NSMutableArray *result=[NSMutableArray array];const uint8_t *bytes=CFDataGetBytePtr(data);
    for(CFIndex i=0;i<CFDataGetLength(data) && i<24;++i)[result addObject:@(bytes[i])];return result;
}
static NSDictionary *probe(size_t width,size_t height,unsigned pattern,uint32_t top,const char *label,unsigned bits)
{
    NSMutableData *raw=[NSMutableData dataWithLength:width*height*4];uint8_t *source=raw.mutableBytes;
    for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x) {
        unsigned i=(unsigned)(y*width+x),r,g,b;
        if(!pattern) {r=i&1023;g=(i*17)&1023;b=(i*73)&1023;}
        else {r=(unsigned)(x*17+y*3)&1023;g=(unsigned)(x*7+y*29)&1023;b=(unsigned)(x*31+y*11)&1023;}
        uint32_t word=top|(r<<20)|(g<<10)|b;memcpy(source+4*(y*width+x),&word,4);
    }
    NSString *input_sha=sha(raw);CGImageRef cif=create(raw,width,height,10);
    assert(CGImageGetBitsPerComponent(cif)==10 && CGImageGetBitsPerPixel(cif)==32 && CGImageGetBitmapInfo(cif)==0x42000);
    Timing original={0},candidate={0};NSData *original_png=[[encode(cif,&original) copy] autorelease];
    uint64_t conversion_begin=wall_ns();NSMutableData *rgb=[NSMutableData dataWithLength:width*height*3*(bits/8)];uint8_t *dest=rgb.mutableBytes;
    for(size_t i=0;i<width*height;++i) {
        uint32_t word;memcpy(&word,source+4*i,4);
        uint16_t values[3]={prediction((word>>20)&1023,bits),prediction((word>>10)&1023,bits),prediction(word&1023,bits)};
        if(bits==16)memcpy(dest+6*i,values,sizeof(values));
        else for(unsigned c=0;c<3;++c)dest[3*i+c]=(uint8_t)values[c];
    }
    uint64_t conversion_ns=wall_ns()-conversion_begin;CGImageRef standard=create(rgb,width,height,bits);
    NSData *candidate_png=[[encode(standard,&candidate) copy] autorelease];
    assert([sha(raw) isEqualToString:input_sha]);Decoded a=decode(original_png),b=decode(candidate_png);
    bool dimensions=CGImageGetWidth(a.image)==width && CGImageGetHeight(a.image)==height && CGImageGetWidth(b.image)==width && CGImageGetHeight(b.image)==height;
    assert(dimensions);
    bool representation=CGImageGetBitsPerComponent(a.image)==CGImageGetBitsPerComponent(b.image) &&
        CGImageGetBitsPerPixel(a.image)==CGImageGetBitsPerPixel(b.image) && CGImageGetBytesPerRow(a.image)==CGImageGetBytesPerRow(b.image) &&
        CGImageGetBitmapInfo(a.image)==CGImageGetBitmapInfo(b.image);
    bool pixels=representation && CFEqual(a.pixels,b.pixels),metadata=CFEqual(a.properties,b.properties);
    bool icc=(a.icc && b.icc)?CFEqual(a.icc,b.icc):a.icc==b.icc;
    bool png_bytes=[original_png isEqualToData:candidate_png];
    uint64_t differences=0,first=UINT64_MAX;
    if(CFDataGetLength(a.pixels)==CFDataGetLength(b.pixels)) {
        const uint8_t *ap=CFDataGetBytePtr(a.pixels),*bp=CFDataGetBytePtr(b.pixels);
        for(CFIndex i=0;i<CFDataGetLength(a.pixels);++i)if(ap[i]!=bp[i]){if(first==UINT64_MAX)first=(uint64_t)i;++differences;}
    }
    NSDictionary *result=@{@"label":[NSString stringWithUTF8String:label],@"candidate_bpc":@(bits),@"CIF10_documented_top_bits":@(top==0xc0000000),@"input":image_dict(cif),
        @"input_sha256":input_sha,@"input_unchanged":@YES,@"original_ImageIO_timing":timing_dict(original),@"candidate_ImageIO_timing":timing_dict(candidate),
        @"candidate_unpack_ns":@(conversion_ns),@"original_PNG_bytes":@(original_png.length),@"candidate_PNG_bytes":@(candidate_png.length),
        @"original_PNG_sha256":sha(original_png),@"candidate_PNG_sha256":sha(candidate_png),
        @"original_decoded":image_dict(a.image),@"candidate_decoded":image_dict(b.image),
        @"original_metadata":[NSJSONSerialization isValidJSONObject:(id)a.properties]?(id)a.properties:[(id)a.properties description],
        @"candidate_metadata":[NSJSONSerialization isValidJSONObject:(id)b.properties]?(id)b.properties:[(id)b.properties description],
        @"original_decoded_bytes":@(CFDataGetLength(a.pixels)),@"candidate_decoded_bytes":@(CFDataGetLength(b.pixels)),
        @"original_decoded_prefix":bytes_prefix(a.pixels),@"candidate_decoded_prefix":bytes_prefix(b.pixels),
        @"decoded_raw_representation_equal":@(representation),@"decoded_raw_pixels_equal":@(pixels),@"metadata_equal":@(metadata),@"ICC_equal":@(icc),
        @"original_ICC_sha256":sha((NSData *)a.icc),@"candidate_ICC_sha256":sha((NSData *)b.icc),@"PNG_exact_bytes_equal":@(png_bytes),
        @"decoded_byte_differences_if_equal_length":@(differences),@"first_difference_if_present":@(first),
        @"eligible_for_byte_preserving_optimization":@(top==0xc0000000 && pixels && metadata && icc && png_bytes)};
    release_decoded(a);release_decoded(b);CGImageRelease(cif);CGImageRelease(standard);return result;
}
int main(int argc,char **argv) { @autoreleasepool {
    unsigned bits=argc==2?(unsigned)atoi(argv[1]):16;assert(argc<=2 && (bits==8 || bits==16));
    assert(!mach_timebase_info(&timebase) && timebase.numer && timebase.denom);
    NSArray *cases=@[probe(1024,3,0,0xc0000000,"all_codes_mixed",bits),probe(256,256,1,0xc0000000,"mixed_cube",bits),
        probe(750,1624,1,0xc0000000,"screen_contract",bits),probe(1024,3,0,0,"undocumented_zero_top_bits",bits)];
    NSDictionary *report=@{@"revision":@1,@"host_only":@YES,@"UIImage_API_exercised":@NO,@"phone_operated":@NO,
        @"inputs_author_generated":@YES,@"ImageIO_original_create_add_finalize_options_NULL":@YES,
        @"hypothesis":@"clamp((code-384)*(candidate_bpc==16?257:1)/2), ties to even; source BGR CIF10 to RGB sRGB",
        @"candidate_not_installed":@YES,@"host_result_does_not_admit_device_optimization":@YES,@"cases":cases};
    NSError *error=nil;NSData *json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:&error];
    assert(json && !error);fwrite(json.bytes,1,json.length,stdout);putchar('\n');
} return 0; }
