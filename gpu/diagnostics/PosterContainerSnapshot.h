/* SPDX-License-Identifier: BSD-2-Clause
 * Read only immediate MCM metadata, never application data or UI state. */
#pragma once
#include "PosterConfigSnapshot.h"

enum { PCON_ROOTS=4, PCON_ENTRIES=256, PCON_FILES=64 };
struct SweetPosterContainerSummary {
    unsigned roots,opened,examined,files,bytes,skipped,errors;int result;
};
static inline void pcon_ranges(SweetPosterConfigLog log,SweetPosterConfigPause pause,
    const char *kind,unsigned id,const unsigned char *bytes,size_t length)
{
    for(size_t at=0;at<length;at+=PCFG_CHUNK){
        size_t n=length-at;if(n>PCFG_CHUNK)n=PCFG_CHUNK;
        char hex[PCFG_CHUNK*2+1];pcfg_hex(bytes+at,n,hex);
        log("SWEET PCON %s id=%u offset=%zu bytes=%zu hex=%s",kind,id,at,n,hex);
        if(pause)pause();
    }
}
static inline struct SweetPosterContainerSummary sweet_poster_container_snapshot_at(
    const char *const *roots,size_t count,SweetPosterConfigLog log,SweetPosterConfigPause pause)
{
    int incoming=errno;struct SweetPosterContainerSummary s={0};
    struct SweetPosterConfigCapture reader={0};reader.log=log;reader.pause=pause;
    if(!roots || !log || count!=PCON_ROOTS){s.result=-EINVAL;errno=incoming;return s;}
    for(size_t i=0;i<count;++i)if(!roots[i] || roots[i][0]!='/' || strlen(roots[i])>=PCFG_PATH){
        s.result=-EINVAL;errno=incoming;return s;}
    s.roots=(unsigned)count;reader.buffer=malloc(PCFG_FILE_BYTES);
    if(!reader.buffer)s.result=-ENOMEM;
    log("SWEET PCON BEGIN revision=1 roots=4 max_entries=256 max_files=64 max_bytes=131072 read_only=1");
    for(unsigned root=0;root<count;++root){
        int fd=s.result?-1:pcfg_open_root(roots[root]);int error=s.result?-s.result:fd<0?errno:0;
        log("SWEET PCON ROOT index=%u path_bytes=%zu result=%d",root,strlen(roots[root]),-error);
        pcon_ranges(log,pause,"ROOTPATH",root,(const unsigned char *)roots[root],strlen(roots[root]));
        if(fd<0){if(error!=ENOENT)++s.errors;continue;}
        DIR *dir=fdopendir(fd);
        if(!dir){error=errno;close(fd);++s.errors;
            log("SWEET PCON WALK root=%u result=%d",root,-error);continue;}
        ++s.opened;
        while(!s.result){
            errno=0;struct dirent *entry=readdir(dir);
            if(!entry){if(errno){++s.errors;log("SWEET PCON WALK root=%u result=%d",root,-errno);}break;}
            const char *name=entry->d_name;if(!strcmp(name,".") || !strcmp(name,".."))continue;
            if(s.examined==PCON_ENTRIES){s.result=-E2BIG;break;}
            unsigned id=++s.examined;char full[PCFG_PATH];
            int length=snprintf(full,sizeof(full),"%s/%s",roots[root],name);
            if(length<=0 || (size_t)length>=sizeof(full)){s.result=-ENAMETOOLONG;break;}
            struct stat catalog;int result=fstatat(dirfd(dir),name,&catalog,AT_SYMLINK_NOFOLLOW)?-errno:0;
            char kind=result?'E':S_ISDIR(catalog.st_mode)?'D':S_ISREG(catalog.st_mode)?'F':S_ISLNK(catalog.st_mode)?'L':'O';
            log("SWEET PCON ENTRY id=%u root=%u kind=%c path_bytes=%d result=%d",id,root,kind,length,result);
            pcon_ranges(log,pause,"PATH",id,(const unsigned char *)full,(size_t)length);
            bool captured=false;size_t bytes=0;
            if(!result && kind=='D'){
                int child=openat(dirfd(dir),name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
                if(child<0)result=-errno;
                else{
                    struct stat current;
                    if(fstat(child,&current) || !pcfg_same_stat(&catalog,&current))result=-ESTALE;
                    else{
                        const char *metadata=".com.apple.mobile_container_manager.metadata.plist";
                        struct stat file;
                        if(fstatat(child,metadata,&file,AT_SYMLINK_NOFOLLOW))result=errno==ENOENT?1:-errno;
                        else if(!S_ISREG(file.st_mode))result=1;
                        else if(file.st_size<=0 || file.st_size>PCFG_FILE_BYTES)result=-EFBIG;
                        else if(s.files==PCON_FILES || (uint64_t)file.st_size>PCFG_TOTAL_BYTES-s.bytes){
                            s.result=-E2BIG;result=-E2BIG;
                        }else{
                            result=pcfg_read_file(&reader,child,metadata,&file,&bytes);
                            captured=!result;
                        }
                    }
                    close(child);
                }
            }else if(!result)result=1;
            if(captured){
                unsigned char digest[32];char hex[65];sweet_cache_digest(reader.buffer,bytes,digest);pcfg_hex(digest,32,hex);
                log("SWEET PCON FILE id=%u bytes=%zu sha256=%s stable=1",id,bytes,hex);
                pcon_ranges(log,pause,"DATA",id,reader.buffer,bytes);
                ++s.files;s.bytes+=(unsigned)bytes;
            }else if(result<0)++s.errors;else ++s.skipped;
            log("SWEET PCON ITEMEND id=%u result=%d captured=%u bytes=%zu",id,result,captured,captured?bytes:0);
        }
        closedir(dir);
    }
    log("SWEET PCON END result=%d roots=%u opened=%u examined=%u files=%u bytes=%u skipped=%u errors=%u read_only=1",
        s.result,s.roots,s.opened,s.examined,s.files,s.bytes,s.skipped,s.errors);
    free(reader.buffer);errno=incoming;return s;
}
static inline struct SweetPosterContainerSummary sweet_poster_container_snapshot(
    SweetPosterConfigLog log,SweetPosterConfigPause pause)
{
    static const char *const roots[]={"/private/var/mobile/Containers/Data/Application",
        "/private/var/mobile/Containers/Shared/AppGroup","/private/var/containers/Data/System",
        "/private/var/containers/Shared/SystemGroup"};
    return sweet_poster_container_snapshot_at(roots,PCON_ROOTS,log,pause);
}
