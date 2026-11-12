#ifndef SWEET_SURFACE_COPY_REFRESH_H
#define SWEET_SURFACE_COPY_REFRESH_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "GLESStateUtils.h"

enum scu_status { SCU_UPLOADED=0, SCU_SKIPPED=1, SCU_LAYOUT=-1, SCU_MEMORY=-2,
    SCU_LOCK=-3, SCU_READ=-4, SCU_UNLOCK=-5, SCU_CHANGED=-6, SCU_GL=-7 };
struct scu_shape { size_t width,height,pitch,bpe,allocation; uint32_t format,type; bool inspect; uint64_t context_id; uint32_t texture_id; };
struct scu_ops {
    void *context;
    uint32_t (*seed)(void *);
    int (*lock)(void *);
    int (*unlock)(void *);
    void *(*allocate)(void *,size_t);
    void (*release)(void *,void *,size_t);
    int (*read)(void *,void *,size_t);
    uint32_t (*upload)(void *,const void *,const struct scu_shape *,bool);
};
struct scu_cache { struct sgl_copy_state state; size_t width,height; uint32_t format,type,texture_id; uint64_t context_id; bool defined; unsigned images,subimages; };
static inline bool scu_owner_matches(const struct scu_cache *cache,uint64_t context,uint32_t texture)
{ return cache && context && texture && cache->context_id==context && cache->texture_id==texture; }
struct scu_result { int status,lock,read,unlock; uint32_t seed,error; size_t bytes,row_bytes,nonzero,last_nonzero; uint8_t preview[16]; bool reused; };
static inline struct scu_result scu_update(const struct scu_ops *ops,const struct scu_shape *shape,
    struct scu_cache *cache,uint64_t now,bool force)
{
    struct scu_result out={0}; out.status=SCU_LAYOUT;
    if (!ops||!shape||!cache||!ops->seed||!ops->lock||!ops->unlock||!ops->allocate||
        !ops->release||!ops->read||!ops->upload) return out;
    out.seed=ops->seed(ops->context);
    size_t span=0,row=0;
    if (sgl_copy_gl_element(shape->format,shape->type)!=shape->bpe||
        !sgl_copy_layout(shape->width,shape->height,shape->pitch,shape->bpe,shape->allocation,&row,&span)||span>(16u<<20)) {
        sgl_copy_finished(&cache->state,out.seed,false,now); return out;
    }
    bool same=scu_owner_matches(cache,shape->context_id,shape->texture_id) && cache->width==shape->width &&
        cache->height==shape->height && cache->format==shape->format && cache->type==shape->type;
    if(!same) {
        memset(cache,0,sizeof(*cache));cache->context_id=shape->context_id;cache->texture_id=shape->texture_id;
        cache->width=shape->width;cache->height=shape->height;cache->format=shape->format;cache->type=shape->type;
    }
    if (!force&&same&&!sgl_copy_needs_refresh(&cache->state,out.seed,now)) {out.status=SCU_SKIPPED;return out;}
    out.bytes=span;out.row_bytes=row;
    void *snapshot=ops->allocate(ops->context,span);
    if (!snapshot) {out.status=SCU_MEMORY;sgl_copy_finished(&cache->state,out.seed,false,now);return out;}
    out.lock=ops->lock(ops->context);
    if (out.lock) out.status=SCU_LOCK;
    else {
        out.seed=ops->seed(ops->context);
        out.read=ops->read(ops->context,snapshot,span);
        bool stable=ops->seed(ops->context)==out.seed;
        out.unlock=ops->unlock(ops->context);
        stable=stable&&ops->seed(ops->context)==out.seed;
        if(out.read)out.status=SCU_READ;
        else if(out.unlock)out.status=SCU_UNLOCK;
        else if(!stable)out.status=SCU_CHANGED;
        else {
            if(shape->inspect) {
                const uint8_t *bytes=snapshot;size_t preview=row<sizeof(out.preview)?row:sizeof(out.preview);
                memcpy(out.preview,bytes+(shape->height/2)*shape->pitch,preview);
                for(size_t y=0;y<shape->height;++y)for(size_t x=0;x<row;++x) {
                    uint8_t value=bytes[y*shape->pitch+x];out.nonzero+=value!=0;
                    out.last_nonzero+=x%shape->bpe==shape->bpe-1&&value!=0;
                }
            }
            /* The source is unlocked now; GL reads only this stable private snapshot. */
            out.reused=same&&cache->defined;
            out.error=ops->upload(ops->context,snapshot,shape,out.reused);
            out.status=out.error?SCU_GL:SCU_UPLOADED;
            if(!out.error){cache->defined=true;if(out.reused)++cache->subimages;else ++cache->images;}
        }
    }
    ops->release(ops->context,snapshot,span);
    sgl_copy_finished(&cache->state,out.seed,out.status==SCU_UPLOADED,now);
    return out;
}
#endif
