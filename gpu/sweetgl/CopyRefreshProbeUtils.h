#ifndef COPY_REFRESH_PROBE_UTILS_H
#define COPY_REFRESH_PROBE_UTILS_H
#include <stdint.h>
#include <stddef.h>
static inline void crp_pixel(uint8_t rgba[4],unsigned x,unsigned y,unsigned phase,unsigned bpe)
{
    if(bpe==2) {rgba[0]=rgba[1]=rgba[2]=(uint8_t)(x*17+phase*59+3);rgba[3]=(uint8_t)(x*7+phase*43+31);}
    else {rgba[0]=(uint8_t)(x*3+y*5+phase*37);rgba[1]=(uint8_t)(x*11+y*7+phase*53);
        rgba[2]=(uint8_t)(x*13+y*17+phase*71);rgba[3]=(uint8_t)(x+y+phase*19+127);}
}
static inline void crp_fill(uint8_t *data,unsigned w,unsigned h,size_t pitch,unsigned phase,unsigned bpe)
{
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){uint8_t p[4];crp_pixel(p,x,y,phase,bpe);
        if(bpe==2){data[y*pitch+x*2]=p[0];data[y*pitch+x*2+1]=p[3];}
        else for(unsigned c=0;c<4;++c)data[y*pitch+x*4+c]=p[c];}
}
static inline unsigned crp_check(const uint8_t *data,unsigned w,unsigned h,unsigned phase,unsigned bpe,uint64_t *hash)
{
    uint64_t value=UINT64_C(14695981039346656037);unsigned wrong=0;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){uint8_t expected[4];crp_pixel(expected,x,y,phase,bpe);
        int different=0;for(unsigned c=0;c<4;++c){uint8_t actual=data[((size_t)y*w+x)*4+c];
            different|=actual!=expected[c];value=(value^actual)*UINT64_C(1099511628211);}wrong+=different;}
    *hash=value;return wrong;
}
#endif
