/* SPDX-License-Identifier: BSD-2-Clause
 * A5xx Z/S raster access uses tiled storage, even in BYPASS mode. Original
 * Metal pages stay linear; the existing Freedreno 2D blitter moves exact raw
 * components between them and a private FDL5 work page. No CPU conversion.
 * See Mesa fd5_gmem.c emit_mem2gmem_surf and commit 939486d3d30caf43. */
#pragma once
#include "freedreno/freedreno_context.h"
#include "freedreno/a5xx/fd5_blitter.h"
#include "freedreno/a5xx/fd5_format.h"
#include "fdl/freedreno_layout.h"
#include "util/u_threaded_context.h"
#include "pipe/p_context.h"
#include "DepthTiledCopyBox.h"

typedef struct SweetDepthTiledPlane {
    struct pipe_resource *attachment,*linear,*raw;
    SweetMetalBacking work;
    SweetShaderTextureBinding original;
    unsigned samples;
} SweetDepthTiledPlane;

static enum pipe_format sweet_tiled_transfer_format(unsigned pixel,unsigned samples)
{
    if(pixel==1){switch(samples){case 1:return PIPE_FORMAT_R8_UINT;case 2:return PIPE_FORMAT_R8G8_UINT;
        case 4:return PIPE_FORMAT_R8G8B8A8_UINT;default:return PIPE_FORMAT_NONE;}}
    if(pixel==4){switch(samples){case 1:return PIPE_FORMAT_R32_UINT;case 2:return PIPE_FORMAT_R32G32_UINT;
        case 4:return PIPE_FORMAT_R32G32B32A32_UINT;default:return PIPE_FORMAT_NONE;}}
    return PIPE_FORMAT_NONE;
}
static void sweet_depth_tiled_retire(SweetDepthTiledPlane *plane)
{
    uint32_t original=plane->original.handle,work=plane->work.handle;
    pipe_resource_reference(&plane->attachment,NULL);pipe_resource_reference(&plane->linear,NULL);pipe_resource_reference(&plane->raw,NULL);
    if(work){int result=sweetgl_metal_buffer_release(work);
        sweet_drm_log("SWEET DEPTH TILE RETIRE original=%u work=%u result=%d references_retired=1",original,work,result);}
    memset(plane,0,sizeof(*plane));
}
static struct pipe_resource *sweet_tiled_alias(struct st_context *context,uint32_t handle,enum pipe_format format,
    unsigned width,unsigned height,unsigned pitch,unsigned offset,unsigned samples,const struct fdl_layout *layout)
{
    struct pipe_resource description={.target=PIPE_TEXTURE_2D,.format=format,.width0=width,.height0=height,.depth0=1,.array_size=1,
        .nr_samples=samples,.nr_storage_samples=samples,.bind=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW|PIPE_BIND_RENDER_TARGET,.usage=PIPE_USAGE_DEFAULT};
    if(util_format_is_depth_or_stencil(format))description.bind=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW|PIPE_BIND_DEPTH_STENCIL;
    if(format==PIPE_FORMAT_S8_UINT)description.bind=PIPE_BIND_SHARED|PIPE_BIND_SAMPLER_VIEW;
    struct winsys_handle exported={.type=WINSYS_HANDLE_TYPE_KMS,.handle=handle,.stride=pitch,.offset=offset,
        .format=format,.modifier=DRM_FORMAT_MOD_LINEAR};
    struct pipe_resource *resource=context->screen->resource_from_handle(context->screen,&description,&exported,PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if(!resource)return NULL;
    SweetMetalBacking backing={0};struct fd_resource *actual=fd_resource(resource);
    if(sweet_drm_metal_buffer_view(handle,&backing) || sweet_drm_metal_buffer_view_keep(handle) ||
        actual->layout.cpp!=util_format_get_blocksize(format)*samples || actual->layout.pitch0!=pitch || actual->layout.slices[0].offset!=offset){
        pipe_resource_reference(&resource,NULL);return NULL;}
    if(layout){if(layout->size>backing.logical_bytes || layout->cpp!=actual->layout.cpp || !layout->tile_mode || layout->slices[0].offset){
            pipe_resource_reference(&resource,NULL);return NULL;}
        actual->layout=*layout;actual->layout.format=format;actual->layout.nr_samples=samples;}
    return resource;
}
static int sweet_depth_tiled_copy(struct st_context *context,SweetDepthTiledPlane *plane,bool to_original)
{
    if(!context || !plane || !plane->linear || !plane->raw || !plane->original.handle)return -EINVAL;
    struct pipe_context *pipe=threaded_context_unwrap_sync(context->pipe);
    if(!pipe){sweet_drm_log("SWEET DEPTH TILE REJECT original=%u work=%u stage=context result=%d dispatched=0",plane->original.handle,plane->work.handle,-ENOTSUP);return -ENOTSUP;}
    struct fd_context *native=fd_context(pipe);
    if(!native->screen || native->screen->gen!=5){sweet_drm_log("SWEET DEPTH TILE REJECT original=%u work=%u stage=generation gen=%u result=%d dispatched=0",
        plane->original.handle,plane->work.handle,native->screen?native->screen->gen:0,-ENOTSUP);return -ENOTSUP;}
    struct pipe_blit_info copy={0};copy.src.resource=to_original?plane->raw:plane->linear;
    copy.dst.resource=to_original?plane->linear:plane->raw;
    copy.src.format=copy.dst.format=copy.src.resource->format;
    // Mesa's compact pipe_box order is x,width,y,height,z,depth. Never use
    // positional x,y,z,width,height,depth here: it makes width0 and z=height.
    copy.src.box=sweet_depth_tiled_copy_box(plane->original.width,plane->original.height);copy.dst.box=copy.src.box;
    copy.mask=util_format_get_mask(copy.src.format);copy.filter=PIPE_TEX_FILTER_NEAREST;
    sweet_drm_log("SWEET DEPTH TILE BLIT original=%u work=%u direction=%s gen=%u src_fmt=%u dst_fmt=%u src_color=%u dst_color=%u src_samples=%u dst_samples=%u src_mask=%u dst_mask=%u mask=%u filter=%u src_tile=%u dst_tile=%u",
        plane->original.handle,plane->work.handle,to_original?"store":"load",native->screen->gen,copy.src.format,copy.dst.format,
        fd5_pipe2color(copy.src.format),fd5_pipe2color(copy.dst.format),copy.src.resource->nr_samples,copy.dst.resource->nr_samples,
        util_format_get_mask(copy.src.format),util_format_get_mask(copy.dst.format),copy.mask,copy.filter,
        fd_resource(copy.src.resource)->layout.tile_mode,fd_resource(copy.dst.resource)->layout.tile_mode);
    sweet_drm_log("SWEET DEPTH TILE BOX original=%u work=%u src=%d,%d,%d,%d,%d,%d dst=%d,%d,%d,%d,%d,%d src_size=%u,%u,%u dst_size=%u,%u,%u src_pitch=%u dst_pitch=%u src_offset=%llu dst_offset=%llu",
        plane->original.handle,plane->work.handle,copy.src.box.x,copy.src.box.y,copy.src.box.z,copy.src.box.width,copy.src.box.height,copy.src.box.depth,
        copy.dst.box.x,copy.dst.box.y,copy.dst.box.z,copy.dst.box.width,copy.dst.box.height,copy.dst.box.depth,
        copy.src.resource->width0,copy.src.resource->height0,copy.src.resource->depth0,copy.dst.resource->width0,copy.dst.resource->height0,copy.dst.resource->depth0,
        fd_resource_pitch(fd_resource(copy.src.resource),0),fd_resource_pitch(fd_resource(copy.dst.resource),0),
        (unsigned long long)fd_resource_offset(fd_resource(copy.src.resource),0,0),(unsigned long long)fd_resource_offset(fd_resource(copy.dst.resource),0,0));
    // This exact hardware entry reports whether its format/layout was admitted.
    // Do not fall back through resource_copy_region to a possible CPU transfer.
    if(!fd5_blitter_blit(native,&copy)){sweet_drm_log("SWEET DEPTH TILE REJECT original=%u work=%u stage=fd5_blitter result=%d dispatched=0",plane->original.handle,plane->work.handle,-ENOTSUP);return -ENOTSUP;}
    struct pipe_fence_handle *fence=NULL;pipe->flush(pipe,&fence,0);
    bool completed=fence && context->screen->fence_finish(context->screen,pipe,fence,UINT64_C(5000000000));
    if(fence)context->screen->fence_reference(context->screen,&fence,NULL);
    uint32_t sequence=0;int result=completed?sweet_drm_metal_buffer_sequence(to_original?plane->original.handle:plane->work.handle,&sequence):-ETIMEDOUT;
    if(!result)result=sweetgl_metal_buffer_synchronize(to_original?plane->original.handle:plane->work.handle);
    sweet_drm_log("SWEET DEPTH TILE COPY original=%u work=%u direction=%s samples=%u width=%u height=%u result=%d sequence=%u GPU_only=1 fence=%u",
        plane->original.handle,plane->work.handle,to_original?"store":"load",plane->samples,plane->original.width,plane->original.height,result,sequence,completed);
    return result;
}
static int sweet_depth_tiled_create(struct st_context *context,const SweetShaderTextureBinding *original,unsigned samples,
    unsigned load,SweetDepthTiledPlane *plane)
{
    if(!context || !original || !plane || (original->format!=252 && original->format!=253) || load>2)return -EINVAL;
    SweetMetalBacking backing={0};if(!texture_layout_samples(original,samples,&backing))return -EINVAL;
    struct fdl_layout layout={.tile_mode=TILE5_2,.tile_all=true,.mip_levels=1};enum pipe_format format=texture_format(original->format);
    fdl5_layout(&layout,format,samples,original->width,original->height,1,1,1,false);
    if(!layout.size || layout.size>(UINT64_C(256)<<20) || !layout.pitch0 || layout.pitch0%64 || layout.cpp!=pixel_bytes(original->format)*samples)return -EINVAL;
    plane->original=*original;plane->samples=samples;int result=sweetgl_metal_buffer_allocate(layout.size,&plane->work);
    if(result)return result;
    enum pipe_format raw=sweet_tiled_transfer_format(pixel_bytes(original->format),samples);
    if(raw==PIPE_FORMAT_NONE){result=-ENOTSUP;goto failure;}
    plane->attachment=sweet_tiled_alias(context,plane->work.handle,format,original->width,original->height,layout.pitch0,0,samples,&layout);
    plane->raw=sweet_tiled_alias(context,plane->work.handle,raw,original->width,original->height,layout.pitch0,0,1,&layout);
    plane->linear=sweet_tiled_alias(context,original->handle,raw,original->width,original->height,original->pitch,(unsigned)original->offset,1,NULL);
    if(!plane->attachment || !plane->raw || !plane->linear){result=-ENOMEM;goto failure;}
    sweet_drm_log("SWEET DEPTH TILE PAGE original=%u work=%u format=%u samples=%u pitch=%u bytes=%llu tile=%u original_pitch=%u original_offset=%llu",
        original->handle,plane->work.handle,original->format,samples,layout.pitch0,(unsigned long long)layout.size,layout.tile_mode,original->pitch,(unsigned long long)original->offset);
    sweet_drm_log("SWEET DEPTH TILE MAP original=%u work=%u original_GPU=%#llx work_GPU=%#llx load=%u width=%u height=%u cpp=%u raw_cpp=%u alias_samples=1",
        original->handle,plane->work.handle,(unsigned long long)backing.gpu_address,(unsigned long long)plane->work.gpu_address,load,
        original->width,original->height,layout.cpp,fd_resource(plane->raw)->layout.cpp);
    if(load==1){result=sweet_depth_tiled_copy(context,plane,false);if(result)goto failure;}
    return 0;
failure:
    sweet_depth_tiled_retire(plane);return result;
}
