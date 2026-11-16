/* SPDX-License-Identifier: BSD-2-Clause
 * Rebuild the exact production XR wrapper around an exported runtime request.
 * Only local diagnostic files are written; no GL/device commands are issued. */
#include "../../gpu/sweetgl/ProgramCacheCodec.h"
#include "../../gpu/sweetgl/XRBlendShader.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t fnv(const char *text)
{uint64_t hash=UINT64_C(14695981039346656037);for(const unsigned char *p=(const unsigned char *)text;*p;++p)hash=(hash^*p)*UINT64_C(1099511628211);return hash;}
static int save(const char *prefix,const char *suffix,const char *source)
{
    char path[1100];int length=snprintf(path,sizeof(path),"%s%s",prefix,suffix);
    if(length<=0 || length>=(int)sizeof(path))return -1;
    FILE *f=fopen(path,"wx");if(!f)return -1;
    size_t bytes=strlen(source);int result=fwrite(source,1,bytes,f)==bytes?0:-1;
    if(fclose(f))result=-1;return result;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f){perror("request open");return 2;}
    if(fseek(f,0,SEEK_END)){fclose(f);return 2;}long count=ftell(f);
    if(count<136 || count>131072 || fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    unsigned char *record=malloc((size_t)count);if(!record){fclose(f);return 2;}
    int read_ok=fread(record,1,(size_t)count,f)==(size_t)count && fgetc(f)==EOF;fclose(f);
    if(!read_ok || !sweet_cache_validate(record,(size_t)count,1,record+40,NULL)){free(record);return 3;}
    SweetRenderPipelineConfig config;memcpy(&config,record+136,sizeof(config));
    uint32_t vs=sweet_cache_get32(record+24),fs=sweet_cache_get32(record+28);
    char *vertex=calloc((size_t)vs+1,1),*fragment=calloc((size_t)fs+1,1);
    if(!vertex || !fragment){free(record);free(vertex);free(fragment);return 3;}
    memcpy(vertex,record+136+sizeof(config),vs);memcpy(fragment,record+136+sizeof(config)+vs,fs);
    unsigned char key[32];sweet_cache_key(record+40,&config,vertex,vs,fragment,fs,key);
    if(memcmp(key,record+72,32)){free(record);free(vertex);free(fragment);return 3;}
    uint32_t mask=sweet_xr_ordered_mask(&config);
    char *wrapped_vertex=mask?sweet_xr_ordered_vertex(vertex):NULL;
    char *wrapped_fragment=mask?sweet_xr_ordered_fragment(fragment,&config,mask):NULL;
    int result=0;
    if(mask && (!wrapped_vertex || !wrapped_fragment))result=4;
    const char *actual_vertex=mask?wrapped_vertex:vertex,*actual_fragment=mask?wrapped_fragment:fragment;
    if(!result && (save(argv[2],".wrapped.vert",actual_vertex) || save(argv[2],".wrapped.frag",actual_fragment))){perror("exclusive output");result=5;}
    if(!result)printf("{\"request_vertex_bytes\":%u,\"request_fragment_bytes\":%u,\"vertex_fnv64\":\"%016" PRIx64 "\",\"fragment_fnv64\":\"%016" PRIx64 "\",\"ordered_xr_mask\":%u,\"wrapped_vertex_bytes\":%zu,\"wrapped_fragment_bytes\":%zu,\"wrapped_vertex_fnv64\":\"%016" PRIx64 "\",\"wrapped_fragment_fnv64\":\"%016" PRIx64 "\",\"device_operations\":0}\n",vs,fs,fnv(vertex),fnv(fragment),mask,strlen(actual_vertex),strlen(actual_fragment),fnv(actual_vertex),fnv(actual_fragment));
    free(record);free(vertex);free(fragment);free(wrapped_vertex);free(wrapped_fragment);return result;
}
