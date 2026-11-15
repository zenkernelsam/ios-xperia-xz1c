// SPDX-License-Identifier: BSD-2-Clause
// Constant-memory aliasing of one complete binary16 leaf; no numeric conversion.
#pragma once
#include <llvm-c/Core.h>
#include <cstdint>

namespace SweetAIR {
inline LLVMValueRef staticHalfBits(LLVMValueRef initial,uint64_t offset,
    uint64_t storageBytes,uint64_t accessBytes,LLVMTypeRef target,LLVMValueRef replacement=nullptr)
{
    if(!initial || !target || offset || storageBytes!=2 || accessBytes!=2)return nullptr;
    LLVMTypeRef source=LLVMTypeOf(initial);
    const auto half=[](LLVMTypeRef t){return LLVMGetTypeKind(t)==LLVMHalfTypeKind;};
    const auto word=[](LLVMTypeRef t){return LLVMGetTypeKind(t)==LLVMIntegerTypeKind && LLVMGetIntTypeWidth(t)==16;};
    if(!((half(source) && word(target)) || (word(source) && half(target))))return nullptr;
    if(!LLVMIsAConstant(initial) || (replacement && (!LLVMIsAConstant(replacement) || LLVMTypeOf(replacement)!=target)))return nullptr;
    return LLVMConstBitCast(replacement?replacement:initial,replacement?source:target);
}
}
