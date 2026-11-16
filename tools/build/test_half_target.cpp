/* Actual ESSL half adaptation: nested constructors and packed names. */
#include "../../gpu/metal/air/HalfESSL.h"
#include <cassert>
#include <fstream>
#include <iostream>
static uint32_t literal_reference(uint16_t h)
{
    uint32_t sign=uint32_t(h&32768u)<<16u,mantissa=h&1023u;int exponent=(h>>10u)&31u;
    if(exponent==31)return sign|0x7f800000u|(mantissa<<13u);
    if(!exponent){if(!mantissa)return sign;exponent=1;while(!(mantissa&1024u)){mantissa<<=1u;--exponent;}mantissa&=1023u;}
    return sign|(uint32_t(exponent+112)<<23u)|(mantissa<<13u);
}
int main(int argc,char **argv)
{
    assert(argc==2);std::ofstream out(argv[1],std::ios::binary);assert(out);out<<sweet_half_shader;
    std::string text="#version 310 es\n#extension GL_AMD_gpu_shader_half_float : require\nuint _q_index(uint i,uint s){return i;}\n"
        "float16_t value=float16_t(1.00049);f16vec4 v=f16vec4(float16_t(value),f16vec2(value),float16_t(0));"
        "vec2 bits=unpackFloat2x16(packFloat2x16(f16vec2(value)));\n";
    std::string error;assert(SweetAIR::halfESSL(text,error));assert(text.find("AMD_gpu")==std::string::npos && text.find("f16vec")==std::string::npos && text.find("float16_t")==std::string::npos);
    assert(text.find("_smtl_half_round(vec4(_smtl_half_round(float(value)),_smtl_half_round(vec2(value)),float(0)))")!=std::string::npos);
    assert(text.find("_smtl_half_unpack2(_smtl_half_pack2(_smtl_half_round(vec2(value))))")!=std::string::npos);
    std::string bad="#version 310 es\n#extension GL_AMD_gpu_shader_half_float : require\nuint _q_index(uint i,uint s){return i;}\nfloat16_t v=float16_t(1;";
    assert(!SweetAIR::halfESSL(bad,error) && !error.empty());
    for(unsigned h=0;h<65536;++h){std::string call="_smtl_half_round(float(_smtl_half_unpack2("+std::to_string(h)+"u).x))";
        assert(SweetAIR::foldHalfCalls(call,error));assert(call=="uintBitsToFloat("+std::to_string(literal_reference(uint16_t(h)))+"u)");}
    std::string repeated="_smtl_half_round(_smtl_half_round(_smtl_half_round(vec4(value))))";
    assert(SweetAIR::foldHalfCalls(repeated,error) && repeated=="_smtl_half_round(vec4(value))");
    for(const char *carrier:{"float","vec2","vec3","vec4"})for(const char *value:{"0","1"}){
        std::string exact=std::string(carrier)+"("+value+")",call="_smtl_half_round("+exact+")";
        assert(SweetAIR::foldHalfCalls(call,error) && call==exact);}
    for(const char *unchanged:{"_smtl_half_round(_smtl_half_round(value)+other)",
        "_smtl_half_round(float(_smtl_half_unpack2(65536u).x))",
        "_smtl_half_round(float(_smtl_half_unpack2(000077u).x))",
        "_smtl_half_round(float(_smtl_half_unpack2(0x3c00u).x))",
        "_smtl_half_round(float(_smtl_half_unpack2(15360u).y))",
        "_smtl_half_round(float(_smtl_half_unpack2(15360u).x)+value)"}){
        std::string call=unchanged;assert(SweetAIR::foldHalfCalls(call,error) && call==unchanged);}
    std::string unclosed="_smtl_half_round(_smtl_half_round(value)";
    assert(!SweetAIR::foldHalfCalls(unclosed,error));
    out.close();std::cout<<"actual half ESSL adapter: 65536 literal bit patterns, nested idempotence, exact constants, intervening-operation/shape rejection PASS\n";return 0;
}
