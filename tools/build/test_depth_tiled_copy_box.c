/* Production box and unmodified fixed Mesa dimension check; host only. */
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include "DepthTiledCopyBox.h"
#include "PinnedFD5Dimensions.h"

int main(void)
{
    struct pipe_resource page={.target=PIPE_TEXTURE_2D,.width0=17,.height0=9,.depth0=1,.array_size=1};
    struct pipe_box old={0,0,0,17,9,1};
    assert(old.x==0 && old.y==0 && old.z==9 && old.width==0 && old.height==17 && old.depth==1);
    assert(!ok_dims(&page,&old,0));
    for(unsigned width=1;width<=16384;width*=2){
        for(unsigned height=1;height<=8192;height*=2){
            page.width0=width;page.height0=height;
            struct pipe_box box=sweet_depth_tiled_copy_box(width,height);
            assert(box.x==0 && box.y==0 && box.z==0 && box.width==width && box.height==height && box.depth==1);
            assert(ok_dims(&page,&box,0));
            ++box.width;assert(!ok_dims(&page,&box,0));
            box=sweet_depth_tiled_copy_box(width,height);++box.height;assert(!ok_dims(&page,&box,0));
            box=sweet_depth_tiled_copy_box(width,height);box.z=1;assert(!ok_dims(&page,&box,0));
        }
    }
    page.width0=17;page.height0=9;
    struct pipe_box fixed=sweet_depth_tiled_copy_box(17,9);
    assert(ok_dims(&page,&fixed,0));
    printf("FD5 dimensions PASS old=(%d,%d,%d,%d,%d,%d) fixed=(%d,%d,%d,%d,%d,%d) sizeof=%zu x=%zu width=%zu y=%zu height=%zu z=%zu depth=%zu host_only=1\n",
        old.x,old.y,old.z,old.width,old.height,old.depth,fixed.x,fixed.y,fixed.z,fixed.width,fixed.height,fixed.depth,
        sizeof(fixed),offsetof(struct pipe_box,x),offsetof(struct pipe_box,width),offsetof(struct pipe_box,y),offsetof(struct pipe_box,height),offsetof(struct pipe_box,z),offsetof(struct pipe_box,depth));
    return 0;
}
