/* Run real C transport body. Decoded PM4 is a host fixture, never GPU proof. */
#define IOConnectCallScalarMethod blit_test_scalar
#define IOConnectCallMethod blit_test_method
#include "../../gpu/sweetgl/sweet_drm.c"
#undef IOConnectCallScalarMethod
#undef IOConnectCallMethod
#include <assert.h>
#include <stdarg.h>
static void *pages[4097];static uint64_t sizes[4097],addresses[4097];
static unsigned allocatedCount,releasedCount,nextHandle=1,submitCalls,cacheCalls;
static uint32_t sequenceCounter;static int publishedFailure,unpublishedFailure;
void sweet_drm_log(const char *format,...){va_list arguments;va_start(arguments,format);vprintf(format,arguments);va_end(arguments);putchar('\n');}
static unsigned char *find_address(uint64_t address,unsigned bytes)
{
    for(unsigned i=1;i<nextHandle;++i)if(pages[i] && address>=addresses[i] && address-addresses[i]<=sizes[i] &&
        bytes<=sizes[i]-(address-addresses[i]))return (unsigned char *)pages[i]+address-addresses[i];
    assert(!"PM4 references an unowned or out-of-range buffer");return NULL;
}
kern_return_t blit_test_scalar(io_connect_t connection,uint32_t selector,const uint64_t *input,uint32_t inputs,uint64_t *output,uint32_t *outputs)
{
    assert(connection==1);
    if(selector==10){assert(inputs==1 && output && *outputs==3);unsigned id=nextHandle++;assert(id<4097);
        sizes[id]=sweet_metal_backing_span(input[0]);assert(sizes[id] && !posix_memalign(&pages[id],16384,sizes[id]));
        addresses[id]=UINT64_C(0x40000000)+id*0x40000;memset(pages[id],0xa5,sizes[id]);
        output[0]=id;output[1]=addresses[id];output[2]=(uintptr_t)pages[id];++allocatedCount;return 0;}
    if(selector==11){assert(inputs==1 && pages[input[0]]);free(pages[input[0]]);pages[input[0]]=NULL;++releasedCount;return 0;}
    if(selector==14){assert(inputs==2 && input[0]<=sequenceCounter);return 0;}
    if(selector==16){assert(inputs==1 && pages[input[0]]);++cacheCalls;return 0;}
    assert(!"unexpected scalar");return kIOReturnUnsupported;
}
kern_return_t blit_test_method(io_connect_t connection,uint32_t selector,const uint64_t *input,uint32_t inputs,
    const void *structure,size_t structureBytes,uint64_t *output,uint32_t *outputs,void *outStructure,size_t *outBytes)
{
    (void)input;(void)outStructure;(void)outBytes;assert(connection==1 && selector==13 && !inputs && *outputs==1);
    ++submitCalls;const uint32_t *payload=structure;assert(structureBytes>=8 && payload[0]==1 && (payload[1]==2 || payload[1]==3));
    assert(structureBytes==8+16+payload[1]*4);const uint32_t *handles=payload+6;
    for(unsigned i=0;i<payload[1];++i)assert(handles[i] && pages[handles[i]]);
    if(unpublishedFailure){*outputs=0;return kIOReturnNotReady;}
    output[0]=++sequenceCounter;
    if(publishedFailure)return kIOReturnTimeout;
    uint64_t commandAddress=(uint64_t)payload[2]|((uint64_t)payload[3]<<32);unsigned count=payload[4];assert(!payload[5]);
    uint32_t *commands=(uint32_t *)find_address(commandAddress,count*4);unsigned cursor=0;
    while(cursor<count){uint32_t header=commands[cursor++];unsigned opcode=(header>>16)&127,words=header&0x3fff;
        assert((header>>28)==7 && cursor+words<=count);
        if(opcode==0x26 || opcode==0x12){assert(!words);continue;}
        if(opcode==0x3d){assert(words>=3);uint64_t destination=(uint64_t)commands[cursor]|((uint64_t)commands[cursor+1]<<32);
            memcpy(find_address(destination,(words-2)*4),commands+cursor+2,(words-2)*4);}
        else{assert(opcode==0x73 && words==5 && commands[cursor]==(1u<<30));
            uint64_t destination=(uint64_t)commands[cursor+1]|((uint64_t)commands[cursor+2]<<32);
            uint64_t source=(uint64_t)commands[cursor+3]|((uint64_t)commands[cursor+4]<<32);
            memcpy(find_address(destination,4),find_address(source,4),4);}
        cursor+=words;
    }
    assert(cursor==count);return 0;
}
int main(void)
{
    conn=1;nbos=SWEET_DRM_HANDLES;bos=calloc(nbos,sizeof(*bos));assert(bos);
    SweetMetalBacking src,dst;assert(!sweetgl_metal_buffer_allocate(32768,&src));assert(!sweetgl_metal_buffer_allocate(32768,&dst));
    unsigned char *source=src.contents,*destination=dst.contents;for(unsigned i=0;i<32768;++i)source[i]=(unsigned char)(i*9+3);
    uint32_t sequence=0;assert(!sweetgl_metal_buffer_blit(0,dst.handle,0,32,64,0x6b,&sequence) && sequence==1);
    for(unsigned i=0;i<32768;++i)assert(destination[i]==(i>=32&&i<96?0x6b:0xa5));
    assert(!sweetgl_metal_buffer_blit(src.handle,dst.handle,16,128,16396,0,&sequence) && sequence==3 && submitCalls==3);
    for(unsigned i=0;i<32768;++i){unsigned char expected=i>=128&&i<16524?source[16+i-128]:i>=32&&i<96?0x6b:0xa5;assert(destination[i]==expected);}
    unsigned before=submitCalls,allocations=allocatedCount;
    assert(sweetgl_metal_buffer_blit(src.handle,dst.handle,1,0,4,0,&sequence)==-EINVAL);
    assert(sweetgl_metal_buffer_blit(src.handle,dst.handle,0,32764,8,0,&sequence)==-EINVAL);
    assert(sweetgl_metal_buffer_blit(src.handle,src.handle,0,4,8,0,&sequence)==-EINVAL);
    assert(submitCalls==before && allocatedCount==allocations);
    publishedFailure=1;assert(sweetgl_metal_buffer_blit(src.handle,dst.handle,0,0,64,0,&sequence)==-ETIMEDOUT && !sequence);
    assert(submitCalls==before+1 && sbo(src.handle)->seq==sequenceCounter && sbo(dst.handle)->seq==sequenceCounter);
    publishedFailure=0;unpublishedFailure=1;assert(sweetgl_metal_buffer_blit(0,dst.handle,0,0,64,0,&sequence)==-ENODEV && !sequence);
    assert(submitCalls==before+2);unpublishedFailure=0;
    assert(cacheCalls==2);assert(!sweetgl_metal_buffer_release(src.handle) && !sweetgl_metal_buffer_release(dst.handle));
    assert(allocatedCount==releasedCount);free(bos);puts("actual blit C bridge: decoded range effects, chunks, fences and no failure replay PASS");return 0;
}
