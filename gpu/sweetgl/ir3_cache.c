/* SPDX-License-Identifier: BSD-2-Clause
 * Persistence for the pinned Freedreno serializer; compilation stays in Mesa. */
#include "IR3CacheAPI.h"
#include "IR3CacheRecord.h"
#include "ProgramCacheBuildIdentity.h"
#include "sweet_drm.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>

static _Atomic uint64_t counters[6]; /* miss, accepted, stored, rejected, read, written */
static pthread_mutex_t cache_write_gate=PTHREAD_MUTEX_INITIALIZER;
static _Atomic unsigned temporary_sequence,log_count;
static const char *root(void)
{const char *name=getenv("SWEETGL_PROGRAM_CACHE_DIR");return name && name[0]=='/'?name:"/private/var/tmp";}
static bool path_for(const void *data,size_t bytes,char path[1024],unsigned char key[32])
{
    if(!data || !bytes || bytes>4096 || strlen(root())>800)return false;
    sweet_ir3_key(sweet_program_cache_build_id,data,bytes,key);char build[33],hex[65];
    for(unsigned i=0;i<16;++i)snprintf(build+i*2,3,"%02x",sweet_program_cache_build_id[i]);
    for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",key[i]);
    int n=snprintf(path,1024,"%s/sgiv-%s-%s.bin",root(),build,hex);return n>0 && n<1024;
}
__attribute__((visibility("default"))) bool sweetgl_ir3_cache_enabled(uint32_t gpu)
{const char *disabled=getenv("SWEETGL_IR3_CACHE_DISABLE");return gpu==540 && (!disabled || strcmp(disabled,"1"));}
__attribute__((visibility("default"))) int sweetgl_ir3_cache_get(const void *data,size_t key_bytes,void **payload,size_t *bytes)
{
    int incoming=errno;*payload=NULL;*bytes=0;char path[1024];unsigned char key[32];int result=-EINVAL;
    if(!path_for(data,key_bytes,path,key))goto done;
    int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0){result=-errno;atomic_fetch_add(&counters[0],1);goto done;}
    struct stat st;if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<SWEET_IR3_HEADER || (uint64_t)st.st_size>SWEET_IR3_HEADER+SWEET_IR3_MAX){close(fd);goto reject;}
    size_t size=(size_t)st.st_size;unsigned char *record=malloc(size);if(!record){close(fd);result=-ENOMEM;goto done;}
    size_t at=0;while(at<size){ssize_t n=read(fd,record+at,size-at);if(n<0 && errno==EINTR)continue;
        if(n<=0){free(record);close(fd);goto reject;}at+=(size_t)n;}
    unsigned char tail;ssize_t remaining=read(fd,&tail,1);close(fd);
    if(remaining!=0 || !sweet_ir3_validate(record,size,sweet_program_cache_build_id,key)){free(record);goto reject;}
    size-=SWEET_IR3_HEADER;memmove(record,record+SWEET_IR3_HEADER,size);*payload=record;*bytes=size;
    atomic_fetch_add(&counters[4],size);result=0;goto done;
reject:
    atomic_fetch_add(&counters[3],1);result=-EINVAL;
done:errno=incoming;return result;
}
static int budget(size_t incoming)
{
    DIR *dir=opendir(root());if(!dir)return -errno;char prefix[39]="sgiv-";
    for(unsigned i=0;i<16;++i)snprintf(prefix+5+i*2,3,"%02x",sweet_program_cache_build_id[i]);strcat(prefix,"-");
    uint64_t total=0;unsigned files=0;struct dirent *entry;int result=0;
    while((entry=readdir(dir))){if(strncmp(entry->d_name,prefix,38))continue;char path[1024];
        int n=snprintf(path,sizeof(path),"%s/%s",root(),entry->d_name);struct stat st;
        if(n<=0 || n>=(int)sizeof(path) || lstat(path,&st) || !S_ISREG(st.st_mode) || st.st_size<0){result=-EINVAL;break;}
        total+=(uint64_t)st.st_size;if(++files>=512 || total>(UINT64_C(64)<<20)){result=-ENOSPC;break;}}
    closedir(dir);return result?result:(files+2<=512 && incoming<=(UINT64_C(64)<<20)-total?0:-ENOSPC);
}
__attribute__((visibility("default"))) int sweetgl_ir3_cache_put(const void *data,size_t key_bytes,const void *payload,size_t bytes)
{
    int incoming=errno,result=-EINVAL;char path[1024];unsigned char key[32],header[SWEET_IR3_HEADER];
    if(!path_for(data,key_bytes,path,key) || !sweet_ir3_record(header,sweet_program_cache_build_id,key,payload,bytes))goto done;
    pthread_mutex_lock(&cache_write_gate);struct stat old;
    if(!lstat(path,&old)){result=-EEXIST;goto unlock;}if(errno!=ENOENT){result=-errno;goto unlock;}
    result=budget(SWEET_IR3_HEADER+bytes);if(result)goto unlock;
    char pending[1100];int n=snprintf(pending,sizeof(pending),"%s.part-%ld-%u",path,(long)getpid(),atomic_fetch_add(&temporary_sequence,1));
    if(n<=0 || n>=(int)sizeof(pending)){result=-EINVAL;goto unlock;}
    int fd=open(pending,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0644);if(fd<0){result=-errno;goto unlock;}
    if(fchmod(fd,0644)){result=-errno;close(fd);goto unlock;}
    const unsigned char *parts[2]={header,payload};size_t sizes[2]={SWEET_IR3_HEADER,bytes};
    for(unsigned i=0;i<2;++i){size_t at=0;while(at<sizes[i]){ssize_t wrote=write(fd,parts[i]+at,sizes[i]-at);
            if(wrote<0 && errno==EINTR)continue;if(wrote<=0){result=wrote<0?-errno:-EIO;close(fd);goto unlock;}at+=(size_t)wrote;}}
    result=fsync(fd)?-errno:0;if(close(fd) && !result)result=-errno;if(result)goto unlock;
#ifdef __APPLE__
    result=renamex_np(pending,path,RENAME_EXCL)?-errno:0;
#else
    result=link(pending,path)?-errno:0; /* Preserve the temporary hardlink. */
#endif
    if(!result){atomic_fetch_add(&counters[2],1);atomic_fetch_add(&counters[5],bytes);
        if(atomic_fetch_add(&log_count,1)<256){char hex[65];for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",key[i]);
            sweet_drm_log("SWEET IR3 CACHE stored key=%s bytes=%lu original_payload_checked=1 immutable=1",hex,(unsigned long)bytes);}}
unlock:pthread_mutex_unlock(&cache_write_gate);
done:errno=incoming;return result;
}
__attribute__((visibility("default"))) void sweetgl_ir3_cache_accepted(size_t bytes)
{
    atomic_fetch_add(&counters[1],1);
    if(atomic_fetch_add(&log_count,1)<128)sweet_drm_log("SWEET IR3 CACHE accepted bytes=%lu original_payload_checked=1 compiler_skipped=1",(unsigned long)bytes);
}
__attribute__((visibility("default"))) void sweetgl_ir3_cache_rejected(void){atomic_fetch_add(&counters[3],1);}
__attribute__((visibility("default"))) void sweetgl_ir3_cache_digest(const void *data,size_t bytes,unsigned char out[32])
{sweet_cache_digest(data,bytes,out);}
__attribute__((visibility("default"))) void sweetgl_ir3_cache_stats(uint64_t out[6])
{if(out)for(unsigned i=0;i<6;++i)out[i]=atomic_load(&counters[i]);}
