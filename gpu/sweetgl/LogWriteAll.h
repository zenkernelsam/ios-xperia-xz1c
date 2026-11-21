/* SPDX-License-Identifier: BSD-2-Clause; bounded progress for process log writes. */
#pragma once
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
typedef ssize_t (*SweetLogWriter)(int,const void *,size_t);
static inline int sweet_log_write_remaining(int fd,const void *source,size_t bytes,SweetLogWriter writer)
{
    if((!source && bytes) || !writer)return -EINVAL;
    const unsigned char *cursor=source;unsigned interrupted=0;
    while(bytes){ssize_t n=writer(fd,cursor,bytes);
        if(n>0){if((size_t)n>bytes)return -EIO;cursor+=(size_t)n;bytes-=(size_t)n;}
        else if(n<0 && errno==EINTR && interrupted++<64)continue;
        else return -(n<0 && errno?errno:EIO);
    }
    return 0;
}
