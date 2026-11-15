/* SPDX-License-Identifier: BSD-2-Clause
 * Immutable bounded cache files. No existing file is overwritten or removed.
 */
#pragma once
#include "ProgramCacheCodec.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#define SWEET_CACHE_MAX_FILES 64u
#define SWEET_CACHE_MAX_BYTES (UINT64_C(32)<<20)
static inline void sweet_cache_hex(const unsigned char *bytes,unsigned count,char *out)
{for(unsigned i=0;i<count;++i)snprintf(out+i*2,3,"%02x",bytes[i]);}
static inline int sweet_cache_path(const char *root,const unsigned char identity[32],const unsigned char key[32],unsigned kind,char out[1024])
{
    if(!root || root[0]!='/' || strlen(root)>800)return -EINVAL;
    char id[33],name[65];sweet_cache_hex(identity,16,id);sweet_cache_hex(key,32,name);
    int n=snprintf(out,1024,"%s/sgpc-%s-%s.%s",root,id,name,kind==SWEET_PROGRAM_CACHE_REQUEST?"req":"bin");
    return n>0 && n<1024?0:-EINVAL;
}
static inline int sweet_cache_read_file(const char *path,unsigned char **bytes,size_t *count)
{
    *bytes=NULL;*count=0;int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -errno;
    struct stat st;if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<SWEET_PROGRAM_CACHE_HEADER ||
        (uint64_t)st.st_size>SWEET_PROGRAM_CACHE_HEADER+SWEET_PROGRAM_CACHE_MAX){close(fd);return -EINVAL;}
    size_t n=(size_t)st.st_size;unsigned char *data=malloc(n);if(!data){close(fd);return -ENOMEM;}
    size_t at=0;while(at<n){ssize_t got=read(fd,data+at,n-at);if(got<0 && errno==EINTR)continue;
        if(got<=0){free(data);close(fd);return -EIO;}at+=(size_t)got;}
    unsigned char extra;if(read(fd,&extra,1)!=0){free(data);close(fd);return -EINVAL;}
    close(fd);*bytes=data;*count=n;return 0;
}
static inline int sweet_cache_budget(const char *root,const unsigned char identity[32],size_t incoming)
{
    DIR *dir=opendir(root);if(!dir)return -errno;char id[33],prefix[40];sweet_cache_hex(identity,16,id);snprintf(prefix,sizeof(prefix),"sgpc-%s-",id);
    unsigned files=0;uint64_t bytes=0;struct dirent *entry;
    while((entry=readdir(dir))){if(strncmp(entry->d_name,prefix,strlen(prefix)))continue;char path[1024];
        int n=snprintf(path,sizeof(path),"%s/%s",root,entry->d_name);struct stat st;
        if(n<=0 || n>=(int)sizeof(path) || lstat(path,&st) || !S_ISREG(st.st_mode) || st.st_size<0){closedir(dir);return -EINVAL;}
        ++files;bytes+=(uint64_t)st.st_size;if(files>=SWEET_CACHE_MAX_FILES || bytes>SWEET_CACHE_MAX_BYTES){closedir(dir);return -ENOSPC;}}
    closedir(dir);return files+2<=SWEET_CACHE_MAX_FILES && incoming<=SWEET_CACHE_MAX_BYTES-bytes?0:-ENOSPC;
}
static inline int sweet_cache_write_file(const char *root,const char *path,const unsigned char identity[32],const unsigned char *bytes,size_t count)
{
    if(count>SWEET_PROGRAM_CACHE_HEADER+SWEET_PROGRAM_CACHE_MAX)return -EINVAL;
    struct stat old;if(!lstat(path,&old))return -EEXIST;
    if(errno!=ENOENT)return -errno;int budget=sweet_cache_budget(root,identity,count);if(budget)return budget;
    static _Atomic unsigned sequence;char pending[1100];int n=snprintf(pending,sizeof(pending),"%s.part-%ld-%u",path,(long)getpid(),atomic_fetch_add(&sequence,1));
    if(n<=0 || n>=(int)sizeof(pending))return -EINVAL;
    int fd=open(pending,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0644);if(fd<0)return -errno;
    if(fchmod(fd,0644)){int error=-errno;close(fd);return error;}
    size_t at=0;while(at<count){ssize_t n=write(fd,bytes+at,count-at);if(n<0 && errno==EINTR)continue;
        if(n<=0){int error=n<0?-errno:-EIO;close(fd);return error;}at+=(size_t)n;}
    int error=fsync(fd)?-errno:0;if(close(fd) && !error)error=-errno;if(error)return error;
#ifdef __APPLE__
    if(renamex_np(pending,path,RENAME_EXCL))return -errno;
#else
    // Portable exclusive publication. The pending hardlink stays recoverable.
    if(link(pending,path))return -errno;
#endif
    return 0;
}
