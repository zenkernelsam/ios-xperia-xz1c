// SPDX-License-Identifier: BSD-2-Clause
// Explicit typed constants for offline AIR graphics reproduction. No defaults.
// Arguments: AIR, entry, constants-table, output-file. Table: index type hex.
#include "Compiler.h"
#include "../../gpu/metal/air/ESSLTarget.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <set>

int main(int argc,char **argv)
{
    if(argc!=5)return 2;
    std::ifstream input(argv[1],std::ios::binary),table(argv[3]);if(!input || !table)return 3;
    std::string bytes{std::istreambuf_iterator<char>(input),{}},error,line;
    std::vector<GPU::Compiler::FunctionConstantValue> supplied,effective;
    std::set<uint32_t> indices;
    while(std::getline(table,line)){
        std::istringstream row(line);std::string hex,extra;
        GPU::Compiler::FunctionConstantValue value;
        if(!(row>>value.index>>value.type>>hex) || row>>extra || value.index>65535 || !indices.insert(value.index).second)return 4;
        unsigned size=GPU::Compiler::constantBytes(value.type);
        if(!size || size>value.bytes.size() || hex.size()!=size*2)return 4;
        auto nibble=[](char c)->int{if(c>='0' && c<='9')return c-'0';if(c>='a' && c<='f')return c-'a'+10;return -1;};
        for(unsigned i=0;i<size;++i){int a=nibble(hex[i*2]),b=nibble(hex[i*2+1]);if(a<0 || b<0)return 4;value.bytes[i]=uint8_t(a*16+b);}
        supplied.push_back(value);
    }
    std::vector<GPU::Compiler::FunctionConstant> declarations;unsigned stage=~0u;
    if(!GPU::Compiler::inspectAIRConstants(bytes.data(),bytes.size(),argv[2],declarations,stage,error) ||
       !GPU::Compiler::resolveFunctionConstants(declarations,supplied,effective,error)){
        std::cerr<<error<<'\n';return 1;
    }
    if(stage!=1 && stage!=2)return 5;
    std::vector<GPU::Compiler::Entry> entries;
    if(!GPU::Compiler::lowerAIR(bytes.data(),bytes.size(),entries,error,argv[2],effective) || entries.size()!=1){std::cerr<<error<<'\n';return 1;}
    std::string essl;
    if(!SweetAIR::graphicsESSL(entries[0].glsl,stage,entries[0].reflection,essl,error)){std::cerr<<error<<'\n';return 1;}
    std::ofstream output(argv[4],std::ios::binary);if(!output)return 3;output<<essl;if(!output)return 3;
    std::cout<<"AIR explicit constants entry="<<entries[0].name<<" supplied="<<supplied.size()<<" effective="<<effective.size()
             <<" source_bytes="<<entries[0].glsl.size()<<" ESSL_bytes="<<essl.size()<<" stage="<<stage<<" device_operations=0\n";
    return 0;
}
