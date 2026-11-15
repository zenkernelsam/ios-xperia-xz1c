// SPDX-License-Identifier: BSD-2-Clause
// Host diagnostics with typed representative constants, not runtime UI proof.
#include "Compiler.h"
#include "../../gpu/metal/air/ESSLTarget.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc,char **argv)
{
    if(argc!=3 && argc!=4)return 2;
    const bool all=argc==4 && std::string(argv[3])=="all";
    if(argc==4 && !all)return 2;
    std::ifstream input(argv[1],std::ios::binary);if(!input)return 3;
    std::string bytes{std::istreambuf_iterator<char>(input),{}},error;
    std::vector<GPU::Compiler::FunctionConstant> declarations;
    unsigned maximum=0;
    if(!GPU::Compiler::inspectAIRConstants(bytes.data(),bytes.size(),argv[2],declarations,maximum,error)){
        std::cerr<<error<<'\n';return 1;
    }
    std::vector<GPU::Compiler::FunctionConstantValue> constants;
    for(const auto &entry:declarations){
        if(!all && !entry.required)continue;
        GPU::Compiler::FunctionConstantValue value;value.index=entry.index;value.type=entry.type;
        auto shape=GPU::Compiler::constantShape(value.type);
        if(!shape.bits || !shape.lanes)return 4;
        uint64_t one=shape.floating?(shape.bits==16?UINT64_C(0x3c00):UINT64_C(0x3f800000)):1;
        for(unsigned lane=0;lane<shape.lanes;++lane)
            for(unsigned n=0;n<shape.bits/8;++n)value.bytes[lane*(shape.bits/8)+n]=uint8_t(one>>(8*n));
        constants.push_back(value);
        std::cerr<<"representative_constant index="<<entry.index<<" type="<<entry.type<<" required="<<entry.required<<'\n';
    }
    std::vector<GPU::Compiler::Entry> entries;
    if(!GPU::Compiler::lowerAIR(bytes.data(),bytes.size(),entries,error,argv[2],constants)){
        std::cerr<<error<<'\n';return 1;
    }
    if(entries.size()!=1 || entries[0].name!=argv[2])return 5;
    const auto &entry=entries[0];std::string essl;
    bool translated=entry.reflection.stage==0?SweetAIR::computeESSL(entry.glsl,{1,1,1},essl,error):
        SweetAIR::graphicsESSL(entry.glsl,entry.reflection.stage,entry.reflection,essl,error);
    if(!translated){std::cerr<<error<<'\n';return 1;}
    if(const char *output=std::getenv("SWEET_AIR_OUTPUT")){
        std::ofstream file(std::string(output)+"/"+entry.name+(entry.reflection.stage==0?".comp":entry.reflection.stage==1?".vert":".frag"),std::ios::binary);
        file<<essl;if(!file)return 6;
    }
    std::cout<<"AIR representative specialization PASS entry="<<entries[0].name
             <<" source_bytes="<<entries[0].glsl.size()<<" ESSL_bytes="<<essl.size()<<" constants="<<constants.size()
             <<" runtime_values_not_inferred=1\n";
    return 0;
}
