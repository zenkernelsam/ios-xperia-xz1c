// SPDX-License-Identifier: BSD-2-Clause
// Keep AIR semantics and adapt only the declared target-language interface.
#pragma once
#include <string>
#include <array>
namespace SweetAIR {
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
