// SPDX-License-Identifier: BSD-2-Clause
// Keep the upstream reader byte-exact; isolate the older LLVM C enum here.
#include <llvm-c/Core.h>
#if defined(SWEET_LLVM_PRE23)
#define LLVMUncondBr ((LLVMOpcode)70)
#define LLVMCondBr ((LLVMOpcode)71)
#endif
#include "LLVM16Compat.h"
#include "reference/AIRRuntime.cpp"
