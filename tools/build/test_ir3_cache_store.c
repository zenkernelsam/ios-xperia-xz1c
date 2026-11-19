/* Real file/record boundaries, faulted bytes and fresh-process restoration. */
#define _GNU_SOURCE 1
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include "../../gpu/sweetgl/ir3_cache.c"
void sweet_drm_log(const char *format,...){(void)format;}
int main(int argc,char **argv)
{
    assert(argc==2);assert(!setenv("SWEETGL_PROGRAM_CACHE_DIR",argv[1],1));
    unsigned char key[128],input[1024];for(unsigned i=0;i<sizeof(key);++i)key[i]=(unsigned char)(i*17+3);
    for(unsigned i=0;i<sizeof(input);++i)input[i]=(unsigned char)(i*29+9);
    assert(sweetgl_ir3_cache_enabled(540) && !sweetgl_ir3_cache_enabled(530));
    assert(!setenv("SWEETGL_IR3_CACHE_DISABLE","1",1) && !sweetgl_ir3_cache_enabled(540));assert(!unsetenv("SWEETGL_IR3_CACHE_DISABLE"));
    void *data=(void *)1;size_t bytes=9;errno=ERANGE;
    assert(sweetgl_ir3_cache_get(key,sizeof(key),&data,&bytes)==-ENOENT && !data && !bytes && errno==ERANGE);
    assert(!sweetgl_ir3_cache_put(key,sizeof(key),input,sizeof(input)) && errno==ERANGE);
    assert(sweetgl_ir3_cache_put(key,sizeof(key),input,sizeof(input))==-EEXIST && errno==ERANGE);
    pid_t child=fork();assert(child>=0);if(!child){assert(!sweetgl_ir3_cache_get(key,sizeof(key),&data,&bytes));assert(bytes==sizeof(input) && !memcmp(data,input,bytes));free(data);_exit(0);}
    int status;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(!sweetgl_ir3_cache_get(key,sizeof(key),&data,&bytes) && bytes==sizeof(input) && !memcmp(data,input,bytes));free(data);
    char path[1024];unsigned char hashed[32];assert(path_for(key,sizeof(key),path,hashed));
    int file=open(path,O_RDWR|O_NOFOLLOW);assert(file>=0);unsigned char original[SWEET_IR3_HEADER+1024];assert(read(file,original,sizeof(original))==sizeof(original));close(file);
    unsigned char bad[sizeof(original)];memcpy(bad,original,sizeof(bad));bad[48]^=1;assert(!sweet_ir3_validate(bad,sizeof(bad),sweet_program_cache_build_id,hashed));
    memcpy(bad,original,sizeof(bad));bad[16]^=1;assert(!sweet_ir3_validate(bad,sizeof(bad),sweet_program_cache_build_id,hashed));
    memcpy(bad,original,sizeof(bad));bad[80]^=1;assert(!sweet_ir3_validate(bad,sizeof(bad),sweet_program_cache_build_id,hashed));
    assert(!sweet_ir3_validate(original,sizeof(original)-1,sweet_program_cache_build_id,hashed));
    memcpy(bad,original,sizeof(bad));sweet_cache_put32(bad+12,SWEET_IR3_MAX+1);assert(!sweet_ir3_validate(bad,sizeof(bad),sweet_program_cache_build_id,hashed));
    file=open(path,O_WRONLY|O_NOFOLLOW);assert(file>=0);unsigned char changed=original[sizeof(original)-1]^1;assert(pwrite(file,&changed,1,sizeof(original)-1)==1);close(file);
    assert(sweetgl_ir3_cache_get(key,sizeof(key),&data,&bytes)==-EINVAL && !data && !bytes && errno==ERANGE);
    assert(sweetgl_ir3_cache_put(key,sizeof(key),input,sizeof(input))==-EEXIST);
    key[0]^=1;assert(sweetgl_ir3_cache_get(key,sizeof(key),&data,&bytes)==-ENOENT);key[0]^=1;
    sweetgl_ir3_cache_accepted(sizeof(input));uint64_t stats[6];sweetgl_ir3_cache_stats(stats);assert(stats[0]==2 && stats[1]==1 && stats[2]==1 && stats[3]>=1 && stats[4]>=sizeof(input));
    printf("IR3 record/file PASS exact_build_key_digest=1 fresh_process=1 corruption_truncation_refused=1 original_files_preserved=1 errno_preserved=1 stats_valid=1\n");return 0;
}
