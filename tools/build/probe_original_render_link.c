/* SPDX-License-Identifier: BSD-2-Clause
 * Offline compiler timing for local original inputs. No draw or device IO.
 * The binary config is the exact 316-byte production pipeline structure.
 */
#define _POSIX_C_SOURCE 200809L
#ifdef SWEET_LINK_OSMESA
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <GL/glext.h>
#else
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#endif
#include "../../gpu/sweetgl/XRBlendShader.h"
#include <inttypes.h>
#include <time.h>

static void *read_input(const char *path, size_t *size)
{
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long n=ftell(f); if(n<=0 || n>=1048576){fclose(f);return NULL;}
    rewind(f); unsigned char *bytes=calloc((size_t)n+1,1);
    if(!bytes || fread(bytes,1,(size_t)n,f)!=(size_t)n){free(bytes);fclose(f);return NULL;}
    fclose(f); *size=(size_t)n; return bytes;
}
static uint64_t clock_ns(clockid_t id)
{
    struct timespec t; if(clock_gettime(id,&t))return 0;
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
static uint64_t fnv(const char *s)
{
    uint64_t h=UINT64_C(14695981039346656037);
    for(const unsigned char *p=(const unsigned char *)s;*p;++p)h=(h^*p)*UINT64_C(1099511628211);
    return h;
}
static int report(GLuint object,int program,const char *stage,uint64_t wall,uint64_t cpu)
{
    GLint ok=0; char error[8192]={0};
    if(program){glGetProgramiv(object,GL_LINK_STATUS,&ok);if(!ok)glGetProgramInfoLog(object,sizeof(error),NULL,error);}
    else{glGetShaderiv(object,GL_COMPILE_STATUS,&ok);if(!ok)glGetShaderInfoLog(object,sizeof(error),NULL,error);}
    printf("LINK_PROBE end stage=%s wall_ns=%" PRIu64 " cpu_ns=%" PRIu64 " status=%d\n",stage,clock_ns(CLOCK_MONOTONIC)-wall,clock_ns(CLOCK_THREAD_CPUTIME_ID)-cpu,ok);
    if(!ok)fprintf(stderr,"%s\n",error);
    return ok?0:1;
}
int main(int argc,char **argv)
{
    if(argc!=3 && argc!=4)return 2; setvbuf(stdout,NULL,_IONBF,0);
    size_t vn=0,fn=0,cn=0; char *vertex=read_input(argv[1],&vn),*fragment=read_input(argv[2],&fn);
    SweetRenderPipelineConfig *config=argc==4?read_input(argv[3],&cn):NULL;
    if(!vertex || !fragment || (argc==4 && (!config || cn!=sizeof(*config))) || memchr(vertex,0,vn) || memchr(fragment,0,fn))return 3;
    if(config && (config->reserved || (sweet_render_color_mask(config)&~255u)))return 3;
    for(unsigned i=0;config && i<8;++i)if(sweet_render_color_mask(config)&(1u<<i)){
        SweetRenderColorConfig c=sweet_render_color(config,i);
        if(!c.format || c.blend_enabled>1 || c.source_rgb>14 || c.destination_rgb>14 || c.operation_rgb>4 ||
           c.source_alpha>14 || c.destination_alpha>14 || c.operation_alpha>4 || c.write_mask>15)return 3;
    }
    uint32_t mask=config?sweet_xr_ordered_mask(config):0;
    char *v=mask?sweet_xr_ordered_vertex(vertex):NULL,*f=mask?sweet_xr_ordered_fragment(fragment,config,mask):NULL;
    if(mask && (!v || !f))return 4;
    const char *sources[2]={v?v:vertex,f?f:fragment};
    for(unsigned i=0;i<2;++i)printf("LINK_PROBE source stage=%s bytes=%zu fnv=%016" PRIx64 " ordered_mask=%u\n",i?"fragment":"vertex",strlen(sources[i]),fnv(sources[i]),mask);
#ifdef SWEET_LINK_OSMESA
    const int attributes[]={OSMESA_FORMAT,OSMESA_RGBA,OSMESA_PROFILE,OSMESA_CORE_PROFILE,OSMESA_CONTEXT_MAJOR_VERSION,4,OSMESA_CONTEXT_MINOR_VERSION,3,0};
    OSMesaContext context=OSMesaCreateContextAttribs(attributes,NULL); unsigned char pixels[4]={0};
    if(!context || !OSMesaMakeCurrent(context,pixels,GL_UNSIGNED_BYTE,1,1))return 5;
    printf("LINK_PROBE context api=OpenGL-core ESSL-input=1 ");
#else
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY); EGLint major,minor,count;
    const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
    const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE}; EGLConfig ec;
    if(display==EGL_NO_DISPLAY || !eglInitialize(display,&major,&minor) || !eglBindAPI(EGL_OPENGL_ES_API) || !eglChooseConfig(display,attributes,&ec,1,&count) || count!=1)return 5;
    EGLContext context=eglCreateContext(display,ec,EGL_NO_CONTEXT,ca);
    if(context==EGL_NO_CONTEXT || !eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context))return 5;
    printf("LINK_PROBE context api=OpenGL-ES ");
#endif
    printf("version=%s renderer=%s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
    GLuint shaders[2]={0},program=0; int result=0;
    for(unsigned i=0;i<2;++i){const char *stage=i?"fragment":"vertex";
        shaders[i]=glCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);glShaderSource(shaders[i],1,&sources[i],NULL);
        printf("LINK_PROBE begin stage=%s\n",stage);uint64_t wall=clock_ns(CLOCK_MONOTONIC),cpu=clock_ns(CLOCK_THREAD_CPUTIME_ID);glCompileShader(shaders[i]);
        if((result=report(shaders[i],0,stage,wall,cpu)))goto done;
    }
    program=glCreateProgram(); glProgramParameteri(program,GL_PROGRAM_BINARY_RETRIEVABLE_HINT,GL_TRUE);
    glAttachShader(program,shaders[0]); glAttachShader(program,shaders[1]); printf("LINK_PROBE begin stage=link\n");
    uint64_t wall=clock_ns(CLOCK_MONOTONIC),cpu=clock_ns(CLOCK_THREAD_CPUTIME_ID);glLinkProgram(program);
    result=report(program,1,"link",wall,cpu);
done:
    if(program)glDeleteProgram(program);for(unsigned i=0;i<2;++i)if(shaders[i])glDeleteShader(shaders[i]);
#ifdef SWEET_LINK_OSMESA
    OSMesaDestroyContext(context);
#else
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglTerminate(display);
#endif
    free(v);free(f);free(vertex);free(fragment);free(config);
    return result;
}
