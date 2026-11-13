// Real legacy C APIs and preserved poison/shuffle/atomic semantics.
#include "LLVM16Compat.h"
#include <llvm-c/IRReader.h>
#include <cassert>
#include <cstring>
#include <iostream>
static LLVMModuleRef module(const char *source,LLVMContextRef context)
{
    LLVMModuleRef result=nullptr;char *error=nullptr;auto input=LLVMCreateMemoryBufferWithMemoryRangeCopy(source,strlen(source),"fixture");
    assert(!LLVMParseIRInContext(context,input,&result,&error));if(error)LLVMDisposeMessage(error);return result;
}
int main()
{
    auto context=LLVMContextCreate();auto m=module("target datalayout = \"e-p:64:64:64\"\n@a = internal constant i32 7\ndefine i32 @f(i32* %p) {\n %q = getelementptr i32, i32* %p, i32 1\n %r = atomicrmw add i32* %p, i32 1 monotonic\n ret i32 %r\n}\ndefine <2 x i32> @g(<2 x i32> %a, <2 x i32> %b) {\n %s = shufflevector <2 x i32> %a, <2 x i32> %b, <2 x i32> <i32 2, i32 1>\n ret <2 x i32> %s\n}",context);
    auto global=LLVMGetNamedGlobal(m,"a");assert(SweetAIR::globalValueType(global)==LLVMInt32TypeInContext(context));
    auto f=LLVMGetNamedFunction(m,"f");auto instruction=LLVMGetFirstInstruction(LLVMGetFirstBasicBlock(f));
    assert(SweetAIR::gepSourceType(instruction)==LLVMInt32TypeInContext(context));
    assert(SweetAIR::atomicOperation(LLVMGetNextInstruction(instruction))==LLVMAtomicRMWBinOpAdd);
    auto g=LLVMGetNamedFunction(m,"g");auto shuffle=LLVMGetFirstInstruction(LLVMGetFirstBasicBlock(g));
    assert(SweetAIR::shuffleMask(shuffle,0)==2 && SweetAIR::shuffleMask(shuffle,1)==1);
    assert(!SweetAIR::isPoison(LLVMGetUndef(LLVMInt32TypeInContext(context))));
    auto error=SweetAIR::runPasses(m,"function(sccp,adce)",nullptr,nullptr);assert(!error);
    error=SweetAIR::runPasses(m,"function(instcombine,simplifycfg,adce)",nullptr,nullptr);assert(!error);
    error=SweetAIR::runPasses(m,"unsupported-pipeline",nullptr,nullptr);assert(error);char *message=SweetAIR::errorMessage(error);
    assert(strstr(message,"unsupported"));SweetAIR::disposeErrorMessage(message);
    LLVMDisposeModule(m);LLVMContextDispose(context);std::cout<<"actual LLVM16 adapters: typed globals/GEP, shuffle, atomic, pipelines and errors PASS\n";return 0;
}
