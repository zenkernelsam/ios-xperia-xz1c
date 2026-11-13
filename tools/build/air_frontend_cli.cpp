// Emit the actual frontend result for host verification and differential tests.
#include "Compiler.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <iomanip>
static void emitQuoted(const std::string &text)
{
    std::cout<<'"';for(unsigned char c:text){if(c=='"'||c=='\\')std::cout<<'\\'<<c;
        else if(c=='\n')std::cout<<"\\n";else if(c=='\r')std::cout<<"\\r";else if(c=='\t')std::cout<<"\\t";
        else if(c<32)std::cout<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::dec;
        else std::cout<<c;}std::cout<<'"';
}
int main(int argc,char **argv)
{
    if(argc<2 || argc>3)return 2;std::ifstream input(argv[1],std::ios::binary);if(!input)return 3;
    std::string bytes{std::istreambuf_iterator<char>(input),{}},error;
    std::vector<GPU::Compiler::Entry> entries;
    if(!GPU::Compiler::lowerAIR(bytes.data(),bytes.size(),entries,error,argc==3?argv[2]:"")){std::cerr<<error<<'\n';return 1;}
    std::cout<<'[';for(unsigned i=0;i<entries.size();++i){auto &e=entries[i];if(i)std::cout<<',';
        std::cout<<"{\"name\":";emitQuoted(e.name);std::cout<<",\"stage\":"<<e.reflection.stage;
        std::cout<<",\"buffer_mask\":"<<e.reflection.bufferMask<<",\"write_mask\":"<<e.reflection.writeMask;
        std::cout<<",\"glsl\":";emitQuoted(e.glsl);std::cout<<'}';}
    std::cout<<"]\n";return 0;
}
