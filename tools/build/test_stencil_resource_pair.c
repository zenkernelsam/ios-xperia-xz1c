/* Exact production split-plane pairing, lifetime and rollback; no GPU draw. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
enum {PIPE_TEXTURE_2D=2,PIPE_FORMAT_Z32_FLOAT=1,PIPE_FORMAT_S8_UINT=2,PIPE_FORMAT_Z32_FLOAT_S8X24_UINT=3};
struct pipe_resource{void *screen;unsigned target,format,width0,height0,depth0,array_size,last_level,nr_samples,nr_storage_samples,refs;};
struct fd_resource{struct pipe_resource base;unsigned internal_format;struct fd_resource *stencil;unsigned pitch,offset,bytes;uint8_t *memory;};
static struct fd_resource *fd_resource(struct pipe_resource *resource){return (struct fd_resource *)resource;}
#define SWEET_STENCIL_PAIR_HOST_TEST 1
#include "../../gpu/sweetgl/StencilResourcePair.h"
static unsigned retired,allocated;
static struct pipe_resource *plane(void *screen,bool stencil)
{
    struct fd_resource *resource=calloc(1,sizeof(*resource));assert(resource);
    resource->base=(struct pipe_resource){screen,PIPE_TEXTURE_2D,stencil?PIPE_FORMAT_S8_UINT:PIPE_FORMAT_Z32_FLOAT,17,9,1,1,0,0,0,1};
    resource->internal_format=resource->base.format;resource->pitch=stencil?64:128;resource->offset=64;resource->bytes=resource->pitch*9+128;
    resource->memory=malloc(resource->bytes);assert(resource->memory);memset(resource->memory,stencil?0xc7:0xb6,resource->bytes);++allocated;return &resource->base;
}
static void release(struct pipe_resource **pointer)
{
    if(!pointer || !*pointer)return;struct pipe_resource *resource=*pointer;*pointer=NULL;assert(resource->refs);if(--resource->refs)return;
    struct fd_resource *actual=fd_resource(resource);struct pipe_resource *stencil=actual->stencil?&actual->stencil->base:NULL;
    // Matches u_transfer_helper_resource_destroy from the pinned Mesa source.
    release(&stencil);free(actual->memory);free(actual);++retired;
}
static void check_original_plane(struct pipe_resource *resource,bool stencil)
{
    struct fd_resource *r=fd_resource(resource);assert(r->pitch==(stencil?64:128) && r->offset==64);
    for(unsigned i=0;i<r->bytes;++i)assert(r->memory[i]==(stencil?0xc7:0xb6));
}
int main(void)
{
    void *screen=(void *)(uintptr_t)0x540;
    for(unsigned fault=0;fault<15;++fault){struct pipe_resource *primary=plane(screen,false),*secondary=plane(screen,true);
        switch(fault){case 0:secondary->screen=NULL;break;case 1:primary->target=1;break;case 2:secondary->target=1;break;
        case 3:primary->format=PIPE_FORMAT_S8_UINT;break;case 4:fd_resource(primary)->internal_format=PIPE_FORMAT_S8_UINT;break;
        case 5:secondary->format=PIPE_FORMAT_Z32_FLOAT;break;case 6:secondary->width0=18;break;case 7:secondary->height0=10;break;
        case 8:primary->depth0=2;break;case 9:secondary->array_size=2;break;case 10:primary->last_level=1;break;
        case 11:secondary->nr_samples=2;break;case 12:primary->nr_storage_samples=2;break;
        case 13:fd_resource(primary)->stencil=(struct fd_resource *)(uintptr_t)1;break;default:fd_resource(secondary)->stencil=(struct fd_resource *)(uintptr_t)1;break;}
        struct fd_resource a=*fd_resource(primary),b=*fd_resource(secondary);struct pipe_resource *saved=secondary;
        assert(!sweet_stencil_pair_take(primary,&secondary) && saved==secondary && !memcmp(fd_resource(primary),&a,sizeof(a)) && !memcmp(fd_resource(secondary),&b,sizeof(b)));
        fd_resource(primary)->stencil=fd_resource(secondary)->stencil=NULL;check_original_plane(primary,false);check_original_plane(secondary,true);release(&secondary);release(&primary);
    }
    struct pipe_resource *primary=plane(screen,false),*secondary=plane(screen,true),*owned=secondary;
    assert(sweet_stencil_pair_take(primary,&secondary) && !secondary && primary->format==PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
    assert(fd_resource(primary)->internal_format==PIPE_FORMAT_Z32_FLOAT && fd_resource(primary)->stencil==fd_resource(owned));
    check_original_plane(primary,false);check_original_plane(owned,true);
    struct pipe_resource *consumer=primary;++consumer->refs;unsigned before=retired;release(&primary);assert(retired==before);
    check_original_plane(consumer,false);check_original_plane(&fd_resource(consumer)->stencil->base,true);release(&consumer);assert(retired==before+2);
    assert(!sweet_stencil_pair_take(NULL,NULL) && retired==allocated);
    puts("Stencil resource pair PASS: exact original layouts/pages, single-reference ownership, shared consumer lifetime, fifteen fault rollback cases; host model only");return 0;
}
