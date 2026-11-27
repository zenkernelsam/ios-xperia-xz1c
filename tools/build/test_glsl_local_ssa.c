/* SPDX-License-Identifier: BSD-2-Clause
 * Authored GLSL semantics: per-call initialization, nested inout, early return,
 * loops, vector locals and SSBO ordering. No original UI pixels/device IO.
 * Short-lived host context is process-owned; LLVM14 destroy failure is tested
 * independently by probe_original_render_link, not hidden by this fixture.
 */
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <GL/glext.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COUNT 256u
static const char source[]=
    "#version 310 es\nprecision highp float;precision highp int;precision highp usampler2D;\n"
    "layout(local_size_x=8) in;layout(std430,binding=0) coherent buffer Values{uint v[];};"
    "layout(binding=0) uniform highp usampler2D marker_tex;uniform uint salt;"
    "uint leaf(inout uint x){uint before=x;uint local=before^0x1357u;x+=9u;local+=x;return local^0x2468u;}"
    "uint branch(uint n){if((n&1u)!=0u)return n*3u+1u;uint x=11u;for(uint j=0u;j<5u;++j){x=x*5u+n+j;if(j==2u&&(n&4u)!=0u)break;}return x;}"
    "uint initialized(uint n){uint total=0u;for(uint j=0u;j<4u;++j){uint local=7u;uint r=leaf(local);total+=local+r+n;}return total;}"
    "uvec3 vector_local(uvec3 x){uvec3 local=x*uvec3(3u,5u,7u);return local^uvec3(1u,2u,3u);}"
    "uint storage_order(uint at,uint x){v[at]=x;return v[at]+1u;}"
    "void main(){uint i=gl_GlobalInvocationID.x;if(i>=256u)return;uint x=i+salt;uint r=leaf(x);"
    "uvec4 marker=texelFetch(marker_tex,ivec2(0),0);uvec3 a=vector_local(uvec3(i,salt,i+salt));"
    "v[i*4u]=x^r^marker.x;v[i*4u+1u]=branch(i+salt)^marker.y;"
    "v[i*4u+2u]=initialized(i+salt)^a.x^a.y^a.z^marker.z;"
    "uint ordered=storage_order(i*4u+3u,r);v[i*4u+3u]=ordered^marker.w;}";
static uint32_t leaf(uint32_t *x)
{uint32_t before=*x,local=before^0x1357u;*x+=9u;local+=*x;return local^0x2468u;}
static uint32_t branch(uint32_t n)
{if(n&1u)return n*3u+1u;uint32_t x=11u;for(uint32_t j=0;j<5;++j){x=x*5u+n+j;if(j==2 && (n&4u))break;}return x;}
static uint32_t initialized(uint32_t n)
{uint32_t total=0;for(unsigned j=0;j<4;++j){uint32_t local=7,r=leaf(&local);total+=local+r+n;}return total;}
static int valid(GLuint n,int program)
{GLint ok=0;char error[4096]={0};if(program){glGetProgramiv(n,GL_LINK_STATUS,&ok);if(!ok)glGetProgramInfoLog(n,sizeof(error),NULL,error);}else{glGetShaderiv(n,GL_COMPILE_STATUS,&ok);if(!ok)glGetShaderInfoLog(n,sizeof(error),NULL,error);}if(!ok)fprintf(stderr,"%s\n",error);return ok;}
int main(void)
{
    const int attrs[]={OSMESA_FORMAT,OSMESA_RGBA,OSMESA_PROFILE,OSMESA_CORE_PROFILE,OSMESA_CONTEXT_MAJOR_VERSION,4,OSMESA_CONTEXT_MINOR_VERSION,3,0};
    OSMesaContext context=OSMesaCreateContextAttribs(attrs,NULL);unsigned char framebuffer[4]={0};
    if(!context || !OSMesaMakeCurrent(context,framebuffer,GL_UNSIGNED_BYTE,1,1))return 2;
    fprintf(stderr,"LOCAL_SSA host=%s renderer=%s process_owned_context=1\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
    GLuint shader=glCreateShader(GL_COMPUTE_SHADER);const char *text=source;glShaderSource(shader,1,&text,NULL);glCompileShader(shader);if(!valid(shader,0))return 3;
    GLuint program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);if(!valid(program,1))return 4;glUseProgram(program);
    GLuint buffer=0,texture=0;glGenBuffers(1,&buffer);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);
    glGenTextures(1,&texture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
    const uint32_t marker[4]={17,31,47,63};glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32UI,1,1);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA_INTEGER,GL_UNSIGNED_INT,marker);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    uint32_t actual[COUNT*4+16];const uint32_t salts[3]={0,17,0xf3abffffu};uint64_t checks=0;
    for(unsigned phase=0;phase<3;++phase){for(unsigned i=0;i<COUNT*4+16;++i)actual[i]=0xa1b2c3d4u;
        glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(actual),actual,GL_DYNAMIC_COPY);glUniform1ui(glGetUniformLocation(program,"salt"),salts[phase]);
        glDispatchCompute(COUNT/8,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,0,sizeof(actual),actual);
        if(glGetError()!=GL_NO_ERROR)return 5;
        for(uint32_t i=0;i<COUNT;++i){uint32_t x=i+salts[phase],r=leaf(&x),expected[4]={x^r^marker[0],branch(i+salts[phase])^marker[1],initialized(i+salts[phase])^((i*3u)^1u)^((salts[phase]*5u)^2u)^(((i+salts[phase])*7u)^3u)^marker[2],(r+1u)^marker[3]};
            for(unsigned lane=0;lane<4;++lane){++checks;if(actual[i*4+lane]!=expected[lane]){fprintf(stderr,"LOCAL_SSA mismatch phase=%u invocation=%u lane=%u actual=%x expected=%x\n",phase,i,lane,actual[i*4+lane],expected[lane]);return 6;}}}
        for(unsigned i=COUNT*4;i<COUNT*4+16;++i){++checks;if(actual[i]!=0xa1b2c3d4u)return 7;}
        if(fwrite(actual,1,sizeof(actual),stdout)!=sizeof(actual))return 8;
    }
    fprintf(stderr,"LOCAL_SSA PASS invocations=768 word_checks=%llu guard_words=48 exact_uint32=1 context_destroy_not_tested=1\n",(unsigned long long)checks);
    glDeleteTextures(1,&texture);glDeleteBuffers(1,&buffer);glDeleteShader(shader);glDeleteProgram(program);
    return 0;
}
