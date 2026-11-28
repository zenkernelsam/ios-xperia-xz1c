/* SPDX-License-Identifier: BSD-2-Clause
 * Bounded metadata snapshots. No UI getters, settings changes or image reads. */
#pragma once
#include "../sweetgl/ProgramCacheCodec.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

enum { PCFG_PATH=512, PCFG_CHUNK=64, PCFG_ENTRIES=256, PCFG_DEPTH=4,
       PCFG_FILES=16, PCFG_FILE_BYTES=65536, PCFG_TOTAL_BYTES=131072 };
typedef void (*SweetPosterConfigLog)(const char *,...);
typedef void (*SweetPosterConfigPause)(void);
struct SweetPosterConfigSummary { unsigned roots,opened,entries,files,bytes,skipped,errors;int result; };
struct SweetPosterConfigCapture {
    SweetPosterConfigLog log;SweetPosterConfigPause pause;
    struct SweetPosterConfigSummary summary;
    unsigned char *buffer;
};

static inline void pcfg_hex(const unsigned char *in,size_t n,char *out)
{
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<n;++i){out[2*i]=digits[in[i]>>4];out[2*i+1]=digits[in[i]&15];}
    out[2*n]=0;
}
static inline void pcfg_ranges(struct SweetPosterConfigCapture *c,const char *kind,
    unsigned id,const unsigned char *bytes,size_t length)
{
    for(size_t at=0;at<length;at+=PCFG_CHUNK){
        size_t n=length-at;if(n>PCFG_CHUNK)n=PCFG_CHUNK;
        char hex[PCFG_CHUNK*2+1];pcfg_hex(bytes+at,n,hex);
        c->log("SWEET PCFG %s id=%u offset=%zu bytes=%zu hex=%s",kind,id,at,n,hex);
        if(c->pause)c->pause();
    }
}
static inline bool pcfg_same_stat(const struct stat *a,const struct stat *b)
{
    if(a->st_dev!=b->st_dev || a->st_ino!=b->st_ino || a->st_mode!=b->st_mode || a->st_size!=b->st_size)return false;
#ifdef __APPLE__
    return a->st_mtimespec.tv_sec==b->st_mtimespec.tv_sec && a->st_mtimespec.tv_nsec==b->st_mtimespec.tv_nsec &&
        a->st_ctimespec.tv_sec==b->st_ctimespec.tv_sec && a->st_ctimespec.tv_nsec==b->st_ctimespec.tv_nsec;
#else
    return a->st_mtim.tv_sec==b->st_mtim.tv_sec && a->st_mtim.tv_nsec==b->st_mtim.tv_nsec &&
        a->st_ctim.tv_sec==b->st_ctim.tv_sec && a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;
#endif
}
static inline bool pcfg_relevant_name(const char *name)
{
    char lower[PCFG_PATH];size_t n=strlen(name);if(n>=sizeof(lower))return false;
    for(size_t i=0;i<=n;++i)lower[i]=name[i]>='A' && name[i]<='Z'?name[i]-'A'+'a':name[i];
    return strstr(lower,"poster") || strstr(lower,"wallpaper") ||
        strstr(lower,"lockbackground") || strstr(lower,"homebackground");
}
static inline bool pcfg_metadata(const unsigned char *p,size_t n)
{
    if(n>=8 && !memcmp(p,"bplist00",8))return true;
    size_t at=0;while(at<n && (p[at]==' ' || p[at]=='\n' || p[at]=='\r' || p[at]=='\t'))++at;
    if(at==n)return false;
    return (n-at>=5 && !memcmp(p+at,"<?xml",5)) || (n-at>=6 && !memcmp(p+at,"<plist",6)) ||
        p[at]=='{' || p[at]=='[';
}
static inline bool pcfg_image_name(const char *name)
{
    const char *suffix=strrchr(name,'.');if(!suffix)return false;
    return !strcasecmp(suffix,".png") || !strcasecmp(suffix,".jpg") || !strcasecmp(suffix,".jpeg") ||
        !strcasecmp(suffix,".heic") || !strcasecmp(suffix,".gif") || !strcasecmp(suffix,".cpbitmap") ||
        !strcasecmp(suffix,".metallib");
}
static inline int pcfg_open_root(const char *path)
{
    if(!path || path[0]!='/' || strlen(path)>=PCFG_PATH){errno=EINVAL;return -1;}
    int fd=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(fd<0)return -1;
    const char *p=path+1;
    while(*p){const char *end=strchr(p,'/');size_t n=end?(size_t)(end-p):strlen(p);char name[PCFG_PATH];
        if(!n || n>=sizeof(name) || (n==1 && *p=='.') || (n==2 && p[0]=='.' && p[1]=='.')){close(fd);errno=EINVAL;return -1;}
        memcpy(name,p,n);name[n]=0;
        int next=openat(fd,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);int error=errno;close(fd);
        if(next<0){errno=error;return -1;}fd=next;
        if(!end)break;
        p=end+1;
    }return fd;
}
static inline int pcfg_read_file(struct SweetPosterConfigCapture *c,int parent,const char *name,
    const struct stat *catalog,size_t *length)
{
    int fd=openat(parent,name,O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return -errno;
    struct stat before,after;int result=0;
    if(fstat(fd,&before) || !S_ISREG(before.st_mode) || !pcfg_same_stat(catalog,&before))result=-ESTALE;
    size_t size=result?0:(size_t)before.st_size,at=0,prefix=size<8?size:8;
    while(!result && at<prefix){ssize_t n=read(fd,c->buffer+at,prefix-at);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){result=n<0?-errno:-EIO;break;}at+=(size_t)n;
    }
    if(!result && !pcfg_metadata(c->buffer,prefix))result=1; /* Only the signature was read. */
    while(!result && at<size){ssize_t n=read(fd,c->buffer+at,size-at);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){result=n<0?-errno:-EIO;break;}at+=(size_t)n;
    }
    if(result>=0 && (fstat(fd,&after) || !pcfg_same_stat(&before,&after)))result=-ESTALE;
    close(fd);if(!result)*length=size;return result;
}
static inline void pcfg_walk(struct SweetPosterConfigCapture *c,int fd,const char *path,
    unsigned root,unsigned depth,bool filtered)
{
    DIR *dir=fdopendir(fd);if(!dir){int error=errno;close(fd);++c->summary.errors;
        c->log("SWEET PCFG WALK root=%u result=%d",root,-error);return;}
    struct dirent *entry;
    while(!c->summary.result){
        errno=0;entry=readdir(dir);
        if(!entry){if(errno){++c->summary.errors;c->log("SWEET PCFG WALK root=%u result=%d",root,-errno);}break;}
        const char *name=entry->d_name;
        if(!strcmp(name,".") || !strcmp(name,".."))continue;
        if(filtered && !pcfg_relevant_name(name))continue;
        if(++c->summary.entries>PCFG_ENTRIES){--c->summary.entries;c->summary.result=-E2BIG;break;}
        unsigned id=c->summary.entries;char full[PCFG_PATH];
        int count=snprintf(full,sizeof(full),"%s/%s",path,name);
        if(count<=0 || (size_t)count>=sizeof(full)){c->summary.result=-ENAMETOOLONG;break;}
        struct stat st;int result=fstatat(dirfd(dir),name,&st,AT_SYMLINK_NOFOLLOW)?-errno:0;
        char kind=result?'E':S_ISDIR(st.st_mode)?'D':S_ISREG(st.st_mode)?'F':S_ISLNK(st.st_mode)?'L':'O';
        c->log("SWEET PCFG ENTRY id=%u root=%u depth=%u kind=%c path_bytes=%d size=%lld result=%d",
            id,root,depth,kind,count,result?0:(long long)st.st_size,result);
        pcfg_ranges(c,"PATH",id,(const unsigned char *)full,(size_t)count);
        if(result){++c->summary.errors;c->log("SWEET PCFG ITEMEND id=%u result=%d captured=0 bytes=0",id,result);continue;}
        if(kind=='D'){
            c->log("SWEET PCFG ITEMEND id=%u result=0 captured=0 bytes=0",id);
            if(depth<PCFG_DEPTH){int child=openat(dirfd(dir),name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
                if(child<0){++c->summary.errors;c->log("SWEET PCFG WALK root=%u result=%d",root,-errno);}
                else{struct stat current;if(fstat(child,&current) || !pcfg_same_stat(&st,&current)){
                        close(child);++c->summary.errors;c->log("SWEET PCFG WALK root=%u result=%d",root,-ESTALE);}
                    else pcfg_walk(c,child,full,root,depth+1,false);}}
            else ++c->summary.skipped;
        }else if(kind=='F' && !pcfg_image_name(name) && st.st_size>0 && st.st_size<=PCFG_FILE_BYTES &&
            c->summary.files<PCFG_FILES && (uint64_t)st.st_size<=PCFG_TOTAL_BYTES-c->summary.bytes){
            size_t bytes=0;result=pcfg_read_file(c,dirfd(dir),name,&st,&bytes);
            bool metadata=!result && pcfg_metadata(c->buffer,bytes);
            if(metadata){unsigned char digest[32];char hex[65];sweet_cache_digest(c->buffer,bytes,digest);pcfg_hex(digest,32,hex);
                c->log("SWEET PCFG FILE id=%u bytes=%zu sha256=%s stable=1",id,bytes,hex);
                pcfg_ranges(c,"DATA",id,c->buffer,bytes);++c->summary.files;c->summary.bytes+=(unsigned)bytes;
            }else if(result<0)++c->summary.errors;else ++c->summary.skipped;
            c->log("SWEET PCFG ITEMEND id=%u result=%d captured=%u bytes=%zu",id,result,metadata,metadata?bytes:0);
        }else{++c->summary.skipped;c->log("SWEET PCFG ITEMEND id=%u result=0 captured=0 bytes=0",id);}
    }
    closedir(dir);
}
static inline struct SweetPosterConfigSummary sweet_poster_config_snapshot_at(const char *const *roots,
    const bool *filtered,size_t count,SweetPosterConfigLog log,SweetPosterConfigPause pause)
{
    int incoming=errno;struct SweetPosterConfigCapture c={0};c.log=log;c.pause=pause;
    if(!roots || !filtered || !log || !count || count>8){c.summary.result=-EINVAL;return c.summary;}
    for(size_t i=0;i<count;++i)if(!roots[i] || roots[i][0]!='/' || strlen(roots[i])>=PCFG_PATH){c.summary.result=-EINVAL;return c.summary;}
    c.summary.roots=(unsigned)count;c.buffer=malloc(PCFG_FILE_BYTES);
    if(!c.buffer)c.summary.result=-ENOMEM;
    log("SWEET PCFG BEGIN revision=1 roots=%zu max_entries=256 max_files=16 max_bytes=131072 max_depth=4 read_only=1",count);
    for(unsigned i=0;i<count && !c.summary.result;++i){
        int fd=pcfg_open_root(roots[i]),error=fd<0?errno:0;
        log("SWEET PCFG ROOT index=%u path_bytes=%zu result=%d",i,strlen(roots[i]),-error);
        pcfg_ranges(&c,"ROOTPATH",i,(const unsigned char *)roots[i],strlen(roots[i]));
        if(fd<0){if(error!=ENOENT)++c.summary.errors;continue;}
        ++c.summary.opened;pcfg_walk(&c,fd,roots[i],i,0,filtered[i]);
    }
    log("SWEET PCFG END result=%d roots=%u opened=%u entries=%u files=%u bytes=%u skipped=%u errors=%u read_only=1",
        c.summary.result,c.summary.roots,c.summary.opened,c.summary.entries,c.summary.files,c.summary.bytes,c.summary.skipped,c.summary.errors);
    free(c.buffer);errno=incoming;return c.summary;
}
static inline struct SweetPosterConfigSummary sweet_poster_config_snapshot(SweetPosterConfigLog log,SweetPosterConfigPause pause)
{
    static const char *const roots[]={"/private/var/mobile/Library/PosterBoard",
        "/private/var/mobile/Library/Application Support/PosterBoard",
        "/private/var/mobile/Library/SpringBoard","/private/var/mobile/Library/Preferences",
        "/private/var/mobile/Library/Wallpaper"};
    static const bool filtered[]={false,false,true,true,false};
    return sweet_poster_config_snapshot_at(roots,filtered,5,log,pause);
}
