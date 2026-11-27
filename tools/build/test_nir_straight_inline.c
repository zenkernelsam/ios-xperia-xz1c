/* SPDX-License-Identifier: BSD-2-Clause
 * Exact pinned NIR inlining/serialization on authored parameter, nested-call,
 * branch and repeated-store graphs. No GL context or device access.
 */
#include "nir.h"
#include "nir_builder.h"
#include "nir_serialize.h"
#include "util/blob.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* The release host archive omits the debug flag. Link a separate real debug
 * validator with checks enabled; zero disables no validation categories. */
uint32_t nir_debug=0;

static nir_function *function(nir_shader *shader,const char *name)
{
    nir_function *f=nir_function_create(shader,name);
    f->num_params=1;f->params=rzalloc_array(shader,nir_parameter,1);
    f->params[0].num_components=1;f->params[0].bit_size=32;
    nir_function_impl_create(f);return f;
}
static void call(nir_builder *b,nir_function *f,nir_def *value)
{
    nir_call_instr *c=nir_call_instr_create(b->shader,f);
    c->params[0]=nir_src_for_ssa(value);nir_builder_instr_insert(b,&c->instr);
}
static void store(nir_builder *b,nir_def *value)
{nir_store_ssbo(b,value,nir_imm_int(b,0),nir_imm_int(b,0),.write_mask=1,.align_mul=4,.align_offset=0);}
int main(int argc,char **argv)
{
    assert(argc==2);unsigned count=(unsigned)strtoul(argv[1],NULL,10);assert(count>0 && count<=16384);
    static const nir_shader_compiler_options options={0};
    glsl_type_singleton_init_or_ref();
    nir_builder b=nir_builder_init_simple_shader(MESA_SHADER_VERTEX,&options,"authored-inline");
    nir_function *leaf=function(b.shader,"leaf"),*nested=function(b.shader,"nested"),*branch=function(b.shader,"branch");
    nir_builder c=nir_builder_create(leaf->impl);c.cursor=nir_after_cf_list(&c.impl->body);
    nir_def *x=nir_load_param(&c,0);store(&c,nir_iadd_imm(&c,x,7));
    c=nir_builder_create(nested->impl);c.cursor=nir_after_cf_list(&c.impl->body);
    x=nir_load_param(&c,0);call(&c,leaf,x);store(&c,nir_imul_imm(&c,x,3));
    c=nir_builder_create(branch->impl);c.cursor=nir_after_cf_list(&c.impl->body);
    x=nir_load_param(&c,0);nir_push_if(&c,nir_ine_imm(&c,x,0));call(&c,nested,x);
    nir_push_else(&c,NULL);store(&c,nir_imm_int(&c,19));nir_pop_if(&c,NULL);
    for(unsigned i=0;i<count;++i){nir_def *value=nir_iadd_imm(&b,nir_load_vertex_id(&b),(int)i);
        call(&b,(i%17)==0?branch:nested,value);}
    nir_validate_shader(b.shader,"before authored inline");
    assert(nir_inline_functions(b.shader));nir_validate_shader(b.shader,"after authored inline");
    nir_function_impl *impl=nir_shader_get_entrypoint(b.shader);unsigned stores=0,calls=0;
    nir_foreach_block(block,impl)nir_foreach_instr(instr,block){
        calls+=instr->type==nir_instr_type_call;
        if(instr->type==nir_instr_type_intrinsic)stores+=nir_instr_as_intrinsic(instr)->intrinsic==nir_intrinsic_store_ssbo;
    }
    unsigned branches=(count+16)/17;assert(!calls && stores==2*count+branches);
    struct blob bytes;blob_init(&bytes);nir_serialize(&bytes,b.shader,false);
    assert(fwrite(bytes.data,1,bytes.size,stdout)==bytes.size);
    fprintf(stderr,"NIR_INLINE calls=%u remaining=0 stores=%u branches=%u serialized=%zu validated=1\n",count,stores,branches,bytes.size);
    blob_finish(&bytes);ralloc_free(b.shader);glsl_type_singleton_decref();return 0;
}
