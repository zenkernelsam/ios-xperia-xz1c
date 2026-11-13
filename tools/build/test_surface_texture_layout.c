#include <assert.h>
#include <stdio.h>
#include "SurfaceTextureLayout.h"
int main(void) {
    assert(sweet_surface_texture_needs_copy(0x40000020,1600));
    assert(!sweet_surface_texture_needs_copy(0x40000040,1600));
    assert(!sweet_surface_texture_needs_copy(0x40000080,1600));
    assert(!sweet_surface_texture_needs_copy(0x41048080,3072));
    assert(sweet_surface_texture_needs_copy(0x40000000,276));
    assert(sweet_surface_texture_needs_copy(0x40000004,3072));
    for(unsigned offset=0;offset<16384;++offset)
        assert(sweet_surface_texture_needs_copy(0x40000000+offset,1600)==((offset&63)!=0));
    puts("production texture layout gate: every 16KiB offset and source pitch PASS");
}
