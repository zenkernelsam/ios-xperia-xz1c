/* Compile and exercise the actual reader; fixtures are retained for recycling. */
#define _GNU_SOURCE 1
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
static unsigned reads;
static int fault;
static int fifo_fd=-1;
static int checked_openat(int parent,const char *name,int flags)
{
    if(fault==3 && !strcmp(name,".com.apple.mobile_container_manager.metadata.plist")){
        assert((flags&O_ACCMODE)==O_RDONLY && (flags&O_NONBLOCK) && (flags&O_NOFOLLOW));
        return dup(fifo_fd);
    }
    return openat(parent,name,flags);
}
static ssize_t checked_read(int fd,void *p,size_t n)
{
    assert((fcntl(fd,F_GETFL)&O_ACCMODE)==O_RDONLY);++reads;
    if(fault==1){fault=0;errno=EINTR;return -1;}
    if(fault==2){errno=EIO;return -1;}
    return read(fd,p,n);
}
#define read checked_read
#define openat checked_openat
#include "../../gpu/diagnostics/PosterContainerSnapshot.h"
#undef read
#undef openat
static void logger(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>0 && n<(int)sizeof(line));puts(line);
}
int main(int argc,char **argv)
{
    assert(argc==6);fault=atoi(argv[1]);
    if(fault==3){int pair[2];assert(pipe(pair)==0);fifo_fd=pair[0];close(pair[1]);
        assert(fcntl(fifo_fd,F_SETFL,O_NONBLOCK)==0);}
    errno=ENOTTY;
    const char *roots[]={argv[2],argv[3],argv[4],argv[5]};
    struct SweetPosterContainerSummary s=sweet_poster_container_snapshot_at(roots,4,logger,NULL);
    assert(errno==ENOTTY);
    if(fifo_fd>=0){assert(reads==0);close(fifo_fd);}
    printf("HOST PCON result=%d files=%u errors=%u reads=%u\n",s.result,s.files,s.errors,reads);
    return 0;
}
