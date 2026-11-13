/* Stable C boundary: only ESSL, owned buffer handles and bounded dispatch. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SweetMetalProgram SweetMetalProgram;
typedef struct SweetShaderBufferBinding {uint32_t handle,index;uint64_t offset,bytes;} SweetShaderBufferBinding;
#define SWEET_METAL_SHADER_API __attribute__((visibility("default")))
SWEET_METAL_SHADER_API SweetMetalProgram *sweetgl_metal_compute_create(const char *source,uint32_t buffer_mask,
    uint32_t write_mask,char *error,size_t error_bytes);
SWEET_METAL_SHADER_API int sweetgl_metal_compute_dispatch(SweetMetalProgram *program,const SweetShaderBufferBinding *bindings,
    uint32_t count,const uint32_t groups[3],const uint32_t local[3],const uint32_t grid[3],uint32_t *sequence);
SWEET_METAL_SHADER_API void sweetgl_metal_program_destroy(SweetMetalProgram *program);
#ifdef __cplusplus
}
#endif
