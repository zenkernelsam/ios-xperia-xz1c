/* SPDX-License-Identifier: BSD-2-Clause; pace each authored range exactly once. */
#pragma once
#include <errno.h>
#include <stdbool.h>
#include <time.h>
enum { CIF10_EXPORT_PAUSE_NS=15000000, CIF10_EXPORT_MAX_INTERRUPTS=64 };
typedef int (*CIF10ExportSleep)(const struct timespec *,struct timespec *);
static inline bool cif10_export_wait(CIF10ExportSleep sleep_fn)
{
    const int saved=errno;bool complete=false;
    struct timespec request={0,CIF10_EXPORT_PAUSE_NS},remaining={0,0};
    if(sleep_fn)for(unsigned interrupted=0;;++interrupted) {
        const int result=sleep_fn(&request,&remaining);
        if(result==0){complete=true;break;}
        if(result!=-1 || errno!=EINTR || interrupted>=CIF10_EXPORT_MAX_INTERRUPTS ||
           remaining.tv_sec!=0 || remaining.tv_nsec<0 || remaining.tv_nsec>request.tv_nsec)break;
        if(!remaining.tv_nsec){complete=true;break;}
        request=remaining;
    }
    errno=saved;return complete;
}
