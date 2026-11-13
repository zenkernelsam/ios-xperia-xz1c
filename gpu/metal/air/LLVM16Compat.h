// SPDX-License-Identifier: BSD-2-Clause
// The target iOS16 exposes the older, typed-pointer LLVM C interface.
#pragma once
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <dlfcn.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

namespace SweetAIR {
inline void *optionalLLVM(const char *name)
{
#if defined(SWEET_LLVM16_FORCE_FALLBACK)
    (void)name;return nullptr;
#else
    void *result=dlsym(RTLD_DEFAULT,name);
#if defined(__APPLE__)
    if(!result){void *module=dlopen("/usr/lib/libLLVM.dylib",RTLD_LAZY|RTLD_NOLOAD);
        if(module){result=dlsym(module,name);dlclose(module);}}
#endif
    return result;
#endif
}
inline LLVMTypeRef globalValueType(LLVMValueRef value)
{
    using Function=LLVMTypeRef(*)(LLVMValueRef);static auto original=(Function)optionalLLVM("LLVMGlobalGetValueType");
    if(original)return original(value);
    auto type=LLVMTypeOf(value);if(LLVMGetTypeKind(type)!=LLVMPointerTypeKind)throw std::runtime_error("AIR16 global needs typed pointer");
    return LLVMGetElementType(type);
}
inline LLVMTypeRef gepSourceType(LLVMValueRef value)
{
    using Function=LLVMTypeRef(*)(LLVMValueRef);static auto original=(Function)optionalLLVM("LLVMGetGEPSourceElementType");
    if(original)return original(value);
    auto type=LLVMTypeOf(LLVMGetOperand(value,0));if(LLVMGetTypeKind(type)!=LLVMPointerTypeKind)throw std::runtime_error("AIR16 GEP needs typed pointer");
    return LLVMGetElementType(type);
}
inline LLVMBool isPoison(LLVMValueRef value)
{
    using Function=LLVMBool(*)(LLVMValueRef);static auto original=(Function)optionalLLVM("LLVMIsPoison");
    if(original)return original(value);
    if(!LLVMIsAConstant(value))return false;
    char *printed=LLVMPrintValueToString(value);std::string text=printed?printed:"";LLVMDisposeMessage(printed);
    const std::string marker=" poison";
    return text.size()>=marker.size() && text.compare(text.size()-marker.size(),marker.size(),marker)==0;
}
inline LLVMValueRef poison(LLVMTypeRef type)
{
    using Function=LLVMValueRef(*)(LLVMTypeRef);static auto original=(Function)optionalLLVM("LLVMGetPoison");
    if(original)return original(type);
    throw std::runtime_error("AIR16 LLVM cannot construct poison; original semantics retained as a compile error");
}
inline int shuffleMask(LLVMValueRef value,unsigned lane)
{
    using Function=int(*)(LLVMValueRef,unsigned);static auto original=(Function)optionalLLVM("LLVMGetMaskValue");
    if(original)return original(value,lane);
    if(LLVMGetNumOperands(value)==2){
        // Newer host LLVM stores the immutable mask outside the operand list.
        // Read that same instruction's printed constant, with exact type/count
        // and all element bounds checked. The iOS16 three-operand path is below.
        unsigned lanes=LLVMGetVectorSize(LLVMTypeOf(value)),width=LLVMGetVectorSize(LLVMTypeOf(LLVMGetOperand(value,0)));
        if(!lanes || lanes>4 || lane>=lanes)throw std::runtime_error("AIR16 shuffle lane bound");
        char *printed=LLVMPrintValueToString(value);std::string text=printed?printed:"";LLVMDisposeMessage(printed);
        std::string marker=", <"+std::to_string(lanes)+" x i32> ";auto at=text.rfind(marker);
        if(at==std::string::npos)throw std::runtime_error("AIR16 shuffle mask constant missing");
        auto mask=text.substr(at+marker.size());if(mask=="undef" || mask=="poison")return -1;
        if(mask=="zeroinitializer")return 0;
        if(mask.size()<2 || mask.front()!='<' || mask.back()!='>')throw std::runtime_error("AIR16 malformed shuffle mask");
        mask=mask.substr(1,mask.size()-2);unsigned selected=0;int result=-1;
        for(unsigned index=0;index<lanes;++index){auto comma=mask.find(',');std::string element=mask.substr(0,comma);
            auto first=element.find_first_not_of(' ');element=first==std::string::npos?"":element.substr(first);
            if(element.rfind("i32 ",0)!=0)throw std::runtime_error("AIR16 shuffle element type mismatch");
            element=element.substr(4);int number=-1;
            if(element!="undef" && element!="poison"){char *end=nullptr;unsigned long value_=std::strtoul(element.c_str(),&end,10);
                if(!end || *end || element.empty() || value_>=2*width)throw std::runtime_error("AIR16 shuffle value out of bound");number=(int)value_;}
            if(index==lane){result=number;++selected;}
            if(index+1<lanes){if(comma==std::string::npos)throw std::runtime_error("AIR16 truncated shuffle mask");mask=mask.substr(comma+1);}
            else if(comma!=std::string::npos)throw std::runtime_error("AIR16 extra shuffle elements");
        }
        if(selected!=1)throw std::runtime_error("AIR16 shuffle selection absent");return result;
    }
    if(LLVMGetNumOperands(value)!=3)throw std::runtime_error("AIR16 shuffle needs original mask operand");
    auto mask=LLVMGetOperand(value,2);auto type=LLVMTypeOf(mask);
    if(LLVMGetTypeKind(type)!=LLVMVectorTypeKind || lane>=LLVMGetVectorSize(type))throw std::runtime_error("AIR16 shuffle mask out of range");
    if(LLVMIsUndef(mask) || isPoison(mask))return -1;
    auto index=LLVMConstInt(LLVMInt32TypeInContext(LLVMGetTypeContext(type)),lane,false);
    auto element=LLVMConstExtractElement(mask,index);
    if(LLVMIsUndef(element) || isPoison(element))return -1;
    if(!LLVMIsAConstantInt(element))throw std::runtime_error("AIR16 shuffle mask is not a constant integer");
    auto width=LLVMGetVectorSize(LLVMTypeOf(LLVMGetOperand(value,0)));auto n=LLVMConstIntGetZExtValue(element);
    if(n>=2*width)throw std::runtime_error("AIR16 shuffle mask exceeds both inputs");return (int)n;
}
inline LLVMAtomicRMWBinOp atomicOperation(LLVMValueRef value)
{
    using Function=LLVMAtomicRMWBinOp(*)(LLVMValueRef);static auto original=(Function)optionalLLVM("LLVMGetAtomicRMWBinOp");
    if(original)return original(value);
    char *printed=LLVMPrintValueToString(value);std::string text=printed?printed:"";LLVMDisposeMessage(printed);
    auto start=text.find("atomicrmw ");if(start==std::string::npos)throw std::runtime_error("AIR16 atomicrmw instruction absent");
    start+=10;if(text.compare(start,9,"volatile ")==0)start+=9;
    auto end=text.find(' ',start);auto operation=text.substr(start,end-start);
    const char *names[]={"xchg","add","sub","and","nand","or","xor","max","min","umax","umin","fadd","fsub"};
    for(unsigned i=0;i<13;++i)if(operation==names[i])return (LLVMAtomicRMWBinOp)i;
    throw std::runtime_error("AIR16 unknown atomicrmw operation");
}
struct PassError {std::string message;};
inline LLVMPassBuilderOptionsRef passOptions(){return nullptr;}
inline void disposePassOptions(LLVMPassBuilderOptionsRef options){if(options)throw std::runtime_error("Unexpected AIR16 pass options");}
inline LLVMErrorRef runPasses(LLVMModuleRef module,const char *pipeline,LLVMTargetMachineRef target,LLVMPassBuilderOptionsRef options)
{
    if(target || options)throw std::runtime_error("AIR16 pass adapter requires target-neutral module optimization");
    auto manager=LLVMCreatePassManager();
    if(!std::strcmp(pipeline,"function(sccp,adce)")){LLVMAddSCCPPass(manager);LLVMAddAggressiveDCEPass(manager);}
    else if(!std::strcmp(pipeline,"function(instcombine,simplifycfg,adce)")){
        LLVMAddInstructionCombiningPass(manager);LLVMAddCFGSimplificationPass(manager);LLVMAddAggressiveDCEPass(manager);}
    else if(!std::strcmp(pipeline,"always-inline,function(sroa,sccp,adce)")){
        LLVMAddAlwaysInlinerPass(manager);LLVMAddScalarReplAggregatesPassSSA(manager);LLVMAddSCCPPass(manager);LLVMAddAggressiveDCEPass(manager);}
    else {LLVMDisposePassManager(manager);return (LLVMErrorRef)new PassError{"AIR16 unsupported pass pipeline"};}
    LLVMRunPassManager(manager,module);LLVMDisposePassManager(manager);
    char *diagnostic=nullptr;
    if(LLVMVerifyModule(module,LLVMReturnStatusAction,&diagnostic)){
        auto error=new PassError{diagnostic?diagnostic:"AIR16 invalid optimized module"};LLVMDisposeMessage(diagnostic);return (LLVMErrorRef)error;}
    LLVMDisposeMessage(diagnostic);return nullptr;
}
inline char *errorMessage(LLVMErrorRef error)
{auto state=(PassError *)error;char *message=strdup(state->message.c_str());delete state;return message;}
inline void disposeErrorMessage(char *message){free(message);}
}

#define LLVMGlobalGetValueType SweetAIR::globalValueType
#define LLVMGetGEPSourceElementType SweetAIR::gepSourceType
#define LLVMIsPoison SweetAIR::isPoison
#define LLVMGetPoison SweetAIR::poison
#define LLVMGetMaskValue SweetAIR::shuffleMask
#define LLVMGetAtomicRMWBinOp SweetAIR::atomicOperation
#define LLVMCreatePassBuilderOptions SweetAIR::passOptions
#define LLVMDisposePassBuilderOptions SweetAIR::disposePassOptions
#define LLVMRunPasses SweetAIR::runPasses
#define LLVMGetErrorMessage SweetAIR::errorMessage
#define LLVMDisposeErrorMessage SweetAIR::disposeErrorMessage
