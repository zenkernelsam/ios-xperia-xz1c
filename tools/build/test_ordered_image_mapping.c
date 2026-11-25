/* SPDX-License-Identifier: MIT
 * The pinned, unchanged Mesa st_bind_fs_images/st_convert_image bodies map
 * actual GL image units into authored Gallium views. No shader or GPU runs. */
#include "../../gpu/sweetgl/OrderedImageBarrier.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef SWEET_MESA_IMAGE_SOURCE
#error "The checker must provide the source-locked Mesa st_atom_image.c"
#endif
#include SWEET_MESA_IMAGE_SOURCE

static struct fd_shaderimg_stateobj received;
static unsigned deliveries;

GLboolean _mesa_is_image_unit_valid(struct gl_context *gl,struct gl_image_unit *unit)
{
    assert(gl && unit);
    return unit->TexObj!=NULL;
}

GLboolean st_finalize_texture(struct gl_context *gl,struct pipe_context *pipe,
    struct gl_texture_object *texture,GLuint face)
{
    assert(gl && pipe && !face);
    return texture && texture->pt;
}

enum pipe_format st_mesa_format_to_pipe_format(const struct st_context *st,mesa_format format)
{
    assert(st && format==MESA_FORMAT_RGBA_FLOAT32);
    return PIPE_FORMAT_R32G32B32A32_FLOAT;
}

static void set_images(struct pipe_context *pipe,enum pipe_shader_type stage,
    unsigned start,unsigned count,unsigned unbind,const struct pipe_image_view *images)
{
    assert(pipe && stage==PIPE_SHADER_FRAGMENT && start==0 && count<=16 && !unbind);
    memset(&received,0,sizeof(received));++deliveries;
    for(unsigned i=0;i<count;++i) {
        received.si[i]=images[i];
        if(images[i].resource)received.enabled_mask|=1u<<i;
    }
}

int main(void)
{
    static struct gl_context gl;
    struct gl_pipeline_object shader={0};struct gl_program fragment={0};
    gl._Shader=&shader;shader.CurrentProgram[MESA_SHADER_FRAGMENT]=&fragment;
    struct pipe_context pipe={.set_shader_images=set_images};
    struct st_context context={.ctx=&gl,.pipe=&pipe};
    struct pipe_resource resources[8]={0};
    struct gl_texture_object textures[8]={0};
    for(unsigned i=0;i<8;++i) {
        resources[i]=(struct pipe_resource){.target=PIPE_TEXTURE_2D,.nr_samples=1,
            .nr_storage_samples=1,.width0=17,.height0=9,.array_size=1};
        textures[i].Target=GL_TEXTURE_2D;textures[i].pt=&resources[i];
        for(unsigned write=0;write<2;++write) {
            const unsigned unit=i+8*write;
            gl.ImageUnits[unit].TexObj=&textures[i];
            gl.ImageUnits[unit].Access=write?GL_WRITE_ONLY:GL_READ_ONLY;
            gl.ImageUnits[unit]._ActualFormat=MESA_FORMAT_RGBA_FLOAT32;
        }
    }
    // Exercise all eight-target masks and both dense declaration orders.
    // The active GLSL image binding may be 15 while the driver slot is 0.
    for(unsigned mask=1;mask<256;++mask)for(unsigned reverse=0;reverse<2;++reverse) {
        unsigned units[16],count=0;
        for(unsigned slot=0;slot<8;++slot)if(mask&(1u<<slot)) {
            units[count++]=slot;units[count++]=slot+8;
        }
        fragment.info.num_images=count;
        for(unsigned dense=0;dense<count;++dense) {
            const unsigned unit=units[reverse?count-1-dense:dense];
            fragment.sh.ImageUnits[dense]=unit;
            fragment.sh.image_access[dense]=ACCESS_COHERENT|
                (unit>=8?ACCESS_NON_READABLE:ACCESS_NON_WRITEABLE);
        }
        context.state.num_images[PIPE_SHADER_FRAGMENT]=count;
        st_bind_fs_images(&context);
        assert(received.enabled_mask==((1u<<count)-1));
        for(unsigned dense=0;dense<count;++dense) {
            const unsigned unit=fragment.sh.ImageUnits[dense];
            assert(received.si[dense].resource==&resources[unit&7]);
            assert(received.si[dense].access==(unit>=8?PIPE_IMAGE_ACCESS_WRITE:PIPE_IMAGE_ACCESS_READ));
            assert(received.si[dense].shader_access==
                ((unit>=8?PIPE_IMAGE_ACCESS_WRITE:PIPE_IMAGE_ACCESS_READ)|PIPE_IMAGE_ACCESS_COHERENT));
        }
        assert(sweet_ordered_image_contract(&gl,&received,mask));
        const uint32_t enabled=received.enabled_mask;
        received.enabled_mask=mask|(mask<<8);
        if(received.enabled_mask!=enabled)assert(!sweet_ordered_image_contract(&gl,&received,mask));
        received.enabled_mask=enabled;
        const unsigned first_unit=fragment.sh.ImageUnits[0],first_access=fragment.sh.image_access[0];
        fragment.sh.ImageUnits[0]=16;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        fragment.sh.ImageUnits[0]=first_unit;
        fragment.sh.image_access[0]=0;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        fragment.sh.image_access[0]=first_access;
        fragment.sh.ImageUnits[1]=first_unit;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        fragment.sh.ImageUnits[1]=units[reverse?count-2:1];
        received.si[0].resource=NULL;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        received.si[0].resource=&resources[first_unit&7];
        resources[first_unit&7].target=PIPE_BUFFER;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        resources[first_unit&7].target=PIPE_TEXTURE_2D;
        fragment.info.num_abos=1;assert(!sweet_ordered_image_contract(&gl,&received,mask));fragment.info.num_abos=0;
        fragment.sh.ShaderStorageBlocksWriteAccess=1;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        fragment.sh.ShaderStorageBlocksWriteAccess=0;
        struct gl_program vertex={0};shader.CurrentProgram[MESA_SHADER_VERTEX]=&vertex;
        vertex.info.num_images=1;assert(!sweet_ordered_image_contract(&gl,&received,mask));vertex.info.num_images=0;
        vertex.info.num_abos=1;assert(!sweet_ordered_image_contract(&gl,&received,mask));vertex.info.num_abos=0;
        vertex.sh.ShaderStorageBlocksWriteAccess=1;assert(!sweet_ordered_image_contract(&gl,&received,mask));
        vertex.sh.ShaderStorageBlocksWriteAccess=0;shader.CurrentProgram[MESA_SHADER_VERTEX]=NULL;
        assert(sweet_ordered_image_contract(&gl,&received,mask));
    }
    assert(deliveries==510);
    assert(!sweet_ordered_image_contract(NULL,&received,1));
    assert(!sweet_ordered_image_contract(&gl,NULL,1));
    assert(!sweet_ordered_image_contract(&gl,&received,0));
    assert(!sweet_ordered_image_contract(&gl,&received,256));
    puts("Ordered image mapping PASS: actual locked Mesa st_bind_fs_images/st_convert_image, GL binding to dense Gallium slots, all 255 masks in both declaration orders, access/resource/SSBO/atomic fallback; host mapping only");
    return 0;
}
