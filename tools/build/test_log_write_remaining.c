#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../../gpu/sweetgl/LogWriteAll.h"
static unsigned char captured[64];static size_t used;static unsigned calls,mode;
static ssize_t writer(int fd,const void *bytes,size_t n)
{
    assert(fd==17);++calls;
    if(mode==1 && calls<=3 || mode==2 && calls%2){errno=EINTR;return -1;}
    if(mode==3)return 0;
    if(mode==4){errno=EACCES;return -1;}
    if(mode==5){errno=EINTR;return -1;}
    if(mode==6)return (ssize_t)n+1;
    size_t take=mode==2 && n>3?3:n;assert(used+take<=sizeof(captured));memcpy(captured+used,bytes,take);used+=take;return (ssize_t)take;
}
int main(void)
{
    uint8_t source[37];for(unsigned i=0;i<sizeof(source);++i)source[i]=(uint8_t)(i*7);
    for(mode=0;mode<3;++mode){used=calls=0;memset(captured,0xa5,sizeof(captured));assert(!sweet_log_write_remaining(17,source,sizeof(source),writer));assert(used==sizeof(source) && !memcmp(captured,source,sizeof(source)));
        for(unsigned i=sizeof(source);i<sizeof(captured);++i)assert(captured[i]==0xa5);}
    mode=3;assert(sweet_log_write_remaining(17,source,37,writer)==-EIO);
    mode=4;assert(sweet_log_write_remaining(17,source,37,writer)==-EACCES);
    mode=5;calls=0;assert(sweet_log_write_remaining(17,source,37,writer)==-EINTR && calls==65);
    mode=6;assert(sweet_log_write_remaining(17,source,37,writer)==-EIO);
    calls=0;assert(!sweet_log_write_remaining(17,NULL,0,writer) && !calls);
    assert(sweet_log_write_remaining(17,NULL,1,writer)==-EINVAL);assert(sweet_log_write_remaining(17,source,37,NULL)==-EINVAL);
    puts("Process log writer PASS cases=10 exact_remaining_bytes=1 EINTR_and_partial=1 bounded_interruption=1 device_operations=0");return 0;
}
