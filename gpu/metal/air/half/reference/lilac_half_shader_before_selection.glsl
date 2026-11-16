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

uint _smtl_half_pack(float value){
    uint bits=floatBitsToUint(value),sign=(bits>>16u)&32768u,exponent=(bits>>23u)&255u,mantissa=bits&8388607u;
    if(exponent==255u)return sign|31744u|(mantissa==0u?0u:max(mantissa>>13u,1u));
    if(exponent<102u)return sign;
    if(exponent>142u)return sign|31744u;
    if(exponent<113u){uint shift=126u-exponent,full=mantissa|8388608u;
        return sign|((full+((1u<<(shift-1u))-1u)+((full>>shift)&1u))>>shift);}
    uint rounded=(bits&2147483647u)+4095u+((bits>>13u)&1u);
    return sign|((rounded-0x38000000u)>>13u);
}
float _smtl_half_unpack(uint value){
    uint sign=(value&32768u)<<16u,exponent=(value>>10u)&31u,mantissa=value&1023u;
    if(exponent==31u)return uintBitsToFloat(sign|2139095040u|(mantissa<<13u));
    if(exponent!=0u)return uintBitsToFloat(sign|((exponent+112u)<<23u)|(mantissa<<13u));
    if(mantissa==0u)return uintBitsToFloat(sign);
    // A binary16 subnormal has only ten significand bits. Construct its
    // float32 exponent/significand directly instead of an inlined loop.
    uint high=uint(findMSB(mantissa));
    return uintBitsToFloat(sign|((103u+high)<<23u)|((mantissa-(1u<<high))<<(23u-high)));
}
float _smtl_half_round(float value){
    // Preserve the exact pack->unpack result without duplicating that call
    // graph at every half SSA value during Mesa link-time function inlining.
    uint bits=floatBitsToUint(value),sign=bits&2147483648u,exponent=(bits>>23u)&255u,mantissa=bits&8388607u;
    if(exponent==255u)return uintBitsToFloat(sign|2139095040u|(mantissa==0u?0u:max(mantissa>>13u,1u)<<13u));
    if(exponent<102u)return uintBitsToFloat(sign);
    if(exponent>142u)return uintBitsToFloat(sign|2139095040u);
    if(exponent<113u){uint shift=126u-exponent,full=mantissa|8388608u;
        uint units=(full+((1u<<(shift-1u))-1u)+((full>>shift)&1u))>>shift;
        // units <= 1024; conversion and multiplication by 2^-24 are exact.
        return uintBitsToFloat(sign|floatBitsToUint(float(units)*0.000000059604644775390625));}
    // Add half an ULP minus one plus the retained low bit: exact ties-to-even
    // without comparison branches at every inlined half SSA value.
    uint whole=((bits&2147483647u)+4095u+((bits>>13u)&1u))&2147475456u;
    return uintBitsToFloat(sign|(whole>=1199570944u?2139095040u:whole));
}
vec2 _smtl_half_round(vec2 v){return vec2(_smtl_half_round(v.x),_smtl_half_round(v.y));}
vec3 _smtl_half_round(vec3 v){return vec3(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z));}
vec4 _smtl_half_round(vec4 v){return vec4(_smtl_half_round(v.x),_smtl_half_round(v.y),_smtl_half_round(v.z),_smtl_half_round(v.w));}
uint _smtl_half_pack2(vec2 v){return _smtl_half_pack(v.x)|(_smtl_half_pack(v.y)<<16u);}
vec2 _smtl_half_unpack2(uint v){return vec2(_smtl_half_unpack(v&65535u),_smtl_half_unpack(v>>16u));}
