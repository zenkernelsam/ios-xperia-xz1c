// SPDX-License-Identifier: BSD-2-Clause
// Reapply current half lowering/XR wrapping to a local, verified old request.
// The caller verifies record SHA256/context/key and current observed hashes.
#include "../../gpu/metal/air/HalfESSL.h"
#include "../../gpu/sweetgl/XRBlendShader.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>

static std::string read(const char *path)
{std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("input unavailable");return {std::istreambuf_iterator<char>(input),{}};}
static void write(const std::string &path,const std::string &bytes)
{std::ofstream output(path,std::ios::binary);output<<bytes;if(!output)throw std::runtime_error("output unavailable");}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    try {
        std::string request=read(argv[1]),old=read(argv[2]),error;
        if(request.size()<136+sizeof(SweetRenderPipelineConfig) || request.compare(0,8,"SGPB0001"))return 3;
        auto word=[&](size_t at){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t((unsigned char)request[at+i])<<(8*i);return v;};
        const uint32_t vs=word(24),fs=word(28),cn=word(32);
        if(word(8)!=1 || word(12)!=1 || word(16) || word(36) || cn!=sizeof(SweetRenderPipelineConfig) ||
           uint64_t(cn)+vs+fs!=word(20) || request.size()!=136+uint64_t(cn)+vs+fs)return 3;
        SweetRenderPipelineConfig config;memcpy(&config,request.data()+136,sizeof(config));
        std::string vertex=request.substr(136+cn,vs),fragment=request.substr(136+cn+vs,fs);
        if(vertex.find('\0')!=std::string::npos || fragment.find('\0')!=std::string::npos || config.reserved)return 3;
        size_t at=old.find("uint _smtl_half_pack(");if(at==std::string::npos)return 3;old.erase(0,at);
        at=fragment.find(old);if(at==std::string::npos || fragment.find(old,at+old.size())!=std::string::npos)return 3;
        std::string helper=sweet_half_shader;if(helper.front()=='\n')helper.erase(0,1);
        fragment.replace(at,old.size(),helper);
        if(!SweetAIR::foldHalfCalls(fragment,error)){std::cerr<<error<<'\n';return 4;}
        uint32_t mask=sweet_xr_ordered_mask(&config);
        char *v=mask?sweet_xr_ordered_vertex(vertex.c_str()):nullptr,*f=mask?sweet_xr_ordered_fragment(fragment.c_str(),&config,mask):nullptr;
        if(mask && (!v || !f)){free(v);free(f);return 4;}
        std::string result_v=v?v:vertex,result_f=f?f:fragment;free(v);free(f);
        write(std::string(argv[3])+"/replayed.vert",result_v);write(std::string(argv[3])+"/replayed.frag",result_f);
        std::cout<<"RENDER_REQUEST replay vertex_bytes="<<result_v.size()<<" fragment_bytes="<<result_f.size()<<" mask="<<mask<<" device_operations=0\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
