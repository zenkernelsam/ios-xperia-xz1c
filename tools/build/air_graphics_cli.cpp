// Materialize actual AIR->ESSL for independent host GLES compiler execution.
#include "Compiler.h"
#include "../../gpu/metal/air/ESSLTarget.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char **argv)
{
    if(argc!=3 && argc!=4)return 2;std::ifstream input(argv[1],std::ios::binary);if(!input)return 3;
    std::string bytes{std::istreambuf_iterator<char>(input),{}},error;std::vector<GPU::Compiler::Entry> entries;
    if(!GPU::Compiler::lowerAIR(bytes.data(),bytes.size(),entries,error,argc==4?argv[3]:"")){std::cerr<<error<<'\n';return 1;}
    for(const auto &entry:entries){std::string output;bool translated=entry.reflection.stage==0?
            SweetAIR::computeESSL(entry.glsl,{64,1,1},output,error):SweetAIR::graphicsESSL(entry.glsl,entry.reflection.stage,entry.reflection,output,error);
        if(!translated){std::cerr<<error<<'\n';return 1;}
        auto filename=std::string(argv[2])+"/"+entry.name+(entry.reflection.stage==0?".comp":entry.reflection.stage==1?".vert":".frag");
        std::ofstream file(filename,std::ios::binary);file<<output;if(!file)return 4;std::cout<<filename<<'\n';}
    return 0;
}
