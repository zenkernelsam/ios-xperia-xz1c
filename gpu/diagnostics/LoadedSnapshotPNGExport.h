/* SPDX-License-Identifier: BSD-2-Clause
 * Copy one successfully loaded original Aegir cache file to the serial log. */
#pragma once
#include "OriginalSnapshotPNGExport.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

static atomic_bool loaded_snapshot_export_scheduled;
static bool loaded_snapshot_path(const char *path)
{
    const char *tail=NULL;
    const char *roots[]={"/var/mobile/Library/Caches/","/private/var/mobile/Library/Caches/"};
    if(!path)return false;
    for(unsigned i=0;i<2;++i)if(!strncmp(path,roots[i],strlen(roots[i])))tail=path+strlen(roots[i]);
    if(!tail)return false;
    if(!strncmp(tail,"sweet-aegir-",12)) {
        const char *slash=strchr(tail,'/');if(!slash || slash-tail!=44)return false;
        for(unsigned i=12;i<44;++i)if(!((tail[i]>='0' && tail[i]<='9') || (tail[i]>='a' && tail[i]<='f')))return false;
        tail=slash+1;
    }
    size_t length=strlen(tail);
    return length>10 && !strncmp(tail,"aegir-",6) && !strchr(tail,'/') &&
        !strstr(tail,"..") && !strcmp(tail+length-4,".png");
}
static NSData *loaded_snapshot_file(const char *path, const char *allowed_root, int *status)
{
    int incoming=errno,result=-EINVAL;NSData *answer=nil;
    size_t root_bytes=allowed_root?strlen(allowed_root):0;
    if(!path || !allowed_root || strlen(path)<=root_bytes || strncmp(path,allowed_root,root_bytes) || path[root_bytes]!='/' ||
        strstr(path+root_bytes+1,"..")){if(status)*status=result;errno=incoming;return nil;}
    int fd=open(path,O_RDONLY|O_NOFOLLOW);
    if(fd<0){if(status)*status=-errno;errno=incoming;return nil;}
    struct stat before,after;uint8_t *bytes=NULL;
    if(!fstat(fd,&before) && S_ISREG(before.st_mode) && before.st_size>=8 && before.st_size<=65536) {
        size_t size=(size_t)before.st_size,at=0;bytes=malloc(size);
        if(bytes) {
            while(at<size){ssize_t count=read(fd,bytes+at,size-at);if(count<0 && errno==EINTR)continue;if(count<=0)break;at+=(size_t)count;}
            uint8_t extra;
            if(at==size && read(fd,&extra,1)==0 && !fstat(fd,&after) &&
                before.st_dev==after.st_dev && before.st_ino==after.st_ino && before.st_size==after.st_size &&
                before.st_mtimespec.tv_sec==after.st_mtimespec.tv_sec && before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec &&
                !memcmp(bytes,"\x89PNG\r\n\x1a\n",8)) {
                answer=[NSData dataWithBytes:bytes length:size];result=0;
            }
        }
    }
    free(bytes);close(fd);if(status)*status=result;errno=incoming;return answer;
}
static void loaded_snapshot_png_export_once(id path, id image, unsigned pid, uint64_t call, SweetOriginalPNGEmit emit)
{
    int incoming=errno;
    @try {
        if(!image || ![path isKindOfClass:[NSString class]] || !emit || !pid || !call)return;
        const char *filename=[(NSString *)path fileSystemRepresentation];
        if(!loaded_snapshot_path(filename))return;
        bool expected=false;
        if(!atomic_compare_exchange_strong_explicit(&loaded_snapshot_export_scheduled,&expected,true,memory_order_acq_rel,memory_order_relaxed))return;
        NSString *retained=[path copy];id loaded=[image retain];
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0), ^{
            @autoreleasepool {
                const char *name=retained.fileSystemRepresentation;
                const char *root=!strncmp(name,"/private/",9)?"/private/var/mobile/Library/Caches":"/var/mobile/Library/Caches";
                int status=0;NSData *bytes=loaded_snapshot_file(name,root,&status);
                emit("SWEET LPNG SOURCE pid=%u call=%llu result=%d original_image=%016llx file_read_only=1",
                    pid,(unsigned long long)call,status,(unsigned long long)(uintptr_t)loaded);
                if(!status)original_png_export(bytes,pid,call,emit);
                [loaded release];[retained release];
            }
        });
    } @finally { errno=incoming; }
}
