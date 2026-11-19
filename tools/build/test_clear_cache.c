/* Exercise the extracted production clear path on real host GLES. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpu/sweetgl/MetalShaderAPI.h"
#include "../../gpu/sweetgl/XRColourShader.h"
#include "../../gpu/sweetgl/XRColourTables.h"
static unsigned compiled,linked,drawn;
static bool fail_compile;
static bool xr_format(uint32_t f){return f>=552 && f<=555;}
static void sweet_drm_log(const char *format,...){(void)format;}
static void observed_compile(GLuint shader){++compiled;if(!fail_compile)glCompileShader(shader);}
static void observed_link(GLuint program){++linked;glLinkProgram(program);}
static void observed_draw(GLenum primitive,GLint first,GLsizei count){++drawn;glDrawArrays(primitive,first,count);}
#define glCompileShader observed_compile
#define glLinkProgram observed_link
#define glDrawArrays observed_draw
#include "ProductionClear.h"
#undef glCompileShader
#undef glLinkProgram
#undef glDrawArrays

static GLuint target(unsigned format,unsigned width,unsigned height)
{
    GLuint texture;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    bool xr=xr_format(format),wide=format<554;
    GLenum storage=xr?(wide?GL_RGBA16UI:GL_R32UI):format==115?GL_RGBA32F:format==10?GL_R8:GL_RGBA8;
    GLenum channels=xr?(wide?GL_RGBA_INTEGER:GL_RED_INTEGER):format==10?GL_RED:GL_RGBA;
    GLenum type=xr?(wide?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT):format==115?GL_FLOAT:GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D,0,storage,width,height,0,channels,type,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    return texture;
}
static void attach(GLuint texture)
{glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);}
static void read_target(unsigned format,unsigned width,unsigned height,unsigned char *data)
{
    bool xr=xr_format(format),wide=format<554;
    GLenum channels=xr?(wide?GL_RGBA_INTEGER:GL_RED_INTEGER):format==10?GL_RED:GL_RGBA;
    GLenum type=xr?(wide?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT):format==115?GL_FLOAT:GL_UNSIGNED_BYTE;
    glReadPixels(0,0,width,height,channels,type,data);assert(glGetError()==GL_NO_ERROR);
}
int main(void)
{
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));
    EGLConfig config;EGLint count,attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
    assert(eglChooseConfig(display,attributes,&config,1,&count) && count);EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(context!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    GLuint framebuffer;glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glColorMask(1,1,1,1);
    SweetMetalShaderCache cache={0};unsigned checks=0,reuses=0;
    const unsigned formats[]={115,10,80,552,553,554,555};
    for(unsigned f=0;f<sizeof(formats)/sizeof(formats[0]);++f){unsigned format=formats[f];
        for(unsigned phase=0;phase<3;++phase){SweetRenderPass pass={0};pass.target.format=format;pass.target.width=17;pass.target.height=9;
            pass.clear[0]=phase==0?.25:phase==1?.75:0;pass.clear[1]=.5;pass.clear[2]=phase==2?1:.125;pass.clear[3]=1;
            GLuint a=target(format,17,9),b=target(format,17,9);unsigned before=compiled;
            attach(a);assert(!bounded_clear(&pass,&cache));glFinish();unsigned char cached[17*9*16],uncached[17*9*16];
            memset(cached,0xaa,sizeof(cached));read_target(format,17,9,cached);if(compiled==before)++reuses;
            attach(b);assert(!bounded_clear(&pass,NULL));glFinish();memset(uncached,0xaa,sizeof(uncached));read_target(format,17,9,uncached);
            assert(!memcmp(cached,uncached,sizeof(cached)));++checks;glDeleteTextures(1,&a);glDeleteTextures(1,&b);
        }
    }
    assert(checks==21 && reuses==16);
    SweetRenderPass pass={0};pass.target.format=10;pass.target.width=17;pass.target.height=9;pass.clear[0]=.25;pass.clear[3]=1;
    EGLContext second=eglCreateContext(display,config,EGL_NO_CONTEXT,version);assert(second!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,second));
    GLuint second_fbo;glGenFramebuffers(1,&second_fbo);glBindFramebuffer(GL_FRAMEBUFFER,second_fbo);SweetMetalShaderCache other={0};
    GLuint t=target(10,17,9);attach(t);unsigned before=compiled;assert(!bounded_clear(&pass,&other) && compiled==before+2);
    unsigned char second_pixels[17*9];glPixelStorei(GL_PACK_ALIGNMENT,1);read_target(10,17,9,second_pixels);
    for(unsigned i=0;i<sizeof(second_pixels);++i)assert(second_pixels[i]==64);
    sweet_metal_shader_cache_dispose(&other);glDeleteTextures(1,&t);glDeleteFramebuffers(1,&second_fbo);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context);eglDestroyContext(display,second);
    t=target(10,17,9);attach(t);
    fail_compile=true;SweetMetalShaderCache failed={0};assert(bounded_clear(&pass,&failed)<0 && !failed.clear[0]);fail_compile=false;
    assert(!bounded_clear(&pass,&failed) && failed.clear[0]);
    GLuint saved[5];unsigned saved_count=0;for(unsigned i=0;i<5;++i)if(cache.clear[i])saved[saved_count++]=cache.clear[i];
    sweet_metal_shader_cache_dispose(&cache);for(unsigned i=0;i<saved_count;++i)assert(!glIsProgram(saved[i]));sweet_metal_shader_cache_dispose(&failed);glDeleteTextures(1,&t);
    printf("production Clear cache PASS: checks=%u reuses=%u compiled=%u linked=%u draws=%u all_bytes_equal=1 failure_retry=1 contexts_isolated=1 programs_retired=1 host_only=1\n",checks,reuses,compiled,linked,drawn);
    glDeleteFramebuffers(1,&framebuffer);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);return 0;
}
