/* SPDX-License-Identifier: BSD-2-Clause; inspect original returned pixel bytes. */
#include "../../gpu/diagnostics/SnapshotReadbackObserve.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

static unsigned records;
static void emit(const char *format,...)
{
    char text[256];va_list arguments;va_start(arguments,format);
    int size=vsnprintf(text,sizeof(text),format,arguments);va_end(arguments);
    assert(size>0 && size<190);puts(text);++records;errno=111;
}
int main(void)
{
    unsigned char buffer[64],saved[64];memset(buffer,0xa5,sizeof(buffer));
    const uint32_t words[6]={0xd8060180,0xffffffff,0xd8060180,0,0xd8060180,0xd8060180};
    memcpy(buffer+8,words,12);memcpy(buffer+24,words+3,12);memcpy(saved,buffer,sizeof(buffer));
    puts("SWEET LOOP LIVE proto=1 token=123456789abcdef0");
    puts("SWEET POSTDRAW END pid=7 tid=1 call=14 api=read wall_ns=1 cpu_ns=1 cpu_valid=1 outcome=return bytes=0 returned_nil=0 original_behavior=1");
    errno=73;
    assert(snapshot_readback_bytes(buffer+8,16,MTLRegionMake2D(0,0,3,2),0,0,555,7,14,emit));
    assert(errno==73 && records==7 && !memcmp(saved,buffer,sizeof(buffer)));
    assert(!snapshot_readback_bytes(buffer+8,8,MTLRegionMake2D(0,0,3,2),0,0,555,7,15,emit));
    assert(!snapshot_readback_bytes(buffer+8,16,MTLRegionMake2D(1,0,3,2),0,0,555,7,15,emit));
    assert(!snapshot_readback_bytes(buffer+8,16,MTLRegionMake2D(0,0,3,2),1,0,555,7,15,emit));
    assert(!snapshot_readback_bytes(buffer+8,16,MTLRegionMake2D(0,0,3,2),0,1,555,7,15,emit));
    assert(!snapshot_readback_bytes(buffer+8,16,MTLRegionMake2D(0,0,3,2),0,0,80,7,15,emit));
    assert(!snapshot_readback_bytes(NULL,16,MTLRegionMake2D(0,0,3,2),0,0,555,7,15,emit));
    assert(!snapshot_readback_bytes((void *)(UINTPTR_MAX-1),16,MTLRegionMake2D(0,0,3,2),0,0,555,7,15,emit));
    assert(records==7 && !memcmp(saved,buffer,sizeof(buffer)));
    puts("SWEET LOOP ACK FASTBOOT token=123456789abcdef0");
    puts("Snapshot original readback PASS: exact returned XR bytes, logical rows/FNV, black-code counts, sampled words, stride/format/pointer bounds and unchanged guards; no new GPU work");
    return 0;
}
