/* Exercise the production reader against real files, never remove fixtures. */
#define _GNU_SOURCE 1
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int read_fault,attack_fd=-1;static unsigned reads;
static int replacement_fifo=-1;
static int test_openat(int fd,const char *name,int flags)
{
    if(replacement_fifo>=0 && !strcmp(name,"pipe-race.plist")){
        assert((flags&O_ACCMODE)==O_RDONLY && (flags&O_NONBLOCK) && (flags&O_NOFOLLOW));
        return dup(replacement_fifo);
    }
    return openat(fd,name,flags);
}
static ssize_t test_read(int fd,void *p,size_t n)
{
    ++reads;
    assert((fcntl(fd,F_GETFL)&O_ACCMODE)==O_RDONLY);
    if(read_fault==1){read_fault=0;errno=EINTR;return -1;}
    if(read_fault==2){errno=EIO;return -1;}
    ssize_t result=read(fd,p,n);
    if(attack_fd>=0 && result>0){struct stat st;assert(fstat(attack_fd,&st)==0);
        assert(pwrite(attack_fd,"!",1,0)==1 && ftruncate(attack_fd,st.st_size+1)==0);attack_fd=-1;}
    return result;
}
#define read test_read
#define openat test_openat
#include "../../gpu/diagnostics/PosterConfigSnapshot.h"
#undef read
#undef openat
static unsigned data_records,max_line;
static void logger(const char *format,...)
{
    char line[2048];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>=0 && n<511);if((unsigned)n>max_line)max_line=(unsigned)n;
    if(strstr(line,"SWEET PCFG DATA "))++data_records;
    puts(line);
}
static void directory(const char *path){assert(mkdir(path,0700)==0);}
static void path(char *out,const char *base,const char *name)
{assert(snprintf(out,512,"%s/%s",base,name)>0 && strlen(out)<511);}
static void put(const char *name,const void *data,size_t size)
{
    int fd=open(name,O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600);assert(fd>=0);
    assert(write(fd,data,size)==(ssize_t)size);assert(close(fd)==0);
}
static void unchanged(const char *name,const void *want,size_t size)
{
    unsigned char *p=malloc(size+1);assert(p);int fd=open(name,O_RDONLY|O_CLOEXEC);assert(fd>=0);
    assert(read(fd,p,size+1)==(ssize_t)size && !memcmp(p,want,size));close(fd);free(p);
}
static struct SweetPosterConfigSummary run(const char *name,const char *const *roots,const bool *filter,size_t count)
{
    printf("HOST PCFG %s BEGIN\n",name);data_records=0;reads=0;errno=ENOTTY;
    struct SweetPosterConfigSummary r=sweet_poster_config_snapshot_at(roots,filter,count,logger,NULL);
    assert(errno==ENOTTY);printf("HOST PCFG %s END\n",name);return r;
}
int main(int argc,char **argv)
{
    assert(argc==2 && argv[1][0]=='/');char base[512],prefs[512],nested[512],file[512],external[512],linkpath[512];
    path(base,argv[1],"normal");directory(base);path(prefs,argv[1],"prefs");directory(prefs);
    path(nested,base,"UUID");directory(nested);
    static const char json[]="{\"providerIdentifier\":\"com.apple.NanoUniverse.AegirProxyApp.AegirPoster\",\"configurationIdentifier\":\"fixture\"}";
    path(file,nested,"configuration.json");put(file,json,sizeof(json)-1);
    char json_path[512];strcpy(json_path,file);
    static const unsigned char binary[]={0x62,0x70,0x6c,0x69,0x73,0x74,0x30,0x30,0xd2,0x1,0x2,0x3,0x4,0x5f,0x10,0x17,0x63,0x6f,0x6e,0x66,0x69,0x67,0x75,0x72,0x61,0x74,0x69,0x6f,0x6e,0x49,0x64,0x65,0x6e,0x74,0x69,0x66,0x69,0x65,0x72,0x5f,0x10,0x12,0x70,0x72,0x6f,0x76,0x69,0x64,0x65,0x72,0x49,0x64,0x65,0x6e,0x74,0x69,0x66,0x69,0x65,0x72,0x57,0x66,0x69,0x78,0x74,0x75,0x72,0x65,0x5f,0x10,0x30,0x63,0x6f,0x6d,0x2e,0x61,0x70,0x70,0x6c,0x65,0x2e,0x4e,0x61,0x6e,0x6f,0x55,0x6e,0x69,0x76,0x65,0x72,0x73,0x65,0x2e,0x41,0x65,0x67,0x69,0x72,0x50,0x72,0x6f,0x78,0x79,0x41,0x70,0x70,0x2e,0x41,0x65,0x67,0x69,0x72,0x50,0x6f,0x73,0x74,0x65,0x72,0x8,0xd,0x27,0x3c,0x44,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x77};
    path(file,base,"configuration.archive");put(file,binary,sizeof(binary));
    path(file,base,"opaque.png");put(file,"PNG data must not be exported",29);
    path(file,base,"xml-config.plist");static const char xml[]="<?xml version=\"1.0\"?><plist><dict/></plist>";put(file,xml,sizeof(xml)-1);
    path(file,prefs,"com.apple.PosterBoard.plist");put(file,json,sizeof(json)-1);
    path(file,prefs,"unrelated.plist");put(file,json,sizeof(json)-1);
    path(external,argv[1],"external.json");put(external,"{\"external\":true}",17);
    path(linkpath,base,"symlink.json");assert(symlink(external,linkpath)==0);
    char missing[512];path(missing,argv[1],"missing");const char *roots[]={base,prefs,missing};bool filters[]={false,true,false};
    struct SweetPosterConfigSummary r=run("normal",roots,filters,3);
    assert(!r.result && r.opened==2 && r.files==4 && r.errors==0 && data_records>0);
    unchanged(json_path,json,sizeof(json)-1);unchanged(external,"{\"external\":true}",17);
    assert(lstat(linkpath,&(struct stat){0})==0); /* Link and FIFO remain untouched. */
    read_fault=1;r=run("EINTR",roots,filters,3);assert(!r.result && r.files==4 && !r.errors);
    read_fault=2;r=run("read_fault",roots,filters,3);assert(!r.result && r.files==0 && r.errors==4 && !data_records);read_fault=0;
    char race[512],racefile[512];path(race,argv[1],"race");directory(race);path(racefile,race,"current.plist");put(racefile,json,sizeof(json)-1);
    int writer=open(racefile,O_WRONLY|O_CLOEXEC);assert(writer>=0);attack_fd=writer;
    const char *race_roots[]={race};bool nofilter[]={false};r=run("race",race_roots,nofilter,1);
    assert(!r.result && !r.files && r.errors==1 && !data_records);close(writer);
    char fifo_case[512];path(fifo_case,argv[1],"fifo-race");directory(fifo_case);
    path(file,fifo_case,"pipe-race.plist");put(file,json,sizeof(json)-1);
    int pipes[2];assert(pipe(pipes)==0);assert(fcntl(pipes[0],F_SETFL,O_NONBLOCK)==0);replacement_fifo=pipes[0];
    const char *fifo_roots[]={fifo_case};r=run("fifo_race",fifo_roots,nofilter,1);
    assert(!r.result && !r.files && r.errors==1 && !data_records && !reads);
    replacement_fifo=-1;close(pipes[0]);close(pipes[1]);
    char root_link[512];path(root_link,argv[1],"root-link");assert(symlink(base,root_link)==0);
    const char *linked[]={root_link};r=run("root_link",linked,nofilter,1);assert(!r.result && !r.opened && r.errors==1 && !reads);
    char huge[512];path(huge,argv[1],"huge");directory(huge);unsigned char *large=calloc(1,PCFG_FILE_BYTES+1);assert(large);memcpy(large,"bplist00",8);
    path(file,huge,"oversize.plist");put(file,large,PCFG_FILE_BYTES+1);free(large);
    const char *huge_roots[]={huge};r=run("oversize",huge_roots,nofilter,1);assert(!r.result && !r.files && !reads && r.skipped==1);
    char limit[512];path(limit,argv[1],"limit");directory(limit);
    for(unsigned i=0;i<PCFG_ENTRIES+1;++i){char name[64];snprintf(name,sizeof(name),"item-%03u",i);path(file,limit,name);put(file,"x",1);}
    const char *limit_roots[]={limit};r=run("limit",limit_roots,nofilter,1);assert(r.result==-E2BIG && r.entries==PCFG_ENTRIES);
    const char *invalid[]={NULL};r=sweet_poster_config_snapshot_at(invalid,nofilter,1,logger,NULL);assert(r.result==-EINVAL);
    char escaped[64];pcfg_hex((const unsigned char *)"quoted\nname",11,escaped);assert(strstr(escaped,"0a"));
    printf("Poster config snapshot host PASS: read-only files, nofollow, FIFO, filter, EINTR, faults, race, bounds, max_line=%u\n",max_line);
    return 0;
}
