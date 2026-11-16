/* SPDX-License-Identifier: BSD-2-Clause
 * Read two observed Earth requests only. No compile, file write or removal. */
#pragma once
#include "../sweetgl/ProgramCacheCodec.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef void (*SweetRequestLog)(const char *, ...);
static inline void sweet_request_hex(const unsigned char *p,size_t n,char *out)
{static const char digits[]="0123456789abcdef";for(size_t i=0;i<n;++i){out[2*i]=digits[p[i]>>4];out[2*i+1]=digits[p[i]&15];}out[2*n]=0;}
static inline int sweet_request_read(int fd,unsigned char *out,size_t count)
{size_t offset=0;while(offset<count){ssize_t got=read(fd,out+offset,count-offset);if(got<0 && errno==EINTR)continue;if(got<=0)return -EIO;offset+=(size_t)got;}return 0;}
static inline int sweet_request_export_at(const char *root,const unsigned char identity[32],SweetRequestLog log)
{
    unsigned records=0,seen=0,mask=0;size_t total=0;int result=0;
    DIR *dir=opendir(root);if(!dir)return -errno;
    char id[65],prefix[40];sweet_request_hex(identity,32,id);snprintf(prefix,sizeof(prefix),"sgpc-%.32s-",id);
    struct dirent *entry;
    while((entry=readdir(dir))){size_t name_bytes=strlen(entry->d_name);
        if(strncmp(entry->d_name,prefix,strlen(prefix)) || name_bytes<4 || strcmp(entry->d_name+name_bytes-4,".req"))continue;
        if(++seen>64){result=-E2BIG;break;}
        char path[1024];int length=snprintf(path,sizeof(path),"%s/%s",root,entry->d_name);
        if(length<=0 || length>=(int)sizeof(path)){result=-EINVAL;break;}
        int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0){result=-errno;break;}
        struct stat st;unsigned char head[SWEET_PROGRAM_CACHE_HEADER];
        if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<(off_t)sizeof(head) || sweet_request_read(fd,head,sizeof(head))){close(fd);result=-EINVAL;break;}
        uint32_t vs=sweet_cache_get32(head+24),fs=sweet_cache_get32(head+28);
        unsigned bit=fs==86136?1:fs==45773?2:0;
        if(vs!=4840 || !bit){close(fd);continue;}
        size_t bytes=(size_t)st.st_size;
        if(bytes>131072 || records>=2 || bytes>262144-total || (mask&bit)){close(fd);result=-E2BIG;break;}
        unsigned char *record=malloc(bytes);if(!record){close(fd);result=-ENOMEM;break;}
        memcpy(record,head,sizeof(head));unsigned char extra;
        result=sweet_request_read(fd,record+sizeof(head),bytes-sizeof(head));
        if(!result && read(fd,&extra,1)!=0)result=-EINVAL;
        close(fd);
        if(!result && !sweet_cache_validate(record,bytes,SWEET_PROGRAM_CACHE_REQUEST,identity,NULL))result=-EINVAL;
        char key[65],expected[128];unsigned char actual_key[32];SweetRenderPipelineConfig config;
        if(!result){memcpy(&config,record+SWEET_PROGRAM_CACHE_HEADER,sizeof(config));
            const char *vertex=(const char *)record+SWEET_PROGRAM_CACHE_HEADER+sizeof(config),*fragment=vertex+vs;
            sweet_cache_key(identity,&config,vertex,vs,fragment,fs,actual_key);
            sweet_request_hex(actual_key,32,key);snprintf(expected,sizeof(expected),"%s%s.req",prefix,key);
            if(memcmp(actual_key,record+72,32) || strcmp(entry->d_name,expected))result=-EINVAL;}
        if(!result){unsigned char digest[32];char sha[65];sweet_cache_digest(record,bytes,digest);sweet_request_hex(digest,32,sha);
            /* The fixed console tee truncates long application writes. Keep
             * each record below 180 bytes; integrity still covers all bytes. */
            log("SWEET PROGRAM REQUEST begin index=%u bytes=%lu vertex_bytes=%u fragment_bytes=%u read_only=1 chunks=32",records,(unsigned long)bytes,vs,fs);
            log("SWEET PROGRAM REQUEST identity index=%u value=%s",records,id);
            log("SWEET PROGRAM REQUEST key index=%u value=%s",records,key);
            log("SWEET PROGRAM REQUEST digest index=%u record_sha=%s",records,sha);
            for(size_t at=0;at<bytes;){size_t count=bytes-at;if(count>32)count=32;char hex[65];sweet_request_hex(record+at,count,hex);
                log("SWEET PROGRAM REQUEST data index=%u offset=%lu bytes=%lu hex=%s",records,(unsigned long)at,(unsigned long)count,hex);at+=count;}
            log("SWEET PROGRAM REQUEST END index=%u bytes=%lu record_sha=%s complete=1 read_only=1",records,(unsigned long)bytes,sha);
            ++records;mask|=bit;total+=bytes;}
        free(record);if(result)break;
    }
    closedir(dir);if(!result && mask!=3)result=-ENOENT;
    log("SWEET PROGRAM REQUEST EXPORT END result=%d records=%u bytes=%lu limit=262144 read_only=1 no_compile=1",result,records,(unsigned long)total);
    return result;
}
