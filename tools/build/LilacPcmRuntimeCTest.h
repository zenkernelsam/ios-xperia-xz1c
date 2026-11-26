/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LILAC_PCM_RUNTIME_C_TEST_H
#define LILAC_PCM_RUNTIME_C_TEST_H
#include "../../xnu/support/LilacPcmRuntimeAPI.h"
#ifdef __cplusplus
extern "C" {
#endif
size_t c_pcm_bytes(void);
size_t c_pcm_alignment(void);
uint32_t c_pcm_construct(void *,size_t,const struct lilac_pcm_runtime_config *,
                         const struct lilac_pcm_runtime_callbacks *);
uint32_t c_pcm_start(void *,size_t);
uint32_t c_pcm_advance(void *,size_t);
enum lilac_asm_pcm_queue_result c_pcm_queue(void *,size_t,uint32_t,uint32_t,
                                           const void *,uint32_t,uint32_t *);
uint32_t c_pcm_drain(void *,size_t);
uint32_t c_pcm_status(void *,size_t,struct lilac_pcm_runtime_snapshot *);
uint32_t c_pcm_uncertain(void *,size_t);
uint32_t c_pcm_destroy(void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
