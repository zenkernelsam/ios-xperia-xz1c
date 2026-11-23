/* SPDX-License-Identifier: BSD-2-Clause; retained filesystem fixtures, no cleanup deletes. */
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include "../../gpu/diagnostics/ProgramCacheInventory.h"

static unsigned char identity[32];
static unsigned calls,mode;
static char active_root[1024],active_request[256];
static unsigned char active_key[32];
static uint32_t active_vs,active_fs;
static unsigned logged;
static void emit(const char *format,...)
{
    char line[256];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>=0 && n<=160);++logged;
    printf("%s\n",line);
}
static void path_join(char out[1200],const char *root,const char *name)
{int n=snprintf(out,1200,"%s/%s",root,name);assert(n>0 && n<1200);}
static void directory(const char *base,const char *name,char out[1024])
{
    int n=snprintf(out,1024,"%s/%s",base,name);assert(n>0 && n<1024);
    assert(!mkdir(out,01777) && !chmod(out,01777));
}
static void name_for(char out[256],const unsigned char key[32],const char *suffix)
{char id[65],hex[65];pcinv_hex(identity,32,id);pcinv_hex(key,32,hex);snprintf(out,256,"sgpc-%.32s-%s.%s",id,hex,suffix);}
static void create_file(const char *root,const char *name,const void *bytes,size_t count)
{
    char path[1200];path_join(path,root,name);int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0644);assert(fd>=0);
    size_t at=0;while(at<count){ssize_t n=write(fd,(const unsigned char *)bytes+at,count-at);assert(n>0);at+=(size_t)n;}
    assert(!close(fd));
}
static void request(const char *root,unsigned number,bool corrupt,bool wrong_name)
{
    char vertex[40],fragment[40];snprintf(vertex,sizeof(vertex),"author vertex %u",number);snprintf(fragment,sizeof(fragment),"author fragment %u",number);
    size_t vs=strlen(vertex),fs=strlen(fragment);SweetRenderPipelineConfig config;memset(&config,0,sizeof(config));
    unsigned char payload[sizeof(config)+80];memcpy(payload,&config,sizeof(config));memcpy(payload+sizeof(config),vertex,vs);memcpy(payload+sizeof(config)+vs,fragment,fs);
    unsigned char key[32],data[SWEET_PROGRAM_CACHE_HEADER+sizeof(payload)];sweet_cache_key(identity,&config,vertex,vs,fragment,fs,key);
    assert(sweet_cache_header(data,SWEET_PROGRAM_CACHE_REQUEST,0,identity,key,payload,sizeof(config)+vs+fs,vs,fs));
    memcpy(data+SWEET_PROGRAM_CACHE_HEADER,payload,sizeof(config)+vs+fs);
    if(corrupt)data[SWEET_PROGRAM_CACHE_HEADER+sizeof(config)]^=1;
    memcpy(active_key,key,32);active_vs=(uint32_t)vs;active_fs=(uint32_t)fs;
    if(wrong_name)key[0]^=1;
    name_for(active_request,key,"req");create_file(root,active_request,data,SWEET_PROGRAM_CACHE_HEADER+sizeof(config)+vs+fs);
}
static void binary(const char *root,bool wrong_lengths)
{
    static const unsigned char payload[]="Author binary fixture; not a Mesa import or GPU proof";
    unsigned char data[SWEET_PROGRAM_CACHE_HEADER+sizeof(payload)];
    assert(sweet_cache_header(data,SWEET_PROGRAM_CACHE_BINARY,0x875f,identity,active_key,payload,sizeof(payload),active_vs,active_fs+(wrong_lengths?1:0)));
    memcpy(data+SWEET_PROGRAM_CACHE_HEADER,payload,sizeof(payload));char name[256];name_for(name,active_key,"bin");create_file(root,name,data,sizeof(data));
}
static int primer(uint32_t *compiled,uint32_t *failed)
{
    ++calls;*compiled=mode?0:1;*failed=0;
    if(!mode)binary(active_root,false);
    return 0;
}
static void expect_rejected(const char *root)
{
    struct SweetCacheInventorySummary summary;errno=77;int result=sweet_cache_inventory_at(root,identity,'B',emit,&summary);
    assert(result && errno==77 && !summary.verified);
    unsigned previous=calls;assert(sweet_cache_prime_if_missing(&summary,primer,emit)==-EINVAL && calls==previous);
}
int main(int argc,char **argv)
{
    assert(argc==2);const char *base=argv[1];assert(!mkdir(base,0700));
    for(unsigned i=0;i<32;++i)identity[i]=(unsigned char)(17*i+3);
    struct SweetCacheInventorySummary before,after;char root[1024],path[1200],name[256];
    directory(base,"missing",active_root);request(active_root,1,false,false);
    puts("HOST PCINV successful conditional fixture BEGIN");
    errno=91;assert(!sweet_cache_inventory_at(active_root,identity,'B',emit,&before) && errno==91);
    assert(before.verified && before.requests==1 && before.missing==1 && !before.present);
    assert(!sweet_cache_prime_if_missing(&before,primer,emit) && calls==1 && errno==91);
    assert(!sweet_cache_inventory_at(active_root,identity,'A',emit,&after));
    assert(after.verified && after.requests==1 && !after.missing && after.present==1 && after.binaries==1);
    emit("SWEET PCINV COMPLETE pid=%d result=0 verified_before_after=1",getpid());
    puts("HOST PCINV successful conditional fixture END");
    unsigned prior=calls;
    puts("HOST PCINV successful skip fixture BEGIN");
    assert(!sweet_cache_inventory_at(active_root,identity,'B',emit,&before));
    errno=92;assert(!sweet_cache_prime_if_missing(&before,primer,emit) && calls==prior && errno==92);
    assert(!sweet_cache_inventory_at(active_root,identity,'A',emit,&after));
    emit("SWEET PCINV COMPLETE pid=%d result=0 verified_before_after=1",getpid());
    puts("HOST PCINV successful skip fixture END");

    directory(base,"empty",root);assert(!sweet_cache_inventory_at(root,identity,'B',emit,&before));
    assert(before.verified && !before.files && !before.requests && !before.missing);
    assert(!sweet_cache_prime_if_missing(&before,primer,emit) && calls==prior);
    create_file(root,"unrelated-prefix.bin","unchanged",9);
    assert(!sweet_cache_inventory_at(root,identity,'A',emit,&after) && !after.files);

    directory(base,"mode",root);assert(!chmod(root,0700));expect_rejected(root);
    char linked[1024];snprintf(linked,sizeof(linked),"%s/root-link",base);assert(!symlink(active_root,linked));expect_rejected(linked);
    directory(base,"symlink",root);name_for(name,active_key,"req");path_join(path,root,name);
    char source[1200];path_join(source,active_root,active_request);assert(!symlink(source,path));expect_rejected(root);
    directory(base,"fifo",root);path_join(path,root,name);assert(!mkfifo(path,0600));expect_rejected(root);

    directory(base,"digest",root);request(root,2,true,false);expect_rejected(root);
    directory(base,"key",root);request(root,3,false,true);expect_rejected(root);
    directory(base,"lengths",root);request(root,4,false,false);binary(root,true);expect_rejected(root);

    directory(base,"pending",root);request(root,5,false,false);name_for(name,active_key,"req.part-123-0");
    create_file(root,name,NULL,0);assert(!sweet_cache_inventory_at(root,identity,'B',emit,&before));
    assert(before.verified && before.pending==1 && before.records==1 && before.missing==1 && before.files==2);
    directory(base,"bad-name",root);char id[65];pcinv_hex(identity,32,id);snprintf(name,sizeof(name),"sgpc-%.32s-not-a-key.req",id);
    create_file(root,name,"not read",8);expect_rejected(root);

    directory(base,"bound",root);for(unsigned i=0;i<65;++i)request(root,i+100,false,false);expect_rejected(root);
    directory(base,"byte-bound",root);
    for(unsigned i=0;i<5;++i){unsigned char key[32]={0};key[0]=(unsigned char)i;name_for(name,key,"bin.part-123-0");path_join(path,root,name);
        int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0644);assert(fd>=0 && !ftruncate(fd,SWEET_PROGRAM_CACHE_HEADER+SWEET_PROGRAM_CACHE_MAX) && !close(fd));}
    expect_rejected(root);

    directory(base,"false-completed",active_root);request(active_root,700,false,false);assert(!sweet_cache_inventory_at(active_root,identity,'B',emit,&before));
    mode=1;prior=calls;assert(sweet_cache_prime_if_missing(&before,primer,emit)==-EIO && calls==prior+1);
    struct SweetCacheInventorySummary forged={0};assert(sweet_cache_prime_if_missing(&forged,primer,emit)==-EINVAL);
    forged=before;forged.missing=9;forged.requests=forged.present+9;assert(sweet_cache_prime_if_missing(&forged,primer,emit)==-EINVAL);
    prior=logged;assert(sweet_cache_inventory_at(root,identity,'X',emit,&before)==-EINVAL && logged==prior);
    puts("Cache inventory host PASS: conditional skip/prime, digest/key/lengths, nofollow/FIFO, mode, pending, file/byte bounds, errno, false completion; fixtures retained; GPU_imports=0");
    return 0;
}
