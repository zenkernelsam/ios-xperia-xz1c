/* SPDX-License-Identifier: BSD-2-Clause; no existing file/directory is removed. */
#include "../../gpu/diagnostics/ProgramCacheScope.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    char root[]="/tmp/lilac-cache-scope-XXXXXX";assert(mkdtemp(root));
    char directory[160],other[160],linkpath[160],private[160],file[180];
    snprintf(directory,sizeof(directory),"%s/current",root);snprintf(other,sizeof(other),"%s/other",root);
    snprintf(linkpath,sizeof(linkpath),"%s/link",root);snprintf(private,sizeof(private),"%s/private",root);
    assert(!unsetenv("SWEETGL_PROGRAM_CACHE_DIR"));int inherited=-1;
    assert(!sweet_program_cache_scope_at(directory,&inherited) && inherited==0);
    struct stat st;assert(!lstat(directory,&st) && (st.st_mode&07777)==01777);
    snprintf(file,sizeof(file),"%s/retained",directory);FILE *f=fopen(file,"wx");assert(f);assert(fputs("original",f)>=0 && !fclose(f));
    assert(!sweet_program_cache_scope_at(directory,&inherited) && inherited==1);
    f=fopen(file,"rb");assert(f);char content[9]={0};assert(fread(content,1,8,f)==8 && !strcmp(content,"original"));fclose(f);
    assert(!setenv("SWEETGL_PROGRAM_CACHE_DIR",other,1));
    assert(sweet_program_cache_scope_at(directory,&inherited)==-EINVAL && lstat(other,&st)<0 && errno==ENOENT);
    assert(!unsetenv("SWEETGL_PROGRAM_CACHE_DIR") && !symlink(directory,linkpath));
    assert(sweet_program_cache_scope_at(linkpath,&inherited)==-EINVAL);
    assert(!mkdir(private,0700));assert(sweet_program_cache_scope_at(private,&inherited)==-EPERM);
    assert(!lstat(private,&st) && (st.st_mode&07777)==0700);
    puts("PASS: new sticky shared root, exact inherited root, retained file, foreign override/symlink/mode refusal; fixtures retained");return 0;
}
