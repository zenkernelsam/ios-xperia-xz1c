/* SPDX-License-Identifier: BSD-2-Clause */
#include "../../gpu/diagnostics/OriginalSnapshotPNGExport.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static NSData *expected;
static unsigned begins,ends,blocks,seen;
static uint8_t compressed[1024];
static void emit(const char *format,...)
{
    char line[320];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>0 && n<=190);
    if(strstr(line,"SWEET OPNG BEGIN "))++begins;
    if(strstr(line,"SWEET OPNG END "))++ends;
    if(strstr(line,"SWEET OPNG DATA ")) {
        unsigned pid,call,offset,bytes;char hex[97]={0};
        assert(sscanf(line,"SWEET OPNG DATA pid=%u call=%u offset=%u bytes=%u hex=%96s",&pid,&call,&offset,&bytes,hex)==5);
        assert(pid==7 && call==9 && offset==seen && bytes<=48 && strlen(hex)==2*bytes);
        for(unsigned i=0;i<bytes;++i){unsigned value;assert(sscanf(hex+2*i,"%2x",&value)==1);assert(offset+i<sizeof(compressed));compressed[offset+i]=(uint8_t)value;}
        seen+=bytes;++blocks;
    }
    puts(line);errno=117;
}
int main(void)
{
    @autoreleasepool {
        uint8_t bytes[121]={137,80,78,71,13,10,26,10};
        for(unsigned i=8;i<sizeof(bytes);++i)bytes[i]=(uint8_t)(i*7);
        expected=[NSData dataWithBytes:bytes length:sizeof(bytes)];
        errno=73;assert(original_png_export(expected,7,9,emit));assert(errno==73);
        assert(begins==1 && ends==1 && blocks>0 && !memcmp(expected.bytes,bytes,sizeof(bytes)));
        uint8_t restored[121]={0};uLongf restored_bytes=sizeof(restored);
        assert(uncompress(restored,&restored_bytes,compressed,seen)==Z_OK && restored_bytes==sizeof(bytes) && !memcmp(restored,bytes,sizeof(bytes)));
        NSData *invalid=[NSData dataWithBytes:"invalid" length:7];
        assert(!original_png_export(invalid,7,9,emit));
        assert(!original_png_export([NSMutableData dataWithLength:65537],7,9,emit));
        assert(!original_png_export(expected,0,9,emit));
        puts("original PNG export PASS: unchanged returned bytes, finite chunks, signature/size refusal, original errno preserved");
    }
    return 0;
}
