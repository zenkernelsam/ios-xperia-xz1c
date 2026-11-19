/* SPDX-License-Identifier: BSD-2-Clause. Author pixels and independent CPU oracle. */
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
#define SWEET_CA_PHASES 6u
#define SWEET_CA_WIDTH 16u
#define SWEET_CA_HEIGHT 4u
#define SWEET_CA_BYTES (SWEET_CA_WIDTH*SWEET_CA_HEIGHT*4u)
#define SWEET_CA_QUANTIZATION 1u
static const uint8_t sweet_ca_sources[8][4]={
    {0,0,0,0},{255,0,0,255},{32,64,96,128},{16,32,48,64},
    {128,64,32,192},{0,0,0,255},{64,32,16,128},{255,255,255,255}};
static const uint8_t sweet_ca_destinations[8][4]={
    {0,0,0,0},{0,255,0,255},{64,32,96,128},{32,16,48,64},
    {64,128,32,192},{255,128,64,255},{16,32,64,128},{128,64,255,255}};
static inline void sweet_ca_input(uint8_t source[SWEET_CA_BYTES],uint8_t target[SWEET_CA_BYTES])
{
    for(unsigned pixel=0;pixel<SWEET_CA_WIDTH*SWEET_CA_HEIGHT;++pixel){
        memcpy(source+pixel*4,sweet_ca_sources[pixel%8],4);
        memcpy(target+pixel*4,sweet_ca_destinations[pixel%8],4);
    }
}
static inline void sweet_ca_matrix(unsigned phase,float matrix[20])
{
    memset(matrix,0,20*sizeof(float));for(unsigned c=0;c<4;++c)matrix[c*4+c]=1;
    if(phase==1 || phase==4){matrix[0]=matrix[10]=0;matrix[2]=matrix[8]=1;}
    if(phase==2 || phase==5){matrix[0]=0.5f;matrix[5]=1.5f;matrix[16]=0.25f;matrix[17]=-0.125f;}
}
static inline uint8_t sweet_ca_reference(unsigned phase,unsigned pixel,unsigned channel)
{
    float m[20],s[4],d[4],input[4],u[4];sweet_ca_matrix(phase,m);
    for(unsigned c=0;c<4;++c){s[c]=sweet_ca_sources[pixel%8][c]/255.0f;d[c]=sweet_ca_destinations[pixel%8][c]/255.0f;}
    for(unsigned c=0;c<4;++c)input[c]=phase<3?s[c]:(c==3?d[3]:d[c]/fmaxf(d[3],0.005f));
    for(unsigned c=0;c<4;++c){float value=m[16+c];for(unsigned column=0;column<4;++column)value+=input[column]*m[column*4+c];u[c]=fminf(fmaxf(value,0),1);}
    if(phase<3){for(unsigned c=0;c<3;++c)u[c]=fminf(u[c],u[3]);}
    else{for(unsigned c=0;c<3;++c)u[c]*=u[3];for(unsigned c=0;c<4;++c)u[c]=d[c]*(1-s[3])+(d[3]>0?u[c]:0)*s[3];}
    return (uint8_t)floorf(fminf(fmaxf(u[channel],0),1)*255.0f+0.5f);
}
