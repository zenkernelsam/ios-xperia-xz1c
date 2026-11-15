// SPDX-License-Identifier: BSD-2-Clause
// Exhaustive binary16 storage aliasing through the actual LLVM C implementation.
#include "../../gpu/metal/air/StaticHalfBits.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"static half check line=%d\n",__LINE__);std::exit(1);}}while(0)

int main()
{
    auto context=LLVMContextCreate();auto half=LLVMHalfTypeInContext(context),word=LLVMInt16TypeInContext(context);
    auto zero=LLVMConstInt(word,0,false),halfZero=LLVMConstBitCast(zero,half);
    auto bits=[&](LLVMValueRef value){CHECK(value);auto result=LLVMConstBitCast(value,word);CHECK(LLVMIsAConstantInt(result));return LLVMConstIntGetZExtValue(result);};
    for(unsigned n=0;n<65536;++n){
        auto integer=LLVMConstInt(word,n,false),floating=LLVMConstBitCast(integer,half);
        auto read=SweetAIR::staticHalfBits(floating,0,2,2,word);CHECK(read && LLVMIsAConstantInt(read) && LLVMConstIntGetZExtValue(read)==n);
        CHECK(bits(SweetAIR::staticHalfBits(integer,0,2,2,half))==n);
        CHECK(bits(SweetAIR::staticHalfBits(halfZero,0,2,2,word,integer))==n);
        auto replaced=SweetAIR::staticHalfBits(zero,0,2,2,half,floating);CHECK(replaced && LLVMIsAConstantInt(replaced) && LLVMConstIntGetZExtValue(replaced)==n);
    }
    CHECK(!SweetAIR::staticHalfBits(halfZero,1,2,2,word));
    CHECK(!SweetAIR::staticHalfBits(halfZero,0,2,1,word));
    CHECK(!SweetAIR::staticHalfBits(halfZero,0,4,2,word));
    CHECK(!SweetAIR::staticHalfBits(halfZero,0,2,2,LLVMInt8TypeInContext(context)));
    CHECK(!SweetAIR::staticHalfBits(LLVMConstNull(LLVMFloatTypeInContext(context)),0,4,4,LLVMInt32TypeInContext(context)));
    CHECK(!SweetAIR::staticHalfBits(LLVMConstNull(LLVMVectorType(half,2)),0,4,4,LLVMInt32TypeInContext(context)));
    CHECK(!SweetAIR::staticHalfBits(halfZero,0,2,2,word,halfZero));
    LLVMContextDispose(context);
    std::puts("static half bits PASS bit_patterns=65536 read_write_paths=4 checks=262144 signed_zero_subnormal_infinity_all_NaN_payloads=1 offsets_sizes_wrong_types_vectors_rejected=1");
}
