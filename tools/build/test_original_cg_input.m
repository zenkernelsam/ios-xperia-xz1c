/* SPDX-License-Identifier: BSD-2-Clause; original CIF10 provider bytes. */
#include "../../gpu/diagnostics/OriginalCGInputObserve.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
static unsigned records;
static void emit(const char *format,...)
{
    char text[256];va_list args;va_start(args,format);
    int size=vsnprintf(text,sizeof(text),format,args);va_end(args);assert(size>0 && size<190);
    puts(text);++records;errno=111;
}
int main(void)
{
    @autoreleasepool {
        uint32_t pixels[6]={0xd8060180,0xffffffff,0xd8060180,0,0xd8060180,0xd8060180},saved[6];
        memcpy(saved,pixels,sizeof(pixels));CFDataRef data=CFDataCreate(NULL,(const UInt8 *)pixels,sizeof(pixels));
        CGDataProviderRef provider=CGDataProviderCreateWithCFData(data);
        CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);
        CGImageRef image=CGImageCreate(3,2,10,32,12,color,(CGBitmapInfo)0x42000,provider,NULL,false,kCGRenderingIntentDefault);
        assert(image);
        puts("SWEET LOOP LIVE proto=1 token=123456789abcdef0");
        puts("SWEET POSTDRAW BEGIN pid=7 tid=1 call=15 root=15 depth=1 api=png at_ns=1 original_forwarded=0");
        errno=73;assert(snapshot_cg_input_once(image,7,15,emit) && errno==73);
        assert(!snapshot_cg_input_once(image,7,16,emit) && records==7 && !memcmp(saved,pixels,sizeof(pixels)));
        puts("SWEET LOOP ACK FASTBOOT token=123456789abcdef0");
        CGImageRelease(image);CGColorSpaceRelease(color);CGDataProviderRelease(provider);CFRelease(data);
        puts("Original CGImage input PASS: original CIF10 provider bytes/FNV/counts, one observation, errno and guards preserved; no pixel conversion or GPU draw");
    }
    return 0;
}
