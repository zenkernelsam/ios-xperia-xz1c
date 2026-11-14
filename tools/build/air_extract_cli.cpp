/* Extract exact original AIR sections using the pinned OSS MTLB parser. */
#include "../../gpu/metal/air/reference/LibraryArchive.h"
#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cctype>
int main(int argc,char **argv)
{
    if(argc!=3)return 2;std::ifstream input(argv[1],std::ios::binary);if(!input)return 3;
    std::string bytes{std::istreambuf_iterator<char>(input),{}},error;std::vector<GPU::Compiler::LibraryArchive::Function> entries;
    if(!GPU::Compiler::LibraryArchive::parse(bytes.data(),bytes.size(),entries,error)){std::cerr<<error<<'\n';return 4;}
    for(const auto &entry:entries){
        if(entry.name.empty())return 5;for(unsigned char c:entry.name)if(!std::isalnum(c) && c!='_')return 5;
        unsigned char hash[32];CC_SHA256(bytes.data()+entry.offset,(CC_LONG)entry.size,hash);if(std::memcmp(hash,entry.hash.data(),32))return 6;
        std::ofstream output(std::string(argv[2])+"/"+entry.name+".air.bc",std::ios::binary);output.write(bytes.data()+entry.offset,entry.size);if(!output)return 7;
        char hex[65]={0};for(unsigned i=0;i<32;++i)std::snprintf(hex+2*i,3,"%02x",hash[i]);
        std::cout<<entry.name<<" type="<<entry.type<<" offset="<<entry.offset<<" bytes="<<entry.size<<" sha256="<<hex<<'\n';
    }return 0;
}
