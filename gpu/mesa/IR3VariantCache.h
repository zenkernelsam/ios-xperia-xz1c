/* SPDX-License-Identifier: MIT
 * Pinned Freedreno store_variant layout, with checked temporary restoration.
 * The normal compiler and other Mesa backends keep their original path. */
#pragma once
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __APPLE__
#define LILAC_IR3_OPTIONAL __attribute__((weak_import))
#else
#define LILAC_IR3_OPTIONAL
#endif
extern bool sweetgl_ir3_cache_enabled(uint32_t) LILAC_IR3_OPTIONAL;
extern int sweetgl_ir3_cache_get(const void *,size_t,void **,size_t *) LILAC_IR3_OPTIONAL;
extern int sweetgl_ir3_cache_put(const void *,size_t,const void *,size_t) LILAC_IR3_OPTIONAL;
extern void sweetgl_ir3_cache_digest(const void *,size_t,unsigned char[32]) LILAC_IR3_OPTIONAL;
extern void sweetgl_ir3_cache_accepted(size_t) LILAC_IR3_OPTIONAL;
extern void sweetgl_ir3_cache_rejected(void) LILAC_IR3_OPTIONAL;

#define LILAC_IR3_MAX (8u<<20)
static bool lilac_ir3_enabled(struct ir3_compiler *compiler)
{
    return compiler->gen==5 && !(ir3_shader_debug & IR3_DBG_NOCACHE) &&
        sweetgl_ir3_cache_enabled && sweetgl_ir3_cache_get && sweetgl_ir3_cache_put &&
        sweetgl_ir3_cache_digest && sweetgl_ir3_cache_accepted && sweetgl_ir3_cache_rejected &&
        sweetgl_ir3_cache_enabled(fd_dev_gpu_id(compiler->dev_id));
}
static void lilac_ir3_shader_key(struct ir3_shader *shader)
{
    shader->lilac_key_valid=false;
    if(!lilac_ir3_enabled(shader->compiler))return;
    struct blob blob;blob_init(&blob);nir_serialize(&blob,shader->nir,true);
    blob_write_bytes(&blob,&shader->options,sizeof(shader->options));
    blob_write_bytes(&blob,&shader->stream_output,sizeof(shader->stream_output));
    blob_write_uint32(&blob,shader->type);
    if(!blob.out_of_memory && blob.size && blob.size<=(32u<<20)){
        sweetgl_ir3_cache_digest(blob.data,blob.size,shader->lilac_key);
        shader->lilac_key_valid=true;
    }blob_finish(&blob);
}
static bool lilac_ir3_variant_key(struct ir3_shader *shader,struct ir3_shader_variant *v,struct blob *key)
{
    if(!shader->lilac_key_valid || !lilac_ir3_enabled(shader->compiler))return false;
    blob_init(key);blob_write_uint32(key,1);blob_write_bytes(key,shader->lilac_key,32);
    blob_write_bytes(key,&v->key,sizeof(v->key));blob_write_uint32(key,v->type);
    blob_write_uint32(key,v->binning_pass);blob_write_uint32(key,v->binning!=NULL);
    blob_write_uint32(key,sizeof(*v));blob_write_uint32(key,VARIANT_CACHE_SIZE);
    blob_write_uint32(key,sizeof(*v->const_state));blob_write_uint32(key,sizeof(v->info));
    blob_write_uint32(key,fd_dev_gpu_id(shader->compiler->dev_id));blob_write_uint64(key,ir3_shader_debug);
    if(v->type==MESA_SHADER_COMPUTE || v->type==MESA_SHADER_KERNEL){
        blob_write_uint32(key,shader->cs.req_input_mem);blob_write_uint32(key,shader->cs.req_local_mem);
        blob_write_uint32(key,shader->cs.force_linear_dispatch);
    }
    if(key->out_of_memory){blob_finish(key);return false;}return true;
}
/* Serialize the original fixed variant suffix + binary + const_state +
 * immediates. Transient pointer values are not persisted or later reused. */
static bool lilac_ir3_store_one(struct blob *blob,const struct ir3_shader_variant *v)
{
    if(!v->bin || !v->info.size || v->info.size>LILAC_IR3_MAX)return false;
    struct ir3_shader_variant image=*v;image.info.data=NULL;
    if(!blob_write_bytes(blob,VARIANT_CACHE_PTR(&image),VARIANT_CACHE_SIZE) ||
       !blob_write_bytes(blob,v->bin,v->info.size))return false;
    if(!v->binning_pass){
        if(!v->const_state || v->const_state->immediates_size>LILAC_IR3_MAX/4 ||
           (v->const_state->immediates_size && !v->const_state->immediates))return false;
        struct ir3_const_state state=*v->const_state;state.immediates=NULL;
        for(unsigned i=0;i<ARRAY_SIZE(state.ubo_state.range);++i){
            if(state.ubo_state.range[i].ubo.global)return false;
            state.ubo_state.range[i].ubo.global_base=NULL;
        }
        if(!blob_write_bytes(blob,&state,sizeof(state)) ||
           !blob_write_bytes(blob,v->const_state->immediates,(size_t)state.immediates_size*4))return false;
    }return !blob->out_of_memory;
}
static bool lilac_ir3_valid_variant(struct ir3_shader_variant *v,const struct ir3_shader_variant *expected,size_t remaining)
{
    return !v->info.data && v->info.size && v->info.size<=remaining && v->info.size<=LILAC_IR3_MAX &&
        v->info.sizedwords && (size_t)v->info.sizedwords*4<=v->info.size &&
        v->info.constant_data_offset<=v->info.size && v->constant_data_size<=v->info.size-v->info.constant_data_offset &&
        v->outputs_count<=ARRAY_SIZE(v->outputs) && v->inputs_count<=ARRAY_SIZE(v->inputs) &&
        v->num_samp>=0 && v->num_samp<=32 && v->num_ssbos<=32 && v->num_ibos<=32 &&
        v->num_sampler_prefetch<=ARRAY_SIZE(v->sampler_prefetch) && v->total_in<=128 && v->sysval_in<=128 &&
        v->constlen<=4096 && v->branchstack<=128 &&
        !memcmp(&v->shader_options,&expected->shader_options,sizeof(v->shader_options)) &&
        !memcmp(&v->stream_output,&expected->stream_output,sizeof(v->stream_output));
}
static bool lilac_ir3_load_one(struct blob_reader *reader,const struct ir3_shader_variant *expected,
    struct ir3_shader_variant *loaded,void *temporary)
{
    *loaded=*expected;memset(VARIANT_CACHE_PTR(loaded),0,VARIANT_CACHE_SIZE);
    blob_copy_bytes(reader,VARIANT_CACHE_PTR(loaded),VARIANT_CACHE_SIZE);
    if(reader->overrun || !lilac_ir3_valid_variant(loaded,expected,(size_t)(reader->end-reader->current)))return false;
    loaded->bin=rzalloc_size(temporary,loaded->info.size);if(!loaded->bin)return false;
    blob_copy_bytes(reader,loaded->bin,loaded->info.size);if(reader->overrun)return false;
    if(!loaded->binning_pass){
        struct ir3_const_state state;memset(&state,0,sizeof(state));blob_copy_bytes(reader,&state,sizeof(state));
        if(reader->overrun || state.immediates || state.immediates_size>(size_t)(reader->end-reader->current)/4 ||
           state.immediates_count>state.immediates_size || state.num_ubos>32 || state.num_app_ubos>32 || state.ubo_state.num_enabled>ARRAY_SIZE(state.ubo_state.range))return false;
        for(unsigned i=0;i<ARRAY_SIZE(state.ubo_state.range);++i)
            if(state.ubo_state.range[i].ubo.global_base || state.ubo_state.range[i].ubo.global)return false;
        loaded->const_state=rzalloc_size(temporary,sizeof(state));if(!loaded->const_state)return false;
        *loaded->const_state=state;size_t immediate_bytes=(size_t)state.immediates_size*4;
        if(immediate_bytes){loaded->const_state->immediates=ralloc_size(loaded->const_state,immediate_bytes);
            if(!loaded->const_state->immediates)return false;
            blob_copy_bytes(reader,loaded->const_state->immediates,immediate_bytes);}
    }return !reader->overrun;
}
static void lilac_ir3_commit_one(struct ir3_shader_variant *v,struct ir3_shader_variant *loaded)
{
    memcpy(VARIANT_CACHE_PTR(v),VARIANT_CACHE_PTR(loaded),VARIANT_CACHE_SIZE);v->info.data=v;
    ralloc_steal(v,loaded->bin);v->bin=loaded->bin;
    if(!v->binning_pass){ralloc_free(v->const_state);ralloc_steal(v,loaded->const_state);v->const_state=loaded->const_state;}
}
static bool lilac_ir3_retrieve(struct ir3_shader *shader,struct ir3_shader_variant *v)
{
    struct blob key;if(!lilac_ir3_variant_key(shader,v,&key))return false;
    void *payload=NULL;size_t bytes=0;int result=sweetgl_ir3_cache_get(key.data,key.size,&payload,&bytes);blob_finish(&key);
    if(result)return false;
    void *temporary=ralloc_context(NULL);struct ir3_shader_variant loaded,binning;
    struct blob_reader reader;blob_reader_init(&reader,payload,bytes);
    bool valid=temporary && lilac_ir3_load_one(&reader,v,&loaded,temporary);
    if(valid && v->binning)valid=lilac_ir3_load_one(&reader,v->binning,&binning,temporary);
    valid=valid && !reader.overrun && reader.current==reader.end;
    if(valid){
        lilac_ir3_commit_one(v,&loaded);if(v->binning)lilac_ir3_commit_one(v->binning,&binning);
        sweetgl_ir3_cache_accepted(bytes);
    }else sweetgl_ir3_cache_rejected();
    ralloc_free(temporary);free(payload);return valid;
}
static void lilac_ir3_store(struct ir3_shader *shader,struct ir3_shader_variant *v)
{
    struct blob key;if(!lilac_ir3_variant_key(shader,v,&key))return;
    struct blob payload;blob_init(&payload);bool valid=lilac_ir3_store_one(&payload,v);
    if(valid && v->binning)valid=lilac_ir3_store_one(&payload,v->binning);
    if(valid && !payload.out_of_memory && payload.size<=LILAC_IR3_MAX)
        (void)sweetgl_ir3_cache_put(key.data,key.size,payload.data,payload.size);
    blob_finish(&payload);blob_finish(&key);
}
