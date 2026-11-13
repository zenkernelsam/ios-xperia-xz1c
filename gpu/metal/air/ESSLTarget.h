// SPDX-License-Identifier: BSD-2-Clause
// Keep AIR semantics and adapt only the declared target-language interface.
#pragma once
#include <string>
#include <array>
#include <cstdint>
#include <cctype>
#include <cstdio>
namespace SweetAIR {
inline void replaceToken(std::string &text,const std::string &from,const std::string &to)
{
    size_t position=0;auto word=[](char c){return std::isalnum((unsigned char)c) || c=='_';};
    while((position=text.find(from,position))!=std::string::npos){
        if((position && word(text[position-1])) || (position+from.size()<text.size() && word(text[position+from.size()])))
            {position+=from.size();continue;}
        text.replace(position,from.size(),to);position+=to.size();
    }
}
inline bool vertexWordTextures(std::string &text,unsigned bufferMask,std::string &error)
{
    // Mesa a5xx exposes zero vertex SSBOs. Sample original buffer pages through
    // integer textures, with explicit logical-word bounds before texelFetch.
    std::string header="uniform uvec2 _smtl_vertex_buffer_info[8];\n";
    for(unsigned slot=0;slot<8;++slot){const std::string number=std::to_string(slot),base="_qbuffer"+number+".data";
        if(text.find("_qhalfbuffer"+number)!=std::string::npos){error="Half vertex buffers need a separate packed conversion";return false;}
        const std::string declaration="layout(std430,binding="+number+") readonly buffer QBuffer"+number+" { uint data[]; } _qbuffer"+number+";\n";
        auto at=text.find(declaration);if(at!=std::string::npos)text.erase(at,declaration.size());
        if(!(bufferMask&(1u<<slot))){if(text.find(base)!=std::string::npos){error="AIR vertex buffer missing reflection";return false;}continue;}
        const std::string name="_smtl_vertex_buffer"+number;
        header+="uniform highp usampler2D "+name+";\nuint "+name+"_load(uint word){uvec2 span=_smtl_vertex_buffer_info["+number+"];"
            "if(word>=span.y)return 0u;uint index=span.x+word;return texelFetch("+name+",ivec2(index&4095u,index>>12u),0).r;}\n";
        const std::string access=base+"[";size_t cursor=0;
        while((at=text.find(access,cursor))!=std::string::npos){size_t end=at+access.size();unsigned depth=1;
            for(;end<text.size() && depth;++end){if(text[end]=='[')++depth;else if(text[end]==']')--depth;}
            if(depth){error="Unclosed actual AIR vertex load";return false;}
            auto index=text.substr(at+access.size(),end-at-access.size()-1);
            text.replace(at,end-at,name+"_load("+index+")");cursor=at+name.size()+6+index.size();}
        if(text.find(base)!=std::string::npos){error="Unsupported AIR vertex buffer use";return false;}
    }
    auto main=text.find("void main() {");if(main==std::string::npos){error="AIR graphics main missing";return false;}
    text.insert(main,header);return true;
}
template<class Reflection> inline bool graphicsESSL(const std::string &input,unsigned stage,const Reflection &reflection,
    std::string &output,std::string &error)
{
    output.clear();error.clear();if(stage!=1 && stage!=2){error="Vertex/fragment AIR required";return false;}
    if(input.rfind("#version 430\n",0)!=0){error="Expected actual AIR GLSL430";return false;}
    const std::string precision="precision highp float;\nprecision highp int;\nprecision highp sampler2D;\nprecision highp sampler2DArray;\n";
    auto body=input.substr(13);size_t prefix=0;
    while(body.compare(prefix,11,"#extension ")==0){auto end=body.find('\n',prefix);if(end==std::string::npos){error="Malformed AIR extension";return false;}prefix=end+1;}
    output="#version 310 es\n"+body.substr(0,prefix)+precision+body.substr(prefix);
    for(unsigned i=0;i<reflection.varyingCount;++i){auto &varying=reflection.varyings[i];char identity[80];
        if(varying.mode!=0 && varying.mode!=2 && varying.mode!=3){output.clear();error="AIR interpolation requires unsupported ESSL extension";return false;}
        snprintf(identity,sizeof(identity),"_smtl_vary_%08x%08x%08x%08x",varying.semantic[0],varying.semantic[1],varying.semantic[2],varying.semantic[3]);
        replaceToken(output,"_qvarying"+std::to_string(i),identity);
        std::string location="layout(location="+std::to_string(i)+") ";size_t start=0;
        while((start=output.find(location,start))!=std::string::npos){auto finish=output.find('\n',start);auto line=output.substr(start,finish-start);
            if(line.find(identity)!=std::string::npos){output.erase(start,location.size());break;}start+=location.size();}
    }
    // Independent Metal vertex/fragment buffer namespaces retain independent
    // shader block names and GL binding points after program linking.
    if(stage==2)for(unsigned i=0;i<8;++i){replaceToken(output,"_qbuffer"+std::to_string(i),"_qfragmentbuffer"+std::to_string(i));
        replaceToken(output,"QBuffer"+std::to_string(i),"QFragmentBuffer"+std::to_string(i));
        std::string binding="layout(std430,binding="+std::to_string(i)+")",replacement="layout(std430,binding="+std::to_string(i+8)+")";
        auto at=output.find(binding);if(at!=std::string::npos)output.replace(at,binding.size(),replacement);}
    size_t at=0;while((at=output.find("layout(binding=",at))!=std::string::npos){auto close=output.find(")",at),end=output.find('\n',at);
        if(close==std::string::npos || end==std::string::npos){output.clear();error="Malformed actual texture declaration";return false;}
        if(output.substr(close+1,end-close).find("uniform sampler")!=std::string::npos)output.erase(at,close+1-at);else at=close+1;}
    if(stage==1){if(!vertexWordTextures(output,reflection.bufferMask,error)){output.clear();return false;}
        auto end=output.rfind('}');if(end==std::string::npos){output.clear();error="AIR vertex body missing";return false;}
        // Metal0..1 clip depth and upper-left framebuffer rows mapped to GLES.
        output.insert(end,"gl_Position.y = -gl_Position.y;\ngl_Position.z = 2.0*gl_Position.z - gl_Position.w;\n");}
    return true;
}
inline bool computeESSL(const std::string &input,const std::array<unsigned,3> &local,std::string &output,std::string &error)
{
    output.clear();error.clear();uint64_t threads=1;
    for(unsigned dimension:local){if(!dimension || dimension>1024 || threads>1024/dimension){error="Invalid bounded local size";return false;}threads*=dimension;}
    if(input.rfind("#version 430\n",0)!=0){error="Expected the actual AIR frontend's GLSL430 output";return false;}
    output="#version 310 es\nprecision highp float;\nprecision highp int;\n"+input.substr(13);
    const std::string variable="#extension GL_ARB_compute_variable_group_size : require\nlayout(local_size_variable) in;\n";
    auto position=output.find(variable);if(position==std::string::npos){output.clear();error="Actual compute local-size declaration missing";return false;}
    output.replace(position,variable.size(),"layout(local_size_x="+std::to_string(local[0])+",local_size_y="+std::to_string(local[1])+",local_size_z="+std::to_string(local[2])+") in;\n");
    const std::string old="gl_LocalGroupSizeARB",replacement="gl_WorkGroupSize";
    for(position=0;(position=output.find(old,position))!=std::string::npos;position+=replacement.size())output.replace(position,old.size(),replacement);
    return true;
}
}
