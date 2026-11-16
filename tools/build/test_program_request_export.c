/* SPDX-License-Identifier: BSD-2-Clause; real read-only exporter fault tests. */
#include "../../gpu/diagnostics/ProgramRequestExport.h"
#include <assert.h>
#include <stdarg.h>

static unsigned char captured[2][131072];
static size_t captured_bytes[2];
static unsigned begins,ends;
static void capture(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);assert(n>0 && n<=180);
    unsigned index;size_t offset,count;char hex[257];
    if(sscanf(line,"SWEET PROGRAM REQUEST data index=%u offset=%zu bytes=%zu hex=%256s",&index,&offset,&count,hex)==4){
        assert(index<2 && offset==captured_bytes[index] && count<=32 && strlen(hex)==2*count && offset+count<=sizeof(captured[index]));
        for(size_t i=0;i<count;++i){unsigned value;assert(sscanf(hex+2*i,"%2x",&value)==1);captured[index][offset+i]=(unsigned char)value;}
        captured_bytes[index]+=count;
    }else if(!strncmp(line,"SWEET PROGRAM REQUEST begin ",28))++begins;
    else if(!strncmp(line,"SWEET PROGRAM REQUEST END ",26))++ends;
}
static void reset_capture(void){begins=ends=0;memset(captured_bytes,0,sizeof(captured_bytes));}
static void directory(char out[128]){strcpy(out,"/tmp/lilac-request-export-XXXXXX");assert(mkdtemp(out));}
static unsigned char *fixture(const unsigned char identity[32],unsigned fs,size_t *count)
{
    assert(sizeof(SweetRenderPipelineConfig)==316);SweetRenderPipelineConfig config={0};
    size_t payload=sizeof(config)+4840+fs;unsigned char *record=calloc(136+payload,1);assert(record);unsigned char *data=record+136;
    memcpy(data,&config,sizeof(config));memset(data+sizeof(config),'v',4840);memset(data+sizeof(config)+4840,'f',fs);
    unsigned char key[32];sweet_cache_key(identity,&config,(char *)data+sizeof(config),4840,(char *)data+sizeof(config)+4840,fs,key);
    assert(sweet_cache_header(record,1,0,identity,key,data,payload,4840,fs));*count=136+payload;return record;
}
static void save(const char *root,const unsigned char *record,size_t bytes,char path[512])
{
    char id[33],key[65];sweet_request_hex(record+40,16,id);sweet_request_hex(record+72,32,key);
    snprintf(path,512,"%s/sgpc-%s-%s.req",root,id,key);int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600);assert(fd>=0);
    size_t offset=0;while(offset<bytes){ssize_t written=write(fd,record+offset,bytes-offset);assert(written>0);offset+=(size_t)written;}assert(!close(fd));
}
int main(void)
{
    unsigned char identity[32]={1};size_t n[2];unsigned char *records[2]={fixture(identity,86136,&n[0]),fixture(identity,45773,&n[1])};char root[128],path[512];
    directory(root);save(root,records[0],n[0],path);save(root,records[1],n[1],path);reset_capture();assert(!sweet_request_export_at(root,identity,capture));
    assert(begins==2 && ends==2);
    for(unsigned i=0;i<2;++i){assert(captured_bytes[i]==n[i] || captured_bytes[i]==n[1-i]);unsigned which=captured_bytes[i]==n[0]?0:1;assert(!memcmp(captured[i],records[which],n[which]));}
    /* Full payload corruption, wrong filename, truncation and symlink are refused. */
    directory(root);unsigned char *bad=malloc(n[0]);assert(bad);memcpy(bad,records[0],n[0]);bad[n[0]-1]^=1;save(root,bad,n[0],path);reset_capture();assert(sweet_request_export_at(root,identity,capture)<0 && begins==0);
    directory(root);save(root,records[0],n[0]-1,path);reset_capture();assert(sweet_request_export_at(root,identity,capture)<0 && begins==0);
    directory(root);save(root,records[0],n[0],path);char renamed[512];snprintf(renamed,sizeof(renamed),"%s/sgpc-01000000000000000000000000000000-%064d.req",root,0);assert(!rename(path,renamed));reset_capture();assert(sweet_request_export_at(root,identity,capture)<0 && begins==0);
    directory(root);save(root,records[0],n[0],path);char target[512];strcpy(target,path);strcat(target,".saved");assert(!rename(path,target) && !symlink(target,path));reset_capture();assert(sweet_request_export_at(root,identity,capture)<0 && begins==0);
    directory(root);save(root,records[0],n[0],path);reset_capture();assert(sweet_request_export_at(root,identity,capture)<0 && begins==1 && ends==1);
    printf("PASS: all %zu bytes reconstructed; corrupt/truncated/named/symlink/missing records refused; original fixtures retained\n",n[0]+n[1]);free(records[0]);free(records[1]);free(bad);return 0;
}
