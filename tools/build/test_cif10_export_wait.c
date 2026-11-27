#include "../../gpu/diagnostics/CIF10ExportWait.h"
#include <assert.h>
#include <stdio.h>
static unsigned calls,mode;
static long remaining_ns;
static int sleeper(const struct timespec *request,struct timespec *remaining)
{
    assert(request->tv_sec==0 && request->tv_nsec>0 && request->tv_nsec<=CIF10_EXPORT_PAUSE_NS);
    ++calls;assert(request->tv_nsec==remaining_ns);
    if(mode==0 || (mode==1 && calls==4))return 0;
    *remaining=*request;errno=EINTR;
    if(mode==1)remaining->tv_nsec=remaining_ns-=1000000;
    if(mode==2)errno=EIO;
    if(mode==3)remaining->tv_nsec=request->tv_nsec+1;
    if(mode==4)remaining->tv_sec=1;
    if(mode==5)remaining->tv_nsec=-1;
    if(mode==6)remaining->tv_nsec=0;
    if(mode==8)return 1;
    return -1;
}
int main(void)
{
    for(mode=0;mode<=8;++mode) {
        calls=0;remaining_ns=CIF10_EXPORT_PAUSE_NS;errno=ENOTTY;
        const bool success=cif10_export_wait(sleeper);
        assert(success==(mode==0 || mode==1 || mode==6) && errno==ENOTTY);
        if(mode==1)assert(calls==4);
        if(mode==7)assert(calls==CIF10_EXPORT_MAX_INTERRUPTS+1);
    }
    errno=ENOTTY;assert(!cif10_export_wait(NULL) && errno==ENOTTY);
    puts("CIF10 export pacing PASS: remaining-time EINTR, bounded failures and errno preserved; no duplicate export");
    return 0;
}
