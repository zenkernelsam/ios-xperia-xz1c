/* SPDX-License-Identifier: GPL-2.0-only
 * Separately compiled C11 caller: all lifecycle invocations cross the C ABI. */
#include "LilacPcmRuntimeCTest.h"
_Static_assert(LILAC_PCM_RUNTIME_API_VERSION==1,"C ABI version");
_Static_assert(sizeof(uint32_t)==4 && sizeof(uint64_t)==8,"C ABI integer widths");
size_t c_pcm_bytes(void) {return lilac_pcm_runtime_context_bytes();}
size_t c_pcm_alignment(void) {return lilac_pcm_runtime_context_alignment();}
uint32_t c_pcm_construct(void *p,size_t n,const struct lilac_pcm_runtime_config *c,
                         const struct lilac_pcm_runtime_callbacks *o)
{return lilac_pcm_runtime_construct(p,n,c,o);}
uint32_t c_pcm_start(void *p,size_t n) {return lilac_pcm_runtime_start(p,n);}
uint32_t c_pcm_advance(void *p,size_t n) {return lilac_pcm_runtime_advance(p,n);}
enum lilac_asm_pcm_queue_result c_pcm_queue(void *p,size_t n,uint32_t offset,uint32_t span,
                                           const void *data,uint32_t bytes,uint32_t *token)
{return lilac_pcm_runtime_queue(p,n,offset,span,data,bytes,token);}
uint32_t c_pcm_drain(void *p,size_t n) {return lilac_pcm_runtime_drain(p,n);}
uint32_t c_pcm_status(void *p,size_t n,struct lilac_pcm_runtime_snapshot *out)
{return lilac_pcm_runtime_status(p,n,out);}
uint32_t c_pcm_uncertain(void *p,size_t n) {return lilac_pcm_runtime_uncertain(p,n);}
uint32_t c_pcm_destroy(void *p,size_t n) {return lilac_pcm_runtime_destroy(p,n);}
