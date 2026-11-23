/* SPDX-License-Identifier: BSD-2-Clause
 * Inventory reads immutable records. A separate bounded callback handles only
 * validated requests whose binary is absent; it is never called for an empty set. */
#pragma once
#include "../sweetgl/ProgramCacheCodec.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

enum { PCINV_MAX_FILES=64, PCINV_MAX_SEEN=4096, PCINV_MAX_PRIME=8 };
#define PCINV_MAX_BYTES (UINT64_C(32)<<20)
typedef void (*SweetCacheInventoryLog)(const char *, ...);
typedef int (*SweetCacheInventoryPrimer)(uint32_t *,uint32_t *);
struct SweetCacheInventorySummary {
    unsigned files,records,requests,binaries,present,missing,pending,orphans;
    uint64_t bytes;
    bool verified;
};
struct SweetCacheInventoryRecord {
    unsigned char key[32],digest[32],config[32],vertex[32],fragment[32];
    uint32_t kind,vs,fs,format;
    size_t bytes;
};
static inline void pcinv_hex(const unsigned char *p,size_t n,char *out)
{
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<n;++i){out[2*i]=digits[p[i]>>4];out[2*i+1]=digits[p[i]&15];}
    out[2*n]=0;
}
static inline int pcinv_nibble(char c)
{return c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:-1;}
static inline int pcinv_name(const char *name,const char *prefix,unsigned char key[32],unsigned *kind)
{
    size_t n=strlen(prefix);
    if(strncmp(name,prefix,n) || strlen(name)<n+68)return -EINVAL;
    for(unsigned i=0;i<32;++i){int a=pcinv_nibble(name[n+2*i]),b=pcinv_nibble(name[n+2*i+1]);
        if(a<0 || b<0)return -EINVAL;key[i]=(unsigned char)(a*16+b);}
    const char *suffix=name+n+64;
    if(!strcmp(suffix,".req")){*kind=SWEET_PROGRAM_CACHE_REQUEST;return 0;}
    if(!strcmp(suffix,".bin")){*kind=SWEET_PROGRAM_CACHE_BINARY;return 0;}
    if(strncmp(suffix,".req.part-",10) && strncmp(suffix,".bin.part-",10))return -EINVAL;
    suffix+=10;
    if(*suffix<'0' || *suffix>'9')return -EINVAL;
    while(*suffix>='0' && *suffix<='9')++suffix;
    if(*suffix++!='-' || *suffix<'0' || *suffix>'9')return -EINVAL;
    while(*suffix>='0' && *suffix<='9')++suffix;
    if(*suffix)return -EINVAL;
    *kind=0;return 0; /* A retained unpublished file is counted, never read. */
}
static inline bool pcinv_same_stat(const struct stat *a,const struct stat *b)
{
    if(a->st_dev!=b->st_dev || a->st_ino!=b->st_ino || a->st_size!=b->st_size || a->st_mode!=b->st_mode)return false;
#ifdef __APPLE__
    return a->st_mtimespec.tv_sec==b->st_mtimespec.tv_sec && a->st_mtimespec.tv_nsec==b->st_mtimespec.tv_nsec &&
        a->st_ctimespec.tv_sec==b->st_ctimespec.tv_sec && a->st_ctimespec.tv_nsec==b->st_ctimespec.tv_nsec;
#else
    return a->st_mtim.tv_sec==b->st_mtim.tv_sec && a->st_mtim.tv_nsec==b->st_mtim.tv_nsec &&
        a->st_ctim.tv_sec==b->st_ctim.tv_sec && a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;
#endif
}
static inline int pcinv_read(int rootfd,const char *name,const struct stat *expected,
    unsigned char **out,size_t *count)
{
    *out=NULL;*count=0;
    int fd=openat(rootfd,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)return -errno;
    int result=0;struct stat first,last,path;
    if(fstat(fd,&first))result=-errno;
    else if(!S_ISREG(first.st_mode) || !pcinv_same_stat(&first,expected) ||
        first.st_size<SWEET_PROGRAM_CACHE_HEADER || (uint64_t)first.st_size>SWEET_PROGRAM_CACHE_HEADER+SWEET_PROGRAM_CACHE_MAX)result=-EINVAL;
    size_t bytes=result?0:(size_t)first.st_size,at=0;unsigned interrupts=0;
    unsigned char *data=result?NULL:malloc(bytes);
    if(!result && !data)result=-ENOMEM;
    while(!result && at<bytes){ssize_t got=read(fd,data+at,bytes-at);
        if(got<0 && errno==EINTR && interrupts++<64)continue;
        if(got<=0){result=-(got<0 && errno?errno:EIO);break;}at+=(size_t)got;}
    if(!result){unsigned char extra;ssize_t got;
        do{got=read(fd,&extra,1);}while(got<0 && errno==EINTR && interrupts++<64);
        if(got)result=got<0?-errno:-EINVAL;}
    if(!result && (fstat(fd,&last) || fstatat(rootfd,name,&path,AT_SYMLINK_NOFOLLOW)))result=-errno;
    if(!result && (!pcinv_same_stat(&first,&last) || !pcinv_same_stat(&first,&path)))result=-ESTALE;
    if(close(fd) && !result)result=-errno;
    if(result){free(data);return result;}*out=data;*count=bytes;return 0;
}
static inline int pcinv_compare_names(const void *a,const void *b)
{return strcmp((const char *)a,(const char *)b);}
static inline void pcinv_emit_record(SweetCacheInventoryLog log,int pid,char phase,unsigned index,
    const struct SweetCacheInventoryRecord *record)
{
    char hex[65];
    log("SWEET PCINV ITEM pid=%d phase=%c index=%u kind=%c vs=%u fs=%u bytes=%lu format=%u",
        pid,phase,index,record->kind==SWEET_PROGRAM_CACHE_REQUEST?'R':'B',record->vs,record->fs,(unsigned long)record->bytes,record->format);
    pcinv_hex(record->key,32,hex);log("SWEET PCINV KEY pid=%d phase=%c index=%u value=%s",pid,phase,index,hex);
    pcinv_hex(record->digest,32,hex);log("SWEET PCINV SHA pid=%d phase=%c index=%u value=%s",pid,phase,index,hex);
    if(record->kind==SWEET_PROGRAM_CACHE_REQUEST){
        pcinv_hex(record->config,32,hex);log("SWEET PCINV CFG pid=%d phase=%c index=%u value=%s",pid,phase,index,hex);
        pcinv_hex(record->vertex,32,hex);log("SWEET PCINV VS pid=%d phase=%c index=%u value=%s",pid,phase,index,hex);
        pcinv_hex(record->fragment,32,hex);log("SWEET PCINV FS pid=%d phase=%c index=%u value=%s",pid,phase,index,hex);
    }
    log("SWEET PCINV ENDITEM pid=%d phase=%c index=%u record_verified=1 import_performed=0",pid,phase,index);
}
static inline int sweet_cache_inventory_at(const char *root,const unsigned char identity[32],char phase,
    SweetCacheInventoryLog log,struct SweetCacheInventorySummary *summary)
{
    int saved=errno;
    if(!summary || !log || !identity || !root || root[0]!='/' || strlen(root)>800 || (phase!='B' && phase!='A')){errno=saved;return -EINVAL;}
    memset(summary,0,sizeof(*summary));int result=0,pid=getpid();DIR *dir=NULL;
    char id[65],prefix[40],names[PCINV_MAX_FILES][256];unsigned seen=0,names_used=0;
    struct SweetCacheInventoryRecord records[PCINV_MAX_FILES];memset(records,0,sizeof(records));
    pcinv_hex(identity,32,id);snprintf(prefix,sizeof(prefix),"sgpc-%.32s-",id);
    log("SWEET PCINV BEGIN pid=%d phase=%c revision=1 max_files=64 max_bytes=33554432 read_only=1 no_compile=1",pid,phase);
    log("SWEET PCINV ID pid=%d phase=%c value=%s",pid,phase,id);
    int fd=open(root,O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW);
    if(fd<0){result=-errno;goto done;}
    struct stat directory;
    if(fstat(fd,&directory)){result=-errno;close(fd);goto done;}
    if(!S_ISDIR(directory.st_mode) || (directory.st_mode&07777)!=01777){result=-EPERM;close(fd);goto done;}
    dir=fdopendir(fd);if(!dir){result=-errno;close(fd);goto done;}
    for(;;){errno=0;struct dirent *entry=readdir(dir);
        if(!entry){if(errno)result=-errno;break;}
        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))continue;
        if(++seen>PCINV_MAX_SEEN){result=-E2BIG;break;}
        if(strncmp(entry->d_name,prefix,strlen(prefix)))continue;
        if(names_used==PCINV_MAX_FILES || strlen(entry->d_name)>=sizeof(names[0])){result=-E2BIG;break;}
        strcpy(names[names_used++],entry->d_name);
    }
    if(result)goto done;
    qsort(names,names_used,sizeof(names[0]),pcinv_compare_names);
    for(unsigned i=0;i<names_used;++i){
        struct stat st;unsigned kind;unsigned char key[32];
        if(pcinv_name(names[i],prefix,key,&kind)){result=-EINVAL;break;}
        if(fstatat(dirfd(dir),names[i],&st,AT_SYMLINK_NOFOLLOW)){result=-errno;break;}
        if(!S_ISREG(st.st_mode) || st.st_size<0 || (uint64_t)st.st_size>SWEET_PROGRAM_CACHE_HEADER+SWEET_PROGRAM_CACHE_MAX){result=-EINVAL;break;}
        if((uint64_t)st.st_size>PCINV_MAX_BYTES-summary->bytes){result=-E2BIG;break;}
        ++summary->files;summary->bytes+=(uint64_t)st.st_size;
        if(!kind){++summary->pending;continue;}
        unsigned char *data=NULL;size_t count=0;
        result=pcinv_read(dirfd(dir),names[i],&st,&data,&count);if(result)break;
        if(!sweet_cache_validate(data,count,kind,identity,key)){free(data);result=-EINVAL;break;}
        struct SweetCacheInventoryRecord *record=&records[summary->records];
        memcpy(record->key,key,32);record->kind=kind;record->bytes=count;
        record->vs=sweet_cache_get32(data+24);record->fs=sweet_cache_get32(data+28);record->format=sweet_cache_get32(data+16);
        sweet_cache_digest(data,count,record->digest);
        if(kind==SWEET_PROGRAM_CACHE_REQUEST){
            SweetRenderPipelineConfig config;const unsigned char *payload=data+SWEET_PROGRAM_CACHE_HEADER;
            memcpy(&config,payload,sizeof(config));unsigned char computed[32];
            sweet_cache_key(identity,&config,(const char *)payload+sizeof(config),record->vs,
                (const char *)payload+sizeof(config)+record->vs,record->fs,computed);
            if(memcmp(computed,key,32)){free(data);result=-EINVAL;break;}
            sweet_cache_digest(payload,sizeof(config),record->config);
            sweet_cache_digest(payload+sizeof(config),record->vs,record->vertex);
            sweet_cache_digest(payload+sizeof(config)+record->vs,record->fs,record->fragment);++summary->requests;
        }else ++summary->binaries;
        pcinv_emit_record(log,pid,phase,summary->records,record);++summary->records;free(data);
    }
    if(result)goto done;
    for(unsigned i=0;i<summary->records;++i){
        if(records[i].kind!=SWEET_PROGRAM_CACHE_REQUEST)continue;bool found=false;
        for(unsigned j=0;j<summary->records;++j){
            if(records[j].kind!=SWEET_PROGRAM_CACHE_BINARY || memcmp(records[i].key,records[j].key,32))continue;
            if(records[i].vs!=records[j].vs || records[i].fs!=records[j].fs){result=-EINVAL;goto done;}found=true;
        }
        if(found)++summary->present;else ++summary->missing;
    }
    summary->orphans=summary->binaries-summary->present;
done:
    if(dir && closedir(dir) && !result)result=-errno;
    summary->verified=result==0;
    log("SWEET PCINV END pid=%d phase=%c result=%d files=%u records=%u requests=%u bins=%u present=%u missing=%u",
        pid,phase,result,summary->files,summary->records,summary->requests,summary->binaries,summary->present,summary->missing);
    log("SWEET PCINV TOTAL pid=%d phase=%c bytes=%llu pending=%u orphans=%u read_only=1 import_performed=0",
        pid,phase,(unsigned long long)summary->bytes,summary->pending,summary->orphans);
    errno=saved;return result;
}
static inline int sweet_cache_prime_if_missing(const struct SweetCacheInventorySummary *before,
    SweetCacheInventoryPrimer primer,SweetCacheInventoryLog log)
{
    int saved=errno,result=0;unsigned action=before && before->missing?1:0;
    if(!before || !before->verified || !primer || !log || before->requests!=before->present+before->missing ||
        before->files!=before->records+before->pending || before->records!=before->requests+before->binaries ||
        before->binaries!=before->present+before->orphans || before->missing>PCINV_MAX_PRIME){errno=saved;return -EINVAL;}
    log("SWEET PCINV ACTION pid=%d action=%s missing=%u max_entries=8 actual_files_checked=1",getpid(),action?"prime":"skip",before->missing);
    if(action){uint32_t compiled=0,failed=0;result=primer(&compiled,&failed);
        log("SWEET PCINV PRIME pid=%d result=%d compiled=%u failed=%u expected=%u",getpid(),result,compiled,failed,before->missing);
        if(!result && (failed || compiled!=before->missing))result=-EIO;
    }
    errno=saved;return result;
}
