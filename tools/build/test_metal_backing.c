/* Execute the actual userland bridge against a bounded host transport. */
#define IOConnectCallScalarMethod metal_test_scalar
#include "../../gpu/sweetgl/sweet_drm.c"
#undef IOConnectCallScalarMethod
#include <assert.h>
static void *host_pages[4097];
static unsigned next_handle=1,allocated,released,invalidated,alloc_fail,release_fail,bad_map,bad_va;
void sweet_drm_log(const char *format,...){(void)format;}
kern_return_t metal_test_scalar(io_connect_t connection,uint32_t selector,const uint64_t *input,uint32_t inputs,uint64_t *output,uint32_t *outputs)
{
    assert(connection==1);
    if(selector==10) {
        assert(inputs==1 && *outputs==3 && output && input);
        if(alloc_fail)return kIOReturnNoMemory;
        uint64_t span=sweet_metal_backing_span(input[0]);assert(span && span<=32768);
        unsigned handle=next_handle++;assert(handle<=4096);
        assert(!posix_memalign(&host_pages[handle],16384,span));++allocated;
        output[0]=handle;output[1]=bad_va?UINT64_C(0x80000000):UINT64_C(0x40000000)+handle*32768;
        output[2]=(uintptr_t)host_pages[handle]+(bad_map?1:0);return 0;
    }
    if(selector==11) {
        assert(inputs==1 && input[0]<=4096 && host_pages[input[0]]);
        if(release_fail)return kIOReturnTimeout;
        free(host_pages[input[0]]);host_pages[input[0]]=NULL;++released;return 0;
    }
    if(selector==14){assert(inputs==2);return 0;}
    if(selector==16){assert(inputs==1 && host_pages[input[0]]);++invalidated;return 0;}
    assert(!"unexpected transport operation");return kIOReturnUnsupported;
}
int main(void)
{
    conn=1;nbos=SWEET_DRM_HANDLES;bos=calloc(nbos,sizeof(*bos));assert(bos);
    SweetMetalBacking backing;unsigned before=allocated;
    const uint64_t rejected[]={0,(UINT64_C(256)<<20)+1,UINT64_MAX};
    for(unsigned i=0;i<3;++i){memset(&backing,0xff,sizeof(backing));assert(sweetgl_metal_buffer_allocate(rejected[i],&backing)==-EINVAL);
        SweetMetalBacking zero={0};assert(!memcmp(&backing,&zero,sizeof(zero)));}
    assert(sweetgl_metal_buffer_allocate(432,NULL)==-EINVAL && allocated==before);
    assert(!sweetgl_metal_buffer_allocate(432,&backing) && sweet_metal_backing_valid(&backing));
    assert(backing.logical_bytes==432 && backing.allocated_bytes==16384);
    memset(backing.contents,0xa5,432);assert(!sweetgl_metal_buffer_synchronize(backing.handle));
    struct sbo *buffer=sbo(backing.handle);buffer->used=1;buffer->seq=7;
    assert(!sweetgl_metal_buffer_synchronize(backing.handle) && invalidated==1);
    release_fail=1;assert(sweetgl_metal_buffer_release(backing.handle)==-ETIMEDOUT);
    assert(sbo(backing.handle)->metal_bytes==432 && host_pages[backing.handle]);
    release_fail=0;assert(!sweetgl_metal_buffer_release(backing.handle));
    assert(sweetgl_metal_buffer_release(backing.handle)==-EINVAL && sweetgl_metal_buffer_synchronize(backing.handle)==-EINVAL);
    alloc_fail=1;assert(sweetgl_metal_buffer_allocate(432,&backing)==-ENOMEM);alloc_fail=0;
    bad_map=1;assert(sweetgl_metal_buffer_allocate(432,&backing)==-EIO);bad_map=0;
    bad_va=1;assert(sweetgl_metal_buffer_allocate(432,&backing)<0);bad_va=0;
    struct drm_msm_gem_new mesa={.size=432};assert(!gem_new(&mesa));
    assert(sweetgl_metal_buffer_release(mesa.handle)==-EINVAL && sweetgl_metal_buffer_synchronize(mesa.handle)==-EINVAL);
    assert(!gem_close(mesa.handle));
    assert(allocated==released);for(unsigned i=0;i<4097;++i)assert(!host_pages[i]);
    free(bos);bos=NULL;puts("actual bridge: sizes, mapping bounds, ownership, retirement and cache path PASS");return 0;
}
