/* Real pinned IR3 structs/blob/ralloc, including failed partial restore. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "nir_serialize.h"
#include "ir3_compiler.h"
#include "ir3_nir.h"
#include "ir3_shader.h"
#include "lilac_ir3_variant_cache.h"

enum ir3_shader_debug ir3_shader_debug;
static void *saved_payload;static size_t saved_bytes;
static unsigned accepted,rejected;
bool sweetgl_ir3_cache_enabled(uint32_t gpu){return gpu==540;}
int sweetgl_ir3_cache_get(const void *key,size_t key_bytes,void **payload,size_t *bytes)
{(void)key;(void)key_bytes;*payload=malloc(saved_bytes);assert(*payload);memcpy(*payload,saved_payload,saved_bytes);*bytes=saved_bytes;return 0;}
int sweetgl_ir3_cache_put(const void *key,size_t key_bytes,const void *payload,size_t bytes)
{(void)key;(void)key_bytes;(void)payload;(void)bytes;return 0;}
void sweetgl_ir3_cache_digest(const void *p,size_t n,unsigned char out[32]){(void)p;(void)n;memset(out,1,32);}
void sweetgl_ir3_cache_accepted(size_t bytes){assert(bytes==saved_bytes);++accepted;}
void sweetgl_ir3_cache_rejected(void){++rejected;}
static void initialize(struct ir3_shader_variant *v,struct ir3_compiler *compiler,const struct ir3_shader_options *options)
{
    memset(v,0,sizeof(*v));v->compiler=compiler;v->type=MESA_SHADER_VERTEX;v->shader_options=*options;
    v->const_state=rzalloc_size(v,sizeof(*v->const_state));assert(v->const_state);
}
int main(void)
{
    const struct fd_dev_id id={.gpu_id=540};struct ir3_compiler compiler={.gen=5,.dev_id=&id};
    struct ir3_shader_options options={0};struct ir3_shader shader={.compiler=&compiler,.type=MESA_SHADER_VERTEX,.lilac_key_valid=true};memset(shader.lilac_key,0x18,32);
    struct ir3_shader_variant *original=rzalloc_size(NULL,sizeof(*original));assert(original);initialize(original,&compiler,&options);
    original->info.size=128;original->info.sizedwords=32;original->info.data=original;original->constlen=8;
    original->bin=rzalloc_size(original,128);assert(original->bin);for(unsigned i=0;i<32;++i)original->bin[i]=i*0x10203+7;
    original->const_state->immediates_size=4;original->const_state->immediates_count=4;
    original->const_state->immediates=ralloc_size(original->const_state,16);assert(original->const_state->immediates);
    for(unsigned i=0;i<4;++i)original->const_state->immediates[i]=0xa5a50000+i;
    original->binning=rzalloc_size(original,sizeof(*original));assert(original->binning);initialize(original->binning,&compiler,&options);
    original->binning->binning_pass=true;original->binning->nonbinning=original;original->binning->info.size=64;original->binning->info.sizedwords=16;
    original->binning->info.data=original->binning;original->binning->bin=rzalloc_size(original->binning,64);assert(original->binning->bin);memset(original->binning->bin,0x3c,64);
    struct blob blob;blob_init(&blob);assert(lilac_ir3_store_one(&blob,original) && lilac_ir3_store_one(&blob,original->binning));
    saved_payload=blob.data;saved_bytes=blob.size;
    struct ir3_shader_variant *result=rzalloc_size(NULL,sizeof(*result));initialize(result,&compiler,&options);
    result->binning=rzalloc_size(result,sizeof(*result));initialize(result->binning,&compiler,&options);result->binning->binning_pass=true;result->binning->nonbinning=result;
    assert(lilac_ir3_retrieve(&shader,result) && accepted==1 && !rejected);
    assert(!memcmp(original->bin,result->bin,128) && !memcmp(original->binning->bin,result->binning->bin,64));
    assert(!memcmp(original->const_state->immediates,result->const_state->immediates,16));
    assert(result->info.data==result && result->binning->info.data==result->binning && result->binning->nonbinning==result);
    struct ir3_shader_variant *failed=rzalloc_size(NULL,sizeof(*failed));initialize(failed,&compiler,&options);
    failed->binning=rzalloc_size(failed,sizeof(*failed));initialize(failed->binning,&compiler,&options);failed->binning->binning_pass=true;failed->binning->nonbinning=failed;
    struct ir3_const_state *old_state=failed->const_state;size_t original_bytes=saved_bytes;saved_bytes--;
    assert(!lilac_ir3_retrieve(&shader,failed) && rejected==1 && failed->const_state==old_state && !failed->bin && !failed->binning->bin);
    saved_bytes=original_bytes;unsigned char *copy=malloc(saved_bytes);assert(copy);memcpy(copy,saved_payload,saved_bytes);
    struct ir3_info info;memcpy(&info,copy,sizeof(info));info.size=UINT32_MAX;memcpy(copy,&info,sizeof(info));saved_payload=copy;
    assert(!lilac_ir3_retrieve(&shader,failed) && rejected==2 && failed->const_state==old_state && !failed->bin);
    info.size=128;info.data=(void *)1;memcpy(copy,&info,sizeof(info));assert(!lilac_ir3_retrieve(&shader,failed) && rejected==3);
    memcpy(copy,blob.data,saved_bytes);struct ir3_const_state state;
    memcpy(&state,copy+VARIANT_CACHE_SIZE+128,sizeof(state));state.immediates_size=UINT32_MAX;
    memcpy(copy+VARIANT_CACHE_SIZE+128,&state,sizeof(state));assert(!lilac_ir3_retrieve(&shader,failed) && rejected==4 && failed->const_state==old_state);
    free(copy);saved_payload=blob.data;
    ralloc_free(failed);ralloc_free(result);ralloc_free(original);blob_finish(&blob);
    puts("real IR3 serializer PASS original_structs_blob_ralloc=1 draw_binning_bytes_equal=1 pointer_rebased=1 truncated_partial_not_published=1 bounds_refused=1");return 0;
}
