/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool sweetgl_ir3_cache_enabled(uint32_t gpu);
int sweetgl_ir3_cache_get(const void *key,size_t key_bytes,void **payload,size_t *bytes);
int sweetgl_ir3_cache_put(const void *key,size_t key_bytes,const void *payload,size_t bytes);
void sweetgl_ir3_cache_accepted(size_t bytes);
void sweetgl_ir3_cache_rejected(void);
void sweetgl_ir3_cache_digest(const void *data,size_t bytes,unsigned char out[32]);
__attribute__((visibility("default"))) void sweetgl_ir3_cache_stats(uint64_t out[6]);
#ifdef __cplusplus
}
#endif
