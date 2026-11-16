/* SPDX-License-Identifier: BSD-2-Clause; only create a new shared cache root. */
#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static inline int sweet_program_cache_scope_at(const char *directory,int *inherited)
{
    if(!directory || directory[0]!='/' || strlen(directory)>160 || !inherited)return -EINVAL;
    const char *current=getenv("SWEETGL_PROGRAM_CACHE_DIR");
    *inherited=current && *current;
    if(*inherited && strcmp(current,directory))return -EINVAL;
    int created=mkdir(directory,01777)==0;
    if(!created && errno!=EEXIST)return -errno;
    struct stat before;if(lstat(directory,&before) || !S_ISDIR(before.st_mode))return -EINVAL;
    if(created){
        int fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -errno;
        struct stat actual;int result=0;
        if(fstat(fd,&actual) || !S_ISDIR(actual.st_mode) || actual.st_ino!=before.st_ino || actual.st_dev!=before.st_dev)result=-EINVAL;
        else if(fchmod(fd,01777))result=-errno;
        if(close(fd) && !result)result=-errno;
        if(result)return result;
    }else if((before.st_mode&07777)!=01777)return -EPERM;
    // Backboard and the original extension may have different UIDs. Sticky
    // shared creation plus the existing immutable/no-follow codec is required.
    if(!*inherited && setenv("SWEETGL_PROGRAM_CACHE_DIR",directory,0))return -errno;
    current=getenv("SWEETGL_PROGRAM_CACHE_DIR");
    return current && !strcmp(current,directory)?0:-EINVAL;
}
