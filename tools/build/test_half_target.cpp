/* Actual ESSL half adaptation: nested constructors and packed names. */
#include "../../gpu/metal/air/HalfESSL.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(int argc,char **argv)
{
    assert(argc==2);std::ofstream out(argv[1],std::ios::binary);assert(out);out<<sweet_half_shader;
    std::string text="#version 310 es\n#extension GL_AMD_gpu_shader_half_float : require\nuint _q_index(uint i,uint s){return i;}\n"
        "float16_t value=float16_t(1.00049);f16vec4 v=f16vec4(float16_t(value),f16vec2(value),float16_t(0));"
        "vec2 bits=unpackFloat2x16(packFloat2x16(f16vec2(value)));\n";
    std::string error;assert(SweetAIR::halfESSL(text,error));assert(text.find("AMD_gpu")==std::string::npos && text.find("f16vec")==std::string::npos && text.find("float16_t")==std::string::npos);
    assert(text.find("_smtl_half_round(vec4(_smtl_half_round(float(value)),_smtl_half_round(vec2(value)),_smtl_half_round(float(0))))")!=std::string::npos);
    assert(text.find("_smtl_half_unpack2(_smtl_half_pack2(_smtl_half_round(vec2(value))))")!=std::string::npos);
    std::string bad="#version 310 es\n#extension GL_AMD_gpu_shader_half_float : require\nuint _q_index(uint i,uint s){return i;}\nfloat16_t v=float16_t(1;";
    assert(!SweetAIR::halfESSL(bad,error) && !error.empty());out.close();std::cout<<"actual half ESSL adapter: nested rounding, packed names, invalid constructor rejection PASS\n";return 0;
}
