/* Verify the real bounded state cache and exact original scalar bits. */
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/PosterStateObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static unsigned values,bits,limits,configs;
static void emit(const char *format,...)
{
    char line[256];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>=0 && n<=190);
    if(strstr(line,"SWEET PST VALUE"))++values;
    if(strstr(line,"SWEET PST BITS"))++bits;
    if(strstr(line,"SWEET PST LIMIT"))++limits;
    if(strstr(line,"SWEET PST CONFIG"))++configs;
    errno=99;
}
static void reset(void)
{
    memset(poster_state_slots,0,sizeof(poster_state_slots));poster_state_used=poster_state_records=0;
    atomic_flag_clear(&poster_state_guard);atomic_store(&poster_state_limit_flags,0);values=bits=limits=configs=0;
}
int main(void) { @autoreleasepool {
    reset();errno=73;poster_state_observe(emit);assert(configs==1 && errno==73);
    for(unsigned i=0;i<100;++i){errno=73;poster_state_value(PST_PREVIEW,0x100,'B',1,0,true);assert(errno==73);}
    assert(values==bits && values==1 && poster_state_records==1 && poster_state_used==1);
    poster_state_value(PST_PREVIEW,0x200,'B',1,0,true);assert(values==2 && poster_state_used==2);
    poster_state_value(PST_PREVIEW,0x100,'B',0,0,true);assert(values==3 && poster_state_used==2);
    poster_state_value(PST_PREVIEW,0x100,'B',0,0,false);assert(values==4);
    poster_state_value(PST_PREVIEW,0x100,'B',0,0,false);assert(values==4);
    poster_state_value(PST_BACKLIGHT,0x100,'D',UINT64_C(0x7ff8000000001234),0,true);
    assert(poster_state_slots[2].first==UINT64_C(0x7ff8000000001234));
    poster_state_value(PST_BACKLIGHT,0x100,'D',UINT64_C(0x7ff8000000005678),0,true);assert(values==6);
    poster_state_value(PST_MIN_SIZE,0x100,'S',UINT64_C(0x3ff0000000000000),UINT64_C(0x8000000000000000),true);
    assert(poster_state_slots[3].second==UINT64_C(0x8000000000000000));
    poster_state_value(PST_ENV_UPDATE,0x100,'E',0x123,0x456,true);
    assert(values==8 && bits==8 && poster_state_used==5);
    unsigned before=values;poster_state_value(PST_API_COUNT,0x100,'B',0,0,true);poster_state_value(PST_PREVIEW,0,'B',0,0,true);assert(values==before);
    atomic_flag_test_and_set(&poster_state_guard);errno=73;poster_state_value(PST_PREVIEW,0x300,'B',0,0,true);assert(errno==73 && limits==1);poster_state_value(PST_PREVIEW,0x300,'B',0,0,true);assert(limits==1);atomic_flag_clear(&poster_state_guard);
    reset();
    for(unsigned i=0;i<PST_SLOTS;++i)poster_state_value(PST_SNAPSHOT,0x100+i,'B',1,0,true);
    assert(values==PST_SLOTS && poster_state_used==PST_SLOTS);
    poster_state_value(PST_SNAPSHOT,0x999,'B',1,0,true);assert(values==PST_SLOTS && limits==1);
    reset();
    for(unsigned i=0;i<PST_MAX_RECORDS;++i)poster_state_value(PST_UNLOCK,0x100,'D',i,0,true);
    assert(values==PST_MAX_RECORDS && poster_state_used==1);
    poster_state_value(PST_UNLOCK,0x100,'D',PST_MAX_RECORDS,0,true);assert(values==PST_MAX_RECORDS && limits==1);
    puts("Poster state observer PASS: original bits, same-value coalescing, new receiver/state, throw/errno, nonblocking busy and slot/record limits");
} return 0; }
