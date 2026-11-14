/* SPDX-License-Identifier: LGPL-3.0-or-later
 * XR code/curve conventions adapted from pinned metal2vulkan format module.
 * The representation is original Metal bytes; all conversions run on GLES.
 */
#pragma once
static const char sweet_xr_shader[] =
"uniform highp usampler2D _smtl_xr_lut;\n"
"uint _smtl_xr_round(float value){uint raw=floatBitsToUint(value);uint exponent=(raw>>23u)&255u;"
"if(exponent<117u)return 0u;uint mantissa=(raw&8388607u)|8388608u;uint hi,lo;umulExtended(mantissa,510u,hi,lo);"
"uint shift=150u-exponent;if(shift>=34u)return 0u;"
"if(shift==33u)return ((hi&1u)!=0u && lo!=0u)?1u:0u;"
"if(shift==32u)return hi+uint(lo>2147483648u || (lo==2147483648u && (hi&1u)!=0u));"
"uint whole=(hi<<(32u-shift))|(lo>>shift),rest=lo&((1u<<shift)-1u),halfway=1u<<(shift-1u);"
"return whole+uint(rest>halfway || (rest==halfway && (whole&1u)!=0u));}\n"
"uint _smtl_xr_encode(float value,bool alpha,bool srgb){if(isnan(value))value=0.0;"
"if(alpha)value=clamp(value,0.0,1.0);bool negative=value<0.0;uint code;"
"if(srgb && !alpha){uint index=uint(roundEven(min(abs(value),2.0)*8192.0));uint low=0u,high=640u;"
"for(uint n=0u;n<10u && low<high;++n){uint mid=(low+high)/2u;uint first=texelFetch(_smtl_xr_lut,ivec2(mid,0),0).g;"
"if(first<=index)low=mid+1u;else high=mid;}code=low==0u?0u:low-1u;}"
"else code=_smtl_xr_round(min(abs(value),1.5));"
"return negative?384u-min(code,384u):min(384u+code,1023u);}\n"
"float _smtl_xr_decode(uint code,bool alpha,bool srgb){int delta=int(code)-384;float value=float(delta)/510.0;"
"if(srgb && !alpha){uint index=uint(abs(delta));value=float(texelFetch(_smtl_xr_lut,ivec2(index,0),0).r)/8192.0;"
"if(delta<0)value=-value;}return alpha?clamp(value,0.0,1.0):value;}\n"
"uvec4 _smtl_xr_pack(vec4 value,uint format){bool srgb=format==553u || format==555u;"
"uvec4 codes=uvec4(_smtl_xr_encode(value.r,false,srgb),_smtl_xr_encode(value.g,false,srgb),"
"_smtl_xr_encode(value.b,false,srgb),_smtl_xr_encode(value.a,true,false));"
"return format>=554u?uvec4(3221225472u|(codes.r<<20u)|(codes.g<<10u)|codes.b,0u,0u,0u):(codes.bgra<<6u);}\n"
"vec4 _smtl_xr_unpack(uvec4 raw,uint format){bool srgb=format==553u || format==555u;"
"uvec4 codes=format>=554u?uvec4((raw.r>>20u)&1023u,(raw.r>>10u)&1023u,raw.r&1023u,894u):(raw.bgra>>6u);"
"return vec4(_smtl_xr_decode(codes.r,false,srgb),_smtl_xr_decode(codes.g,false,srgb),"
"_smtl_xr_decode(codes.b,false,srgb),_smtl_xr_decode(codes.a,true,false));}\n";
