/* Authored page framing, bounds and source immutability; no device access. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef void (*MetalABIEmit)(const char *,...);
#include "../../gpu/diagnostics/AttachmentPageSnapshot.h"
static unsigned lines,blocks,offset;
static const unsigned char *original;
static unsigned original_bytes;
static void emit(const char *format,...)
{
    char line[256];va_list args;va_start(args,format);int count=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(count>0 && count<=190 && (unsigned)count<sizeof(line));++lines;
    if(strstr(line," DATA ")){unsigned phase,slot,block,at,n;char hex[49]={0};
        assert(sscanf(line,"SWEET ATTACH SNAP DATA suite=msaa phase=%u slot=%u block=%u offset=%u n=%u hex=%48s",&phase,&slot,&block,&at,&n,hex)==6);
        assert(phase==17 && slot==11 && block==blocks && at==offset && n>0 && n<=24 && strlen(hex)==n*2 && at+n<=original_bytes);
        for(unsigned i=0;i<n;++i){unsigned value=0;assert(sscanf(hex+i*2,"%2x",&value)==1 && value==original[at+i]);}
        ++blocks;offset+=n;
    }
    puts(line);
}
int main(void)
{
    unsigned char bytes[4096],saved[4096];for(unsigned i=0;i<sizeof(bytes);++i)bytes[i]=(unsigned char)(i*17+3);
    memcpy(saved,bytes,sizeof(bytes));original=bytes;original_bytes=sizeof(bytes);
    attachment_page_snapshot(emit,"msaa",17,11,bytes,sizeof(bytes),UINT64_C(0x40004000),123);
    assert(offset==4096 && blocks==171 && lines==blocks+2 && !memcmp(bytes,saved,sizeof(bytes)));
    unsigned before=lines;
    attachment_page_snapshot(emit,"UI",17,11,bytes,sizeof(bytes),UINT64_C(0x40004000),123);
    attachment_page_snapshot(emit,"msaa",17,11,bytes,4097,UINT64_C(0x40004000),123);
    attachment_page_snapshot(emit,"msaa",17,11,bytes,0,UINT64_C(0x40004000),123);
    attachment_page_snapshot(emit,"msaa",17,11,bytes,24,UINT64_C(0x40004001),123);
    attachment_page_snapshot(emit,"msaa",17,11,bytes,24,UINT64_C(0x40004000),0);
    assert(before==lines && !memcmp(bytes,saved,sizeof(bytes)));
    puts("Attachment page snapshot host PASS: immutable_author_page=1 max_bytes=4096 max_line=190 invalid_no_reads=1");
    return 0;
}
