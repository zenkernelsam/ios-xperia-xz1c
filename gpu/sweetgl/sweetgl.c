/*
 * sweetgl: Mesa (gallium freedreno + GLES) as a library for iOS processes.
 *
 * A minimal gallium frontend after Mesa's Haiku one (frontends/hgl): one
 * pipe_screen over the msm emulation in sweet_drm.c, GLES contexts made
 * current without a drawable (surfaceless: CoreAnimation draws into FBOs
 * only). IOSurfaces will come in as EGL-style images (get_egl_image).
 *
 * SWEETGL_TEST=1 in the environment runs sweetgl_selftest() on a thread at
 * load: an offscreen triangle, read back and checked, result in the kernel
 * log as "sweetgl: ...".
 */
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach_time.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "frontend/api.h"
#include "state_tracker/st_context.h"
#include "util/driconf.h"
#include "util/xmlconfig.h"
#include "freedreno/drm/freedreno_drm_public.h"
#include "renderonly/renderonly.h"
#include <GLES3/gl3.h>

#include "sweet_drm.h"
#include "sweetgl.h"
#include "main/hash.h"

static const driOptionDescription sweetgl_driconf[] = {
#include "pipe-loader/driinfo_gallium.h"
#include "freedreno/driinfo_freedreno.h"
};

/* freedreno calls these only with a renderonly device (KMS scanout); we pass none */
void
renderonly_scanout_destroy(struct renderonly_scanout *scanout, struct renderonly *ro)
{
	(void)scanout;
	(void)ro;
}

struct renderonly_scanout *
renderonly_create_gpu_import_for_resource(struct pipe_resource *rsc, struct renderonly *ro,
    struct winsys_handle *out_handle)
{
	(void)rsc;
	(void)ro;
	(void)out_handle;
	return NULL;
}

struct sweetgl_ctx {
	struct st_context *st;
	int api;
	uint64_t identity;
};

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t next_context_identity=1;
static __thread struct sweetgl_ctx *current_context;
static struct pipe_frontend_screen *fscreen;
static driOptionCache opt_info, opts;

void
sweet_drm_log(const char *fmt, ...)
{
	static int fd = -1;
	if (fd < 0) {
		fd = open("/dev/null", O_WRONLY | O_CLOEXEC);
	}
	char line[512];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(line, sizeof(line) - 1, fmt, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if (n > (int)sizeof(line) - 2) {
		n = sizeof(line) - 2;
	}
	line[n++] = '\n';
	(void)write(fd, line, n);
}

bool
sweetgl_init(void)
{
	pthread_mutex_lock(&init_lock);
	if (fscreen == NULL) {
		static bool waited;
		int fd = waited ? sweet_drm_open() : sweet_drm_wait_ready(90000);
		waited = true;
		if (fd >= 0) {
			/*
			 * Render straight to memory, no GMEM tiles: tile restore/resolve
			 * covers whole 32-row blocks past the end of IOSurfaces that were
			 * not padded for it (GPU fault on the display surface, boot 0154a).
			 */
			setenv("FD_MESA_DEBUG", "sysmem,nofp16", 0);
			/*
			 * nofp16: CA's shaders count on Apple's mediump/lowp being enough for
			 * texture coordinates and corner shapes; fp16 is not (App
			 * Library pod corners/backdrops, the wallpaper "+").
			 */
			setenv("IR3_SHADER_DEBUG", "nofp16", 0);
			/* fd_screen_create fills opts from opt_info (no xmlconfig: defaults) */
			driParseOptionInfo(&opt_info, sweetgl_driconf, ARRAY_SIZE(sweetgl_driconf));
			struct pipe_screen_config cfg = {.options = &opts, .options_info = &opt_info};
			struct pipe_screen *screen = fd_drm_screen_create_renderonly(fd, NULL, &cfg);
			if (screen == NULL) {
				sweet_drm_log("sweetgl: freedreno screen creation failed");
				close(fd);
			} else {
				fscreen = calloc(1, sizeof(*fscreen));
				fscreen->screen = screen;
				sweetgl_image_hooks(fscreen);  /* before any context: st checks them at creation */
				sweet_drm_log("sweetgl: screen %s (%s)", screen->get_name(screen),
				    screen->get_vendor(screen));
			}
		}
	}
	bool ok = fscreen != NULL;
	pthread_mutex_unlock(&init_lock);
	return ok;
}

struct pipe_screen *
sweetgl_screen(void)
{
	return fscreen ? fscreen->screen : NULL;
}

/* Called only before a new private context is published to its caller. */
static bool sweetgl_prepare_shader_names(struct st_context *st,bool fresh)
{
    if(!fresh || !st || !st->ctx || !st->ctx->Shared)return false;
    struct _mesa_HashTable *names=&st->ctx->Shared->ShaderObjects;
    _mesa_HashLockMutex(names);
    bool empty=names->MaxKey==0;
    _mesa_HashUnlockMutex(names);
    /* EnableNameReuse owns this same mutex; never call it while holding the lock. */
    if(empty)_mesa_HashEnableNameReuse(names);
    return empty && names->alloc_via_idalloc;
}

struct sweetgl_ctx *
sweetgl_create(int api, struct sweetgl_ctx *share)
{
	if (!sweetgl_init()) {
		return NULL;
	}
	struct st_context_attribs attribs = {
		.profile = API_OPENGLES2,
		.major = api >= 3 ? 3 : 2,
		.minor = api >= 3 ? 1 : 0,      /* 3.1: glGetTexLevelParameteriv (eagl.c rect textures) */
	};
	enum st_context_error err = ST_CONTEXT_SUCCESS;
	struct st_context *st = st_api_create_context(fscreen, &attribs, &err, share ? share->st : NULL);
	if (st == NULL) {
		sweet_drm_log("sweetgl: GLES %d context failed: %d", attribs.major, err);
		return NULL;
	}
	bool reuse=sweetgl_prepare_shader_names(st,share==NULL);
	struct sweetgl_ctx *c = calloc(1, sizeof(*c));
	if (!c) { st_destroy_context(st); return NULL; }
	pthread_mutex_lock(&init_lock);
	if (next_context_identity) c->identity=next_context_identity++;
	pthread_mutex_unlock(&init_lock);
	c->st = st;
	c->api = attribs.major;
	st->frontend_context = c;
	sweet_drm_log("sweetgl: shader names context=%llu fresh=%u reuse=%u",(unsigned long long)c->identity,share==NULL,reuse);
	return c;
}

bool
sweetgl_make_current(struct sweetgl_ctx *c)
{
	bool ok=st_api_make_current(c ? c->st : NULL, NULL, NULL);
	if (ok) current_context=c;
	return ok;
}

void
sweetgl_destroy(struct sweetgl_ctx *c)
{
	if (c == NULL) {
		return;
	}
	if (st_api_get_current() == c->st) {
		st_api_make_current(NULL, NULL, NULL);
	}
	if (current_context==c) current_context=NULL;
	st_destroy_context(c->st);
	free(c);
}

static double
ms_since(uint64_t t0)
{
	mach_timebase_info_data_t tb;
	mach_timebase_info(&tb);
	return (double)(mach_absolute_time() - t0) * tb.numer / tb.denom / 1e6;
}

static GLuint
shader(GLenum type, const char *src)
{
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	GLint ok = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[256] = "";
		glGetShaderInfoLog(s, sizeof(log), NULL, log);
		sweet_drm_log("sweetgl: shader compile failed: %s", log);
	}
	return s;
}

/* Full current 2x canvas: GPU rasterization and texture sampling, with
 * CPU readback checks over every non-edge pixel and every texture pixel.
 * This offscreen test does not substitute for panel/UI/FPS acceptance. */
static void
selftest_pattern(uint8_t *out, unsigned x, unsigned y)
{
    out[0] = (uint8_t)(x * 17u + y * 3u);
    out[1] = (uint8_t)(x * 5u + y * 11u);
    out[2] = (uint8_t)(x ^ y);
    out[3] = 255;
}
static uint64_t
selftest_hash(const uint8_t *bytes, size_t count)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < count; ++i) { hash ^= bytes[i]; hash *= UINT64_C(1099511628211); }
    return hash;
}
bool
sweetgl_selftest(void)
{
    enum { W = 750, H = 1624 };
    const size_t byte_count = (size_t)W * H * 4;
    uint64_t t0 = mach_absolute_time();
    struct sweetgl_ctx *c = sweetgl_create(3, NULL);
    if (!c || !sweetgl_make_current(c)) {
        sweet_drm_log("sweetgl: full selftest no context");
        if (c) sweetgl_destroy(c);
        return false;
    }
    double t_ctx = ms_since(t0);
    sweet_drm_log("sweetgl: GL_VERSION %s / GL_RENDERER %s / GLSL %s", glGetString(GL_VERSION),
        glGetString(GL_RENDERER), glGetString(GL_SHADING_LANGUAGE_VERSION));
    uint8_t *px = malloc(byte_count);
    if (!px) { sweetgl_make_current(NULL); sweetgl_destroy(c); return false; }
    memset(px, 0xa5, byte_count);
    GLuint textures[2] = {0}, fbo = 0, prog = 0, sample_prog = 0, vbo = 0;
    glGenTextures(2, textures);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W, H);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    GLenum fbs = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    prog = glCreateProgram();
    GLuint vs = shader(GL_VERTEX_SHADER,
        "#version 300 es\nlayout(location=0) in vec2 p;\nvoid main(){gl_Position=vec4(p,0.0,1.0);}\n");
    GLuint fs = shader(GL_FRAGMENT_SHADER,
        "#version 300 es\nprecision highp float;\nout vec4 c;\nvoid main(){c=vec4(1.0,0.0,0.0,1.0);}\n");
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint linked = 0; glGetProgramiv(prog, GL_LINK_STATUS, &linked); glUseProgram(prog);
    static const float tri[] = {-1,-1,1,-1,-1,1};
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glViewport(0, 0, W, H);
    glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT, 4); glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    uint64_t t1 = mach_absolute_time();
    glClearColor(0, 0, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px);
    GLenum err = glGetError();
    unsigned checked = 0, mismatches = 0;
    for (unsigned y = 0; y < H; ++y) for (unsigned x = 0; x < W; ++x) {
        int64_t side = (int64_t)(2u*x+1u)*H + (int64_t)(2u*y+1u)*W - (int64_t)2*W*H;
        if (side >= -(W+H) && side <= W+H) continue; /* uncertain raster edge only */
        const uint8_t *p = px + ((size_t)y*W+x)*4;
        unsigned r = side < 0 ? 255 : 0, b = side < 0 ? 0 : 255;
        if (abs((int)p[0]-(int)r)>1 || p[1]>1 || abs((int)p[2]-(int)b)>1 || p[3]!=255) ++mismatches;
        ++checked;
    }
    bool triangle_ok = fbs == GL_FRAMEBUFFER_COMPLETE && linked && err == GL_NO_ERROR &&
        checked > 1200000 && !mismatches;
    sweet_drm_log("sweetgl: full triangle width=%u height=%u bytes=%zu checked=%u mismatches=%u fbo=%x linked=%d err=%x hash=%016llx ctx_ms=%.1f draw_read_ms=%.2f",
        W, H, byte_count, checked, mismatches, fbs, linked, err,
        (unsigned long long)selftest_hash(px, byte_count), t_ctx, ms_since(t1));
    for (unsigned y=0; y<H; ++y) for (unsigned x=0; x<W; ++x)
        selftest_pattern(px+((size_t)y*W+x)*4, x, y);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    sample_prog = glCreateProgram();
    vs = shader(GL_VERTEX_SHADER,
        "#version 300 es\nout highp vec2 uv;\nvoid main(){vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);uv=(p+1.0)*0.5;gl_Position=vec4(p,0.0,1.0);}\n");
    fs = shader(GL_FRAGMENT_SHADER,
        "#version 300 es\nprecision highp float;\nin highp vec2 uv;\nuniform sampler2D texImage;\nout vec4 c;\nvoid main(){c=texture(texImage,uv);}\n");
    glAttachShader(sample_prog, vs); glAttachShader(sample_prog, fs); glLinkProgram(sample_prog);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint sample_linked = 0; glGetProgramiv(sample_prog, GL_LINK_STATUS, &sample_linked);
    glUseProgram(sample_prog); glUniform1i(glGetUniformLocation(sample_prog,"texImage"),0);
    memset(px, 0xa5, byte_count);
    t1 = mach_absolute_time();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px);
    GLenum texture_err = glGetError(); unsigned texture_mismatches = 0;
    for (unsigned y=0; y<H; ++y) for (unsigned x=0; x<W; ++x) {
        uint8_t expected[4]; selftest_pattern(expected,x,y); const uint8_t *p=px+((size_t)y*W+x)*4;
        if (abs((int)p[0]-expected[0])>1 || abs((int)p[1]-expected[1])>1 ||
            abs((int)p[2]-expected[2])>1 || p[3]!=255) ++texture_mismatches;
    }
    bool texture_ok = sample_linked && texture_err == GL_NO_ERROR && !texture_mismatches;
    sweet_drm_log("sweetgl: full texture width=%u height=%u bytes=%zu checked=%u mismatches=%u linked=%d err=%x hash=%016llx draw_read_ms=%.2f",
        W, H, byte_count, W*H, texture_mismatches, sample_linked, texture_err,
        (unsigned long long)selftest_hash(px,byte_count), ms_since(t1));
    uint64_t warm = mach_absolute_time(); glDrawArrays(GL_TRIANGLES,0,3); glFinish();
    sweet_drm_log("sweetgl: full warm draw_finish_ms=%.2f",ms_since(warm));
    bool ok = triangle_ok && texture_ok;
    sweet_drm_log("sweetgl: full selftest -> %s", ok ? "PASS" : "WRONG");
    glDeleteBuffers(1,&vbo); glDeleteProgram(prog); glDeleteProgram(sample_prog);
    glDeleteFramebuffers(1,&fbo); glDeleteTextures(2,textures);
    free(px); sweetgl_make_current(NULL); sweetgl_destroy(c); return ok;
}

bool sweetgl_surface_selftest(void);
bool sweetgl_caller_selftest(void);
bool sweetgl_copy_refresh_selftest(void);
bool sweetgl_copy_context_selftest(void);
bool sweetgl_bridge_state_selftest(void);
bool sweetgl_program_lifecycle_selftest(void);

static void *
selftest_thread(void *arg)
{
	(void)arg;
	/* sweetgl_init performs one bounded publication wait for this process. */
	if (!sweetgl_init()) {
		sweet_drm_log("sweetgl: selftest: no GPU after publication wait");
		return NULL;
	}
	if (sweetgl_selftest() && sweetgl_surface_selftest() && sweetgl_caller_selftest() && sweetgl_copy_refresh_selftest()) { if (sweetgl_copy_context_selftest() && sweetgl_bridge_state_selftest()) sweetgl_program_lifecycle_selftest(); }
	return NULL;
}

void sweetgl_layerdump_start(void);
void sweetgl_vibrant_fill_install(void);

__attribute__((constructor)) static void
sweetgl_load(void)
{
	if (strcmp(getprogname(), "SpringBoard") == 0) {
		sweetgl_vibrant_fill_install();
		const char *d = getenv("SWEETGL_LAYERDUMP");
		if (d && d[0] == '1') {
			sweetgl_layerdump_start();
		}
		return;
	}
	sweetgl_eagl_install();
	const char *t = getenv("SWEETGL_TEST");
	if (t && t[0] == '1') {
		pthread_t th;
		pthread_create(&th, NULL, selftest_thread, NULL);
		pthread_detach(th);
	}
}

uint64_t sweetgl_current_identity(void)
{
    return current_context && st_api_get_current()==current_context->st ? current_context->identity : 0;
}
