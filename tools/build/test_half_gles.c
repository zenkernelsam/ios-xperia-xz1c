/* Production shader binary16 pack/unpack/round under actual host Mesa. */
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
static char *source(const char *path)
{FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);assert(n>0 && n<1048576);rewind(f);char *data=calloc(n+1,1);assert(data && fread(data,1,n,f)==n);fclose(f);return data;}
static uint32_t float_bits(float v){uint32_t r;memcpy(&r,&v,4);return r;}
static float bits_float(uint32_t v){float r;memcpy(&r,&v,4);return r;}
static float reference_unpack(uint16_t bits)
{
    unsigned exp=(bits>>10)&31,frac=bits&1023;float v=exp==31?(frac?NAN:INFINITY):exp?ldexpf(1.0f+frac/1024.0f,(int)exp-15):ldexpf((float)frac,-24);
    return bits&32768?-v:v;
}
static uint16_t reference_pack(float value)
{
    uint32_t b=float_bits(value);unsigned sign=(b>>16)&32768,exp=(b>>23)&255,m=b&8388607;
    if(exp==255)return sign|31744|(m?((m>>13)?(m>>13):1):0);
    if(value==0)return sign;double magnitude=fabs((double)value);int exponent;
    if(magnitude<0x1p-14){double exact=magnitude*16777216.0,whole=floor(exact),part=exact-whole;return sign|(unsigned)(whole+(part>.5 || (part==.5 && ((unsigned)whole&1))));}
    frexp(magnitude,&exponent);--exponent;if(exponent>15)return sign|31744;
    double exact=(ldexp(magnitude,-exponent)-1)*1024,whole=floor(exact),part=exact-whole;unsigned frac=(unsigned)whole+(part>.5 || (part==.5 && ((unsigned)whole&1)));
    return sign|((exponent+15)*1024+frac);
}
int main(int argc,char **argv)
{
    assert(argc==2);EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,NULL,NULL) && eglBindAPI(EGL_OPENGL_ES_API));EGLConfig cfg;EGLint count;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(display,attrs,&cfg,1,&count) && count);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,version);assert(ctx!=EGL_NO_CONTEXT && eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,ctx));
    char *helpers=source(argv[1]);const char *head="#version 310 es\nprecision highp float;precision highp int;\nlayout(local_size_x=64) in;\n";
    const char *body="layout(std430,binding=0) readonly buffer Input {uint data[];} input_;layout(std430,binding=1) writeonly buffer Output {uvec4 data[];} output_;"
        "uniform uint mode;void main(){uint i=gl_GlobalInvocationID.x;uint value=input_.data[i];float unpacked=_smtl_half_unpack(value);float v=uintBitsToFloat(value);"
        "output_.data[i]=mode==0u?uvec4(floatBitsToUint(unpacked),_smtl_half_pack(unpacked),0u,0u):uvec4(_smtl_half_pack(v),floatBitsToUint(_smtl_half_round(v)),0u,0u);}";
    const char *parts[]={head,helpers,body};GLuint shader=glCreateShader(GL_COMPUTE_SHADER);glShaderSource(shader,3,parts,NULL);glCompileShader(shader);GLint ok;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){char error[8192];glGetShaderInfoLog(shader,sizeof(error),NULL,error);fprintf(stderr,"half compile error %s\n",error);}assert(ok);free(helpers);
    GLuint program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(program);
    uint32_t *input=malloc(65536*4);GLuint buffers[2];glGenBuffers(2,buffers);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[0]);glBufferData(GL_SHADER_STORAGE_BUFFER,65536*4,NULL,GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[1]);glBufferData(GL_SHADER_STORAGE_BUFFER,65536*16,NULL,GL_DYNAMIC_COPY);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers[1]);
    unsigned checked=0;
    for(unsigned mode=0;mode<2;++mode){unsigned n=0;
        if(!mode){for(;n<65536;++n)input[n]=n;}
        else{uint32_t random=123456789;
            for(unsigned h=0;h<31743;h+=3){float a=reference_unpack(h),b=reference_unpack(h+1);float middle=(float)(((double)a+b)*.5);
                float values[]={middle,nextafterf(middle,0),nextafterf(middle,INFINITY),-middle,-nextafterf(middle,0),-nextafterf(middle,INFINITY)};
                for(unsigned v=0;v<6;++v)input[n++]=float_bits(values[v]);}
            for(;n<65536;++n){random^=random<<13;random^=random>>17;random^=random<<5;input[n]=random;}
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[0]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,n*4,input);glUniform1ui(glGetUniformLocation(program,"mode"),mode);
        glDispatchCompute((n+63)/64,1,1);glMemoryBarrier(GL_ALL_BARRIER_BITS);glFinish();assert(glGetError()==GL_NO_ERROR);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[1]);const uint32_t *output=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,n*16,GL_MAP_READ_BIT);assert(output);unsigned mismatch=0;
        for(unsigned i=0;i<n;++i){float value=mode?bits_float(input[i]):reference_unpack(input[i]);uint16_t expected=mode?reference_pack(value):(uint16_t)input[i];
            bool nan=isnan(value);uint32_t actual=output[i*4+(mode?0:1)],bits=output[i*4+(mode?1:0)];
            if(nan){if((actual&31744)!=31744 || !(actual&1023) || (bits&2139095040u)!=2139095040u || !(bits&8388607u))++mismatch;}
            else if(actual!=expected || bits!=float_bits(reference_unpack(expected))){if(mismatch<4)fprintf(stderr,"half mismatch mode=%u i=%u value=%08x actual=%x expected=%x float=%08x wanted=%08x\n",mode,i,input[i],actual,expected,bits,float_bits(reference_unpack(expected)));++mismatch;}
        }assert(!mismatch);checked+=n;glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);fprintf(stderr,"actual binary16 mode=%u values=%u mismatches=%u\n",mode,n,mismatch);
    }
    free(input);printf("production half shader: %u values all16-bit patterns + float32 half-ties/subnormal/overflow/random, finite exact/NaN class PASS (host Mesa)\n",checked);return 0;
}
