/* RAM-only diagnostic, using the current iOS16 API and existing scatter service. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceRef.h>
#include <GLES3/gl3.h>
#include <mach/vm_page_size.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sweetgl.h"
#include "sweet_drm.h"
#include "SurfaceProbeUtils.h"

int sweetgl_probe_import_texture(IOSurfaceRef surface, uint32_t *handle, uint64_t *bytes);
/* The small caller-address allocation is kept for this process's lifetime. */
static void *address_probe_storage;

static void number(CFMutableDictionaryRef d, CFStringRef key, uint64_t value)
{
    int64_t signed_value = (int64_t)value;
    CFNumberRef n = CFNumberCreate(NULL, kCFNumberSInt64Type, &signed_value);
    if (n) { CFDictionarySetValue(d, key, n); CFRelease(n); }
}
static CFMutableDictionaryRef properties(unsigned w, unsigned h, unsigned pitch, uint64_t address)
{
    CFMutableDictionaryRef d = CFDictionaryCreateMutable(NULL, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!d) return NULL;
    number(d, CFSTR("IOSurfaceWidth"), w); number(d, CFSTR("IOSurfaceHeight"), h);
    number(d, CFSTR("IOSurfaceBytesPerElement"), 4);
    number(d, CFSTR("IOSurfaceBytesPerRow"), pitch);
    number(d, CFSTR("IOSurfacePixelFormat"), UINT32_C(0x42475241));
    number(d, CFSTR("IOSurfaceAllocSize"), (uint64_t)pitch * h);
    if (address) number(d, CFSTR("IOSurfaceAddress"), address);
    return d;
}
static void fill_pattern(uint8_t *pixels, unsigned w, unsigned h, size_t pitch)
{
    for (unsigned y = 0; y < h; ++y) for (unsigned x = 0; x < w; ++x) {
        uint8_t rgba[4]; sp_pattern(rgba, x, y);
        uint8_t *p = pixels + (size_t)y * pitch + (size_t)x * 4;
        p[0] = rgba[2]; p[1] = rgba[1]; p[2] = rgba[0]; p[3] = rgba[3];
    }
}
static void observe_address(void)
{
    enum { W = 64, H = 64, PITCH = 256, OFFSET = 128 };
    if (address_probe_storage || posix_memalign(&address_probe_storage, vm_page_size, 32768)) {
        sweet_drm_log("sweetgl: address probe setup failed"); return;
    }
    uint8_t *requested = (uint8_t *)address_probe_storage + OFFSET;
    memset(address_probe_storage, 0xa5, 32768); fill_pattern(requested, W, H, PITCH);
    CFMutableDictionaryRef d = properties(W, H, PITCH, (uintptr_t)requested);
    IOSurfaceRef s = d ? IOSurfaceCreate(d) : NULL;
    if (d) CFRelease(d);
    int locked = s ? IOSurfaceLock(s, kIOSurfaceLockReadOnly, NULL) : -1;
    bool layout = s && IOSurfaceGetWidth(s) == W && IOSurfaceGetHeight(s) == H &&
        IOSurfaceGetBytesPerRow(s) == PITCH && IOSurfaceGetPlaneCount(s) == 0 &&
        sp_layout(W, H, PITCH, IOSurfaceGetBytesPerElement(s), IOSurfaceGetAllocSize(s));
    const uint8_t *base = s ? IOSurfaceGetBaseAddress(s) : NULL;
    uint64_t hash = 0;
    unsigned mismatch = !locked && layout && base ? sp_check_pattern(base, W, H, PITCH, true, &hash) : UINT32_MAX;
    sweet_drm_log("sweetgl: address probe width=%u height=%u pitch=%u requested=%p returned=%p id=%u lock=%x layout=%u checked=%u mismatches=%u hash=%016llx retained_caller=1",
        W, H, PITCH, requested, base, s ? IOSurfaceGetID(s) : 0, locked, layout,
        !locked && layout && base ? W * H : 0, mismatch, (unsigned long long)hash);
    if (s) { if (!locked) IOSurfaceUnlock(s, kIOSurfaceLockReadOnly, NULL); CFRelease(s); }
}
static GLuint compile(GLenum kind, const char *text)
{
    GLuint shader = glCreateShader(kind); glShaderSource(shader, 1, &text, NULL); glCompileShader(shader);
    return shader;
}
bool sweetgl_surface_selftest(void)
{
    enum { W = 750, H = 1624, PITCH = 3072 };
    observe_address();
    struct sweetgl_ctx *context = sweetgl_create(3, NULL);
    if (!context || !sweetgl_make_current(context)) {
        if (context) sweetgl_destroy(context);
        sweet_drm_log("sweetgl: full surface selftest -> WRONG phase=context"); return false;
    }
    CFMutableDictionaryRef d = properties(W, H, PITCH, 0);
    IOSurfaceRef surface = d ? IOSurfaceCreate(d) : NULL;
    if (d) CFRelease(d);
    GLuint tex[2] = {0}, framebuffer = 0, program = 0;
    uint8_t *readback = NULL;
    uint32_t handle = 0; uint64_t bytes = 0;
    bool ok = false; const char *phase = "layout";
    bool surface_locked = false;
    uint32_t lock_options = 0;
    bool layout = surface && IOSurfaceGetWidth(surface) == W && IOSurfaceGetHeight(surface) == H &&
        IOSurfaceGetBytesPerRow(surface) == PITCH && IOSurfaceGetPlaneCount(surface) == 0 &&
        sp_layout(W, H, PITCH, IOSurfaceGetBytesPerElement(surface), IOSurfaceGetAllocSize(surface));
    if (!layout) goto done;
    uint8_t *base = IOSurfaceGetBaseAddress(surface);
    phase = "cpu_seed";
    if (!base || IOSurfaceLock(surface, 0, NULL)) goto done;
    surface_locked = true; fill_pattern(base, W, H, PITCH);
    if (IOSurfaceUnlock(surface, 0, NULL)) goto done;
    surface_locked = false;
    glGenTextures(2, tex); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex[0]);
    phase = "import";
    if (sweetgl_probe_import_texture(surface, &handle, &bytes)) goto done;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, tex[1]); glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W, H);
    glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[1], 0);
    GLenum fbo = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    program = glCreateProgram();
    GLuint vs = compile(GL_VERTEX_SHADER,
        "#version 300 es\nout highp vec2 uv;\nvoid main(){vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);uv=(p+1.0)*0.5;gl_Position=vec4(p,0.0,1.0);}\n");
    GLuint fs = compile(GL_FRAGMENT_SHADER,
        "#version 300 es\nprecision highp float;\nin highp vec2 uv;\nuniform sampler2D sourceImage;\nout vec4 colour;\nvoid main(){colour=texture(sourceImage,uv);}\n");
    glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint linked = 0; glGetProgramiv(program, GL_LINK_STATUS, &linked); glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "sourceImage"), 0);
    glBindTexture(GL_TEXTURE_2D, tex[0]); glViewport(0, 0, W, H);
    glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT, 4); glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    readback = malloc((size_t)W * H * 4); phase = "readback_alloc";
    if (!readback) goto done;
    memset(readback, 0xa5, (size_t)W * H * 4);
    glDrawArrays(GL_TRIANGLES, 0, 3); glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, readback);
    GLenum error = glGetError(); uint64_t hash = 0;
    unsigned mismatch = sp_check_pattern(readback, W, H, (size_t)W * 4, false, &hash);
    sweet_drm_log("sweetgl: full surface sampler id=%u handle=%u width=%u height=%u pitch=%u bytes=%llu checked=%u mismatches=%u fbo=%x linked=%d err=%x hash=%016llx",
        IOSurfaceGetID(surface), handle, W, H, PITCH, (unsigned long long)bytes, W * H,
        mismatch, fbo, linked, error, (unsigned long long)hash);
    phase = "sampler";
    if (fbo != GL_FRAMEBUFFER_COMPLETE || !linked || error || mismatch) goto done;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[0], 0);
    fbo = glCheckFramebufferStatus(GL_FRAMEBUFFER); glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, W / 2, H / 2); glClearColor(1, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    glScissor(W / 2, 0, W - W / 2, H / 2); glClearColor(0, 1, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    glScissor(0, H / 2, W / 2, H - H / 2); glClearColor(0, 0, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
    glScissor(W / 2, H / 2, W - W / 2, H - H / 2); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
    glFinish(); error = glGetError(); phase = "cpu_sync";
    int sync = sweet_drm_surface_sync(handle);
    lock_options = kIOSurfaceLockReadOnly;
    int lock = sync ? -1 : IOSurfaceLock(surface, lock_options, NULL);
    surface_locked = !lock;
    hash = 0; mismatch = !sync && !lock ? sp_check_bands(base, W, H, PITCH, &hash) : UINT32_MAX;
    sweet_drm_log("sweetgl: full surface writer id=%u handle=%u width=%u height=%u pitch=%u checked=%u mismatches=%u fbo=%x err=%x sync=%d lock=%x hash=%016llx",
        IOSurfaceGetID(surface), handle, W, H, PITCH, !sync && !lock ? W * H : 0,
        mismatch, fbo, error, sync, lock, (unsigned long long)hash);
    ok = fbo == GL_FRAMEBUFFER_COMPLETE && !error && !sync && !lock && !mismatch;
    phase = "writer";
done:
    if (surface_locked) IOSurfaceUnlock(surface, lock_options, NULL);
    glDeleteProgram(program); glDeleteFramebuffers(1, &framebuffer); glDeleteTextures(2, tex);
    free(readback); sweetgl_make_current(NULL); sweetgl_destroy(context);
    if (surface) CFRelease(surface);
    sweet_drm_log("sweetgl: full surface selftest -> %s phase=%s", ok ? "PASS" : "WRONG", phase);
    return ok;
}
