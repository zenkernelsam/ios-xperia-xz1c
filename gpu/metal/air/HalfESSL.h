/*
 * Binary16 conversion adapted from Mesa half_float.c, fixed769e51468.
 * Copyright (C) 1999-2007 Brian Paul
 * Copyright 2015 Philip Taylor
 * Copyright 2018 Advanced Micro Devices, Inc.
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <string>
#include <cctype>
#include <cstdint>
static const char sweet_half_shader[]=R"GLSL(
uint _smtl_half_select(uint original,uint replacement,bool use_replacement){
    uint mask=0u-uint(use_replacement);return original^((original^replacement)&mask);
}
uint _smtl_half_pack(float value){
    uint bits=floatBitsToUint(value),sign=(bits>>16u)&32768u,exponent=(bits>>23u)&255u,mantissa=bits&8388607u;
    uint shift=126u-clamp(exponent,102u,112u),full=mantissa|8388608u;
    uint units=(full+((1u<<(shift-1u))-1u)+((full>>shift)&1u))>>shift;
    uint rounded=(bits&2147483647u)+4095u+((bits>>13u)&1u);
    uint result=(rounded-0x38000000u)>>13u;
    result=_smtl_half_select(result,units,exponent<113u);
    result=_smtl_half_select(result,0u,exponent<102u);
    result=_smtl_half_select(result,31744u,exponent>142u);
    result=_smtl_half_select(result,31744u|max(mantissa>>13u,uint(mantissa!=0u)),exponent==255u);
    return sign|result;
}
float _smtl_half_unpack(uint value){
    uint sign=(value&32768u)<<16u,exponent=(value>>10u)&31u,mantissa=value&1023u;
    uint high=uint(findMSB(mantissa|1u));
    uint subnormal=((103u+high)<<23u)|((mantissa-(1u<<high))<<(23u-high));
    subnormal=_smtl_half_select(subnormal,0u,mantissa==0u);
    uint result=((exponent+112u)<<23u)|(mantissa<<13u);
    result=_smtl_half_select(result,subnormal,exponent==0u);
    result=_smtl_half_select(result,2139095040u|(mantissa<<13u),exponent==31u);
    return uintBitsToFloat(sign|result);
}
float _smtl_half_round(float value){
    uint bits=floatBitsToUint(value),sign=bits&2147483648u,exponent=(bits>>23u)&255u,mantissa=bits&8388607u;
    uint shift=126u-clamp(exponent,102u,112u),full=mantissa|8388608u;
    uint units=(full+((1u<<(shift-1u))-1u)+((full>>shift)&1u))>>shift;
    uint subnormal=floatBitsToUint(float(units)*0.000000059604644775390625);
    uint whole=((bits&2147483647u)+4095u+((bits>>13u)&1u))&2147475456u;
    uint result=_smtl_half_select(whole,2139095040u,whole>=1199570944u);
    result=_smtl_half_select(result,subnormal,exponent<113u);
    result=_smtl_half_select(result,0u,exponent<102u);
    result=_smtl_half_select(result,2139095040u,exponent>142u);
    result=_smtl_half_select(result,2139095040u|(max(mantissa>>13u,uint(mantissa!=0u))<<13u),exponent==255u);
    return uintBitsToFloat(sign|result);
}
vec2 _smtl_half_round(vec2 v){return vec2(_smtl_half_round(v.x),_smtl_half_round(v.y));}
vec3 _smtl_half_round(vec3 v){return vec3(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z));}
vec4 _smtl_half_round(vec4 v){return vec4(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z),_smtl_half_round(v.w));}
uint _smtl_half_pack2(vec2 v){return _smtl_half_pack(v.x)|(_smtl_half_pack(v.y)<<16u);}
vec2 _smtl_half_unpack2(uint v){return vec2(_smtl_half_unpack(v&65535u),_smtl_half_unpack(v>>16u));}
)GLSL";
namespace SweetAIR {
inline uint32_t halfLiteralFloatBits(uint16_t value)
{
    const uint32_t sign=uint32_t(value&32768u)<<16u,exponent=(value>>10u)&31u,mantissa=value&1023u;
    if(exponent==31u)return sign|0x7f800000u|(mantissa<<13u);
    if(exponent)return sign|((exponent+112u)<<23u)|(mantissa<<13u);
    if(!mantissa)return sign;
    unsigned high=0;for(uint32_t v=mantissa;v>>=1u;)++high;
    return sign|((103u+high)<<23u)|((mantissa-(1u<<high))<<(23u-high));
}
inline bool halfClose(const std::string &text,size_t open,size_t &close)
{
    unsigned depth=1;for(close=open+1;close<text.size();++close){
        if(text[close]=='(')++depth;else if(text[close]==')' && !--depth)return true;}
    return false;
}
inline bool foldHalfCalls(std::string &text,std::string &error)
{
    // Round is idempotent over its binary16 result, including every NaN bit
    // pattern. Do not remove a round across an intervening operation.
    const std::string call="_smtl_half_round(",literal="_smtl_half_round(float(_smtl_half_unpack2(",tail="u).x))";
    size_t cursor=0;
    while(true){size_t at=text.find(call,cursor);if(at==std::string::npos)return true;
        if(at && (std::isalnum((unsigned char)text[at-1]) || text[at-1]=='_')){cursor=at+call.size();continue;}
        size_t end;if(!halfClose(text,at+call.size()-1,end)){error="Unclosed actual AIR half round";return false;}
        size_t inner=at+call.size();while(inner<end && std::isspace((unsigned char)text[inner]))++inner;
        if(text.compare(inner,call.size(),call)==0){size_t inside;
            if(!halfClose(text,inner+call.size()-1,inside)){error="Unclosed actual AIR nested half round";return false;}
            size_t rest=inside+1;while(rest<end && std::isspace((unsigned char)text[rest]))++rest;
            if(rest==end){text.erase(end,1);text.erase(at,call.size());cursor=at;continue;}}
        // Only the complete canonical scalar literal emitted by AIRRuntime.
        // Integer construction retains every half NaN payload and signed zero.
        if(text.compare(at,literal.size(),literal)==0){size_t p=at+literal.size(),begin=p;uint32_t word=0;bool bounded=true;
            while(p<end && std::isdigit((unsigned char)text[p])){unsigned digit=unsigned(text[p++]-'0');
                if(word>(65535u-digit)/10u)bounded=false;else if(bounded)word=word*10u+digit;}
            if(p>begin && bounded && (p-begin==1 || text[begin]!='0') && p+tail.size()==end+1 && text.compare(p,tail.size(),tail)==0){
                text.replace(at,end+1-at,"uintBitsToFloat("+std::to_string(halfLiteralFloatBits(uint16_t(word)))+"u)");cursor=at;continue;}}
        bool changed=false;
        for(const char *carrier:{"float","vec2","vec3","vec4"})for(const char *value:{"0","1"}){
            const std::string exact=std::string(carrier)+"("+value+")";
            if(text.compare(at,call.size()+exact.size()+1,call+exact+")")==0 && at+call.size()+exact.size()==end){
                text.replace(at,end+1-at,exact);changed=true;break;}}
        cursor=changed?at:at+call.size();
    }
}
// The LLVM frontend preserves logical half types. Only their GLSL carriers and
// explicit constructors change; each half SSA result is separately rounded.
inline bool halfESSL(std::string &text,std::string &error)
{
    const std::string extension="#extension GL_AMD_gpu_shader_half_float : require\n";
    auto found=text.find(extension);if(found==std::string::npos)return true;text.erase(found,extension.size());
    if(text.find("_qhalfbuffer")!=std::string::npos){error="Half storage was not lowered to original packed words";return false;}
    for(unsigned n=1;n<=4;++n){std::string from=n==1?"float16_t":"f16vec"+std::to_string(n),to=n==1?"float":"vec"+std::to_string(n);size_t cursor=0;
        auto word=[](char c){return std::isalnum((unsigned char)c) || c=='_';};
        while((found=text.find(from,cursor))!=std::string::npos){size_t after=found+from.size();
            if((found && word(text[found-1])) || (after<text.size() && word(text[after]))){cursor=after;continue;}
            while(after<text.size() && std::isspace((unsigned char)text[after]))++after;
            if(after<text.size() && text[after]=='('){unsigned depth=1;size_t end=after+1;
                for(;end<text.size() && depth;++end){if(text[end]=='(')++depth;else if(text[end]==')')--depth;}
                if(depth){error="Unclosed actual AIR half constructor";return false;}
                text.insert(end,")");text.replace(found,from.size(),"_smtl_half_round("+to);cursor=found+to.size()+17;
            }else{text.replace(found,from.size(),to);cursor=found+to.size();}
        }
    }
    auto replace=[](std::string &value,const std::string &from,const std::string &to){size_t at=0;while((at=value.find(from,at))!=std::string::npos){value.replace(at,from.size(),to);at+=to.size();}};
    replace(text,"unpackFloat2x16(","_smtl_half_unpack2(");replace(text,"packFloat2x16(","_smtl_half_pack2(");
    replace(text,"unpackHalf2x16(","_smtl_half_unpack2(");replace(text,"packHalf2x16(","_smtl_half_pack2(");
    if(!foldHalfCalls(text,error))return false;
    auto at=text.find("uint _q_index(");if(at==std::string::npos){error="Actual AIR half helper insertion point missing";return false;}
    text.insert(at,sweet_half_shader);return true;
}
}
