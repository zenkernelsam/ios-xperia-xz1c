/* Independent type7 decoder checks the real emitted fields and bounds. */
#include "../../gpu/sweetgl/MetalBlitPlan.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static unsigned parity(uint32_t value){unsigned bits=0;while(value){bits+=value&1;value>>=1;}return (bits&1)^1;}
static void packet(uint32_t word,unsigned opcode,unsigned count)
{
    assert((word>>28)==7 && ((word>>16)&0x7f)==opcode && (word&0x3fff)==count);
    assert(((word>>23)&1)==parity(opcode) && ((word>>15)&1)==parity(count));
}
int main(void)
{
    const unsigned sizes[]={4,64,16380,16384};
    for(unsigned i=0;i<4;++i)for(unsigned copy=0;copy<2;++copy){
        uint64_t source=copy?0x40010000:0,destination=0x40020000;size_t count=smb_words(source,sizes[i]);
        uint32_t *buffer=malloc((count+2)*4);assert(buffer);buffer[0]=0xc0ffee;buffer[count+1]=0xc0ffee;
        size_t written=99;assert(!smb_emit(buffer+1,count-1,source,destination,sizes[i],0x5a,&written) && written==99);
        assert(smb_emit(buffer+1,count,source,destination,sizes[i],0x5a,&written) && written==count);
        packet(buffer[1],0x26,0);packet(buffer[count],0x12,0);
        if(copy){for(unsigned j=0;j<sizes[i]/4;++j){size_t at=2+j*6;packet(buffer[at],0x73,5);
            assert(buffer[at+1]==(1u<<30) && buffer[at+2]==destination+j*4 && buffer[at+3]==0 &&
                buffer[at+4]==source+j*4 && buffer[at+5]==0);}}
        else{packet(buffer[2],0x3d,2+sizes[i]/4);assert(buffer[3]==destination && buffer[4]==0);
            for(unsigned j=0;j<sizes[i]/4;++j)assert(buffer[5+j]==0x5a5a5a5a);}
        assert(buffer[0]==0xc0ffee && buffer[count+1]==0xc0ffee);free(buffer);
    }
    uint32_t buffer[32];size_t count=55;
    assert(!smb_emit(buffer,32,0,0x80000000,4,0,&count));assert(!smb_emit(buffer,32,0,0x7ffffffc,8,0,&count));
    assert(!smb_emit(buffer,32,0,0x40000001,4,0,&count));assert(!smb_emit(buffer,32,0,0x40000000,3,0,&count));
    assert(!smb_emit(buffer,32,0,0x40000000,4,256,&count));assert(count==55);
    assert(!smb_range(64,UINT64_MAX,4) && !smb_range(64,4,UINT64_MAX) && smb_range(64,60,4));
    puts("A5xx packet parity, count, copy/fill addresses, capacity and bounds PASS");return 0;
}
