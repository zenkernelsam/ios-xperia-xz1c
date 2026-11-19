/* Author pixels through the production GLI source rewrite, link and draw. */
#include "eagl.h"
#include "sweetgl.h"
#include "sweet_drm.h"
#include "CAMatrixProbeValues.h"
#include "QuartzCoreMatrixLayout.h"
#include <stdlib.h>
void *sweetgl_bridge_probe_create(void);
void sweetgl_bridge_probe_destroy(void *);
bool sweetgl_ca_matrix_install_state(void);

bool sweetgl_ca_matrix_selftest(void)
{
    void *context=NULL;GLuint textures[2]={0},fbo=0,program=0,vertex=0,fragment=0;
    uint8_t source[SWEET_CA_BYTES],target[SWEET_CA_BYTES],readback[SWEET_CA_BYTES+32];
    unsigned completed=0;bool ok=false;uint64_t identity=0;
    sweet_ca_input(source,target);
    if(!sweetgl_ca_matrix_install_state())goto done;
    context=sweetgl_bridge_probe_create();if(!context)goto done;sgl_bind(context);
    identity=sweetgl_current_identity();if(!identity)goto done;
    glGenTextures(2,textures);glGenFramebuffers(1,&fbo);
    for(unsigned i=0;i<2;++i){glActiveTexture(GL_TEXTURE0+i);sgl_BindTexture(context,GL_TEXTURE_2D,textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SWEET_CA_WIDTH,SWEET_CA_HEIGHT,0,GL_RGBA,GL_UNSIGNED_BYTE,i?target:source);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[1],0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)goto done;
    const char *vs="attribute vec2 position;varying highp vec2 uv;void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}";
    const char *fs[2]={
        "precision highp float;uniform sampler2D image;varying highp vec2 uv;void main(){lowp vec4 s0=texture2D(image,uv);gl_FragColor=sgl_cm4(s0);}",
        "#extension GL_EXT_shader_framebuffer_fetch : require\nprecision highp float;uniform sampler2D image;varying highp vec2 uv;void main(){lowp vec4 s0=texture2D(image,uv);gl_FragColor=sgl_vcm(s0);}"};
    vertex=(GLuint)(uintptr_t)sgl_CreateShaderObjectARB(context,GL_VERTEX_SHADER);
    fragment=(GLuint)(uintptr_t)sgl_CreateShaderObjectARB(context,GL_FRAGMENT_SHADER);
    program=(GLuint)(uintptr_t)sgl_CreateProgramObjectARB(context);if(!vertex || !fragment || !program)goto done;
    glAttachShader(program,vertex);glAttachShader(program,fragment);glBindAttribLocation(program,0,"position");
    const GLfloat positions[]={-1,-1,1,-1,-1,1,1,1};
    glViewport(0,0,SWEET_CA_WIDTH,SWEET_CA_HEIGHT);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,positions);glEnableVertexAttribArray(0);
    glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    for(unsigned phase=0;phase<SWEET_CA_PHASES;++phase){
        const GLcharARB *text=vs;sgl_ShaderSourceARB(context,(GLhandleARB)(uintptr_t)vertex,1,&text,NULL);sgl_CompileShaderARB(context,(GLhandleARB)(uintptr_t)vertex);
        GLint status=0;glGetShaderiv(vertex,GL_COMPILE_STATUS,&status);if(!status)goto done;
        text=fs[phase>=3];sgl_ShaderSourceARB(context,(GLhandleARB)(uintptr_t)fragment,1,&text,NULL);sgl_CompileShaderARB(context,(GLhandleARB)(uintptr_t)fragment);
        glGetShaderiv(fragment,GL_COMPILE_STATUS,&status);if(!status)goto done;
        sgl_LinkProgramARB(context,(GLhandleARB)(uintptr_t)program);glGetProgramiv(program,GL_LINK_STATUS,&status);if(!status)goto done;
        sgl_UseProgramObjectARB(context,(GLhandleARB)(uintptr_t)program);sgl_Uniform1i(context,glGetUniformLocation(program,"image"),0);
        float matrix[20];sweet_ca_matrix(phase,matrix);GLint location=glGetUniformLocation(program,"color_matrix");if(location<0)goto done;
        glUniform4fv(location,5,matrix);glActiveTexture(GL_TEXTURE1);sgl_BindTexture(context,GL_TEXTURE_2D,textures[1]);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SWEET_CA_WIDTH,SWEET_CA_HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,target);
        glActiveTexture(GL_TEXTURE0);sgl_BindTexture(context,GL_TEXTURE_2D,textures[0]);
        memset(readback,0xa5,sizeof(readback));sgl_DrawArrays(context,GL_TRIANGLE_STRIP,0,4);glFinish();
        glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SWEET_CA_WIDTH,SWEET_CA_HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,readback+16);
        unsigned differences=0,guards=0,maximum=0;uint64_t hash=UINT64_C(14695981039346656037);
        for(unsigned i=0;i<SWEET_CA_BYTES;++i){unsigned expected=sweet_ca_reference(phase,i/4,i%4),actual=readback[16+i];
            unsigned difference=actual>expected?actual-expected:expected-actual;if(difference>maximum)maximum=difference;
            if(difference>SWEET_CA_QUANTIZATION)++differences;hash=(hash^actual)*UINT64_C(1099511628211);}
        for(unsigned i=0;i<16;++i)guards+=(readback[i]!=0xa5)+(readback[16+SWEET_CA_BYTES+i]!=0xa5);
        GLenum error=sgl_GetError(context);
        sweet_drm_log("SWEET CA MATRIX phase=%u function=%#x context=%llu bytes=256 tolerance=1 max_diff=%u mismatches=%u guard_fail=%u error=%#x hash=%016llx production_GLI=1",
            phase,phase<3?0x14:0x2d,(unsigned long long)identity,maximum,differences,guards,error,(unsigned long long)hash);
        if(differences || guards || error)goto done;++completed;
    }
    ok=true;
done:
    if(context){sgl_bind(context);sgl_UseProgramObjectARB(context,(GLhandleARB)0);
        if(program)sgl_DeleteObjectARB(context,(GLhandleARB)(uintptr_t)program);
        if(vertex)sgl_DeleteObjectARB(context,(GLhandleARB)(uintptr_t)vertex);
        if(fragment)sgl_DeleteObjectARB(context,(GLhandleARB)(uintptr_t)fragment);
        sgl_DeleteTextures(context,2,textures);glDeleteFramebuffers(1,&fbo);sweetgl_bridge_probe_destroy(context);}
    sweet_drm_log("SWEET CA MATRIX END result=%u phases=%u checked_bytes=%u guard_bytes=%u context_disposed=%u production_GLI=1 matrix_slots=2",
        ok?0:1,completed,completed*SWEET_CA_BYTES,completed*32,context!=NULL);return ok;
}
