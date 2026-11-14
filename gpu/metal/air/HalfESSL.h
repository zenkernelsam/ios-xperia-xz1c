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
static const char sweet_half_shader[]=R"GLSL(
uint _smtl_half_pack(float value){
    uint bits=floatBitsToUint(value),sign=(bits>>16u)&32768u,exponent=(bits>>23u)&255u,mantissa=bits&8388607u;
    if(exponent==255u)return sign|31744u|(mantissa==0u?0u:max(mantissa>>13u,1u));
    if(exponent<102u)return sign;
    if(exponent>142u)return sign|31744u;
    if(exponent<113u){uint shift=126u-exponent,full=mantissa|8388608u,whole=full>>shift,tail=full&((1u<<shift)-1u),middle=1u<<(shift-1u);
        return sign|(whole+uint(tail>middle || (tail==middle && (whole&1u)!=0u)));}
    uint whole=mantissa>>13u,tail=mantissa&8191u;
    return sign|((exponent-112u)*1024u+whole+uint(tail>4096u || (tail==4096u && (whole&1u)!=0u)));
}
float _smtl_half_unpack(uint value){
    uint sign=(value&32768u)<<16u,exponent=(value>>10u)&31u,mantissa=value&1023u;
    if(exponent==31u)return uintBitsToFloat(sign|2139095040u|(mantissa<<13u));
    if(exponent!=0u)return uintBitsToFloat(sign|((exponent+112u)<<23u)|(mantissa<<13u));
    if(mantissa==0u)return uintBitsToFloat(sign);
    uint actual=113u;while((mantissa&1024u)==0u){mantissa<<=1u;--actual;}
    return uintBitsToFloat(sign|(actual<<23u)|((mantissa&1023u)<<13u));
}
float _smtl_half_round(float value){return _smtl_half_unpack(_smtl_half_pack(value));}
vec2 _smtl_half_round(vec2 v){return vec2(_smtl_half_round(v.x),_smtl_half_round(v.y));}
vec3 _smtl_half_round(vec3 v){return vec3(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z));}
vec4 _smtl_half_round(vec4 v){return vec4(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z),_smtl_half_round(v.w));}
uint _smtl_half_pack2(vec2 v){return _smtl_half_pack(v.x)|(_smtl_half_pack(v.y)<<16u);}
vec2 _smtl_half_unpack2(uint v){return vec2(_smtl_half_unpack(v&65535u),_smtl_half_unpack(v>>16u));}
)GLSL";
namespace SweetAIR {
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
    auto at=text.find("uint _q_index(");if(at==std::string::npos){error="Actual AIR half helper insertion point missing";return false;}
    text.insert(at,sweet_half_shader);return true;
}
}
