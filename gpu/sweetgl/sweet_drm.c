/*
 * libdrm + msm DRM emulation for Mesa freedreno on iOS (no DRM here).
 *
 * freedreno's msm backend talks to the kernel only through the libdrm calls
 * declared in include/xf86drm.h and one mmap (freedreno_bo.c, hooked by
 * mesa.patch to sweet_drm_bo_map). This file answers them in user space on
 * top of SweetGPUScatter (xnu iokit/Kernel/arm/SweetGPUScatter.cpp):
 *
 *   GEM_NEW / GEM_CLOSE      sel 10 / 11: wired buffer, kernel GPU VA,
 *                            write-combined mapping made by the kernel
 *   GEM_INFO GET_OFFSET      handle << 32; sweet_drm_bo_map() turns it back
 *                            into the kernel's mapping
 *   GEM_INFO GET_IOVA        the kernel GPU VA (softpin, UAPI 1.7)
 *   GEM_SUBMIT               sel 13: CMD_BUF cmds become IBs at bo VA +
 *                            offset; every listed bo remembers the fence
 *   WAIT_FENCE, CPU_PREP     the read-only fence page, then sel 14
 *   GET_PARAM                sel 15 (chip id, GMEM) and constants
 *
 * One device per process; the "fd" is /dev/null (Mesa dup()s and closes
 * it). sweet_drm_import_iosurface() makes a GEM handle for an IOSurface
 * (sel 12) for the EAGL layer.
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mach/mach_time.h>
#include <IOKit/IOKitLib.h>

#include "xf86drm.h"
#include "drm-uapi/msm_drm.h"
#include "sweet_drm.h"

#define SWEET_DRM_HANDLES 4096u
#define SWEET_DRM_BO_MAX (UINT64_C(256) << 20)
#define SWEET_DRM_INPUT_MAX 65536u

struct sbo {
	uint64_t iova, map, size;
	uint32_t seq;           /* last submit listing it */
	int used;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static io_connect_t conn;
static int dev_fd = -1;
static struct sbo *bos;
static uint32_t nbos;
static uint32_t chip_id, gmem_size;
static uint64_t gmem_base, max_freq;
static uint32_t queues;

/* Lock held. The GPU comes up a while after boot: callers may retry. */
static void
connect_dev(void)
{
    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("SweetGPUScatter"));
    if (!svc) return;
    io_connect_t opened = 0;
    kern_return_t kr = IOServiceOpen(svc, mach_task_self(), 0, &opened);
    IOObjectRelease(svc);
    if (kr != KERN_SUCCESS || !opened) return;
    uint64_t out[5] = {0};
    uint32_t n = 5;
    kr = IOConnectCallScalarMethod(opened, 15, NULL, 0, out, &n);
    if (kr != KERN_SUCCESS || n != 5 || (out[0] & UINT64_C(0xffff0000)) != UINT64_C(0x05040000)) {
        IOServiceClose(opened);
        return;
    }
    int file = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (file < 0) {
        IOServiceClose(opened);
        return;
    }
    /* Reserve all handle metadata before any GPU allocation/publication. */
    if (!bos) {
        bos = calloc(SWEET_DRM_HANDLES, sizeof(*bos));
        if (!bos) {
            close(file);
            IOServiceClose(opened);
            return;
        }
        nbos = SWEET_DRM_HANDLES;
    }
    chip_id = (uint32_t)out[0];
    gmem_size = (uint32_t)out[1];
    gmem_base = out[2];
    max_freq = out[3];
    conn = opened;
    dev_fd = file;
    sweet_drm_log("sweetgl: scatter service ready chip=%#x gmem=%u", chip_id, gmem_size);
}

int
sweet_drm_open(void)
{
	pthread_mutex_lock(&lock);
	if (!conn) {
		connect_dev();
	}
	int fd = conn && dev_fd >= 0 ? dup(dev_fd) : -1;
	pthread_mutex_unlock(&lock);
	return fd;
}

/* The kernel publishes this service only after its current-LIVE GPU start.
 * The render caller may wait for publication; it never initializes GPU here. */
int
sweet_drm_wait_ready(uint32_t timeout)
{
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t begin = (uint64_t)start.tv_sec * 1000 + (uint64_t)start.tv_nsec / 1000000;
    for (;;) {
        int fd = sweet_drm_open();
        if (fd >= 0) return fd;
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t elapsed = (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000 - begin;
        if (elapsed >= timeout) return -1;
        uint64_t remaining = timeout - elapsed;
        uint32_t pause = remaining < 250 ? (uint32_t)remaining : 250;
        struct timespec delay = {0, (long)pause * 1000000};
        nanosleep(&delay, NULL);
    }
}

int
drmOpenWithType(const char *name, const char *busid, int type)
{
	(void)busid;
	(void)type;
	return name && strcmp(name, "msm") == 0 ? sweet_drm_open() : -1;
}

/* Lock held: the bo for a handle, or NULL. */
static struct sbo *
sbo(uint32_t handle)
{
	return handle >= 1 && handle <= nbos && bos[handle - 1].size ? &bos[handle - 1] : NULL;
}

/* Lock held. */
static int
sbo_add(uint32_t handle, uint64_t iova, uint64_t map, uint64_t size)
{
    if (!bos || !handle || handle > SWEET_DRM_HANDLES || handle > nbos || !size ||
        size > SWEET_DRM_BO_MAX || iova < UINT64_C(0x40000000) || iova >= UINT64_C(0x80000000) ||
        size > UINT64_C(0x80000000) - iova || bos[handle - 1].size) return -EINVAL;
    bos[handle - 1] = (struct sbo){.iova = iova, .map = map, .size = size};
    return 0;
}

static int
fence_passed(uint32_t seq)
{
	uint64_t in[2] = {seq, 0};
	return IOConnectCallScalarMethod(conn, 14, in, 2, NULL, NULL) == KERN_SUCCESS;
}

/* Wait until seq completed, at most ms milliseconds (UINT64_MAX: forever). */
static int
fence_wait(uint32_t seq, uint64_t ms)
{
	if (fence_passed(seq)) {
		return 0;
	}
	for (;;) {
		uint64_t chunk = ms > 5000 ? 5000 : ms;
		uint64_t in[2] = {seq, chunk};
		kern_return_t kr = IOConnectCallScalarMethod(conn, 14, in, 2, NULL, NULL);
		if (kr == KERN_SUCCESS || fence_passed(seq)) {
			return 0;
		}
		if (kr != kIOReturnTimeout) {
			sweet_drm_log("sweetgl: wait for %u failed: %x", seq, kr);
			return -EIO;
		}
		if (ms != UINT64_MAX) {
			ms -= chunk;
			if (ms == 0) {
				return -ETIMEDOUT;
			}
		}
	}
}

/* msm timeouts are absolute CLOCK_MONOTONIC times; -> milliseconds from now. */
static uint64_t
timeout_ms(const struct drm_msm_timespec *t)
{
	struct timespec now = {0};
	if (!t || clock_gettime(CLOCK_MONOTONIC, &now) || t->tv_nsec < 0 || t->tv_nsec >= 1000000000)
		return 0;
	if (t->tv_sec < now.tv_sec) return 0;
	uint64_t seconds = (uint64_t)t->tv_sec - (uint64_t)now.tv_sec;
	if (seconds > 3600) return UINT64_MAX;
	int64_t ns = (int64_t)seconds * 1000000000ll + (t->tv_nsec - now.tv_nsec);
	return ns <= 0 ? 0 : (uint64_t)(ns / 1000000) + 1;
}

static int
get_param(struct drm_msm_param *p)
{
	switch (p->param) {
	case MSM_PARAM_GPU_ID:
		p->value = ((chip_id >> 24) & 255u) * 100u + ((chip_id >> 16) & 255u) * 10u + ((chip_id >> 8) & 255u);
		return 0;
	case MSM_PARAM_GMEM_SIZE:
		p->value = gmem_size;
		return 0;
	case MSM_PARAM_GMEM_BASE:
		p->value = gmem_base;
		return 0;
	case MSM_PARAM_CHIP_ID:
		p->value = chip_id;
		return 0;
	case MSM_PARAM_MAX_FREQ:
		p->value = max_freq;
		return 0;
	case MSM_PARAM_TIMESTAMP:
		/* the generic timer and RBBM_ALWAYSON both count XO (19.2 MHz) */
		p->value = mach_absolute_time();
		return 0;
	case MSM_PARAM_PRIORITIES:
		p->value = 1;
		return 0;
	case MSM_PARAM_FAULTS:
	case MSM_PARAM_SUSPENDS:
		p->value = 0;
		return 0;
	default:
		return -EINVAL;
	}
}

static int
gem_new(struct drm_msm_gem_new *r)
{
    if (!r || !r->size || r->size > SWEET_DRM_BO_MAX) return -EINVAL;
    uint64_t in = r->size, out[3] = {0};
    uint32_t n = 3;
    pthread_mutex_lock(&lock);
    kern_return_t kr = IOConnectCallScalarMethod(conn, 10, &in, 1, out, &n);
    int ret = -ENOMEM;
    if (kr == KERN_SUCCESS && n == 3 && out[0] && out[0] <= SWEET_DRM_HANDLES && out[2]) {
        ret = sbo_add((uint32_t)out[0], out[1], out[2], (r->size + 16383) & ~UINT64_C(16383));
        if (ret) IOConnectCallScalarMethod(conn, 11, out, 1, NULL, NULL);
    } else if (kr == KERN_SUCCESS) {
        ret = -EIO;
        if (out[0] && out[0] <= SWEET_DRM_HANDLES)
            IOConnectCallScalarMethod(conn, 11, out, 1, NULL, NULL);
    }
    if (!ret) r->handle = (uint32_t)out[0];
    pthread_mutex_unlock(&lock);
    if (ret) sweet_drm_log("sweetgl: scatter bo new %llu failed: %#x ret=%d", (unsigned long long)r->size, kr, ret);
    return ret;
}

static int
gem_close(uint32_t handle)
{
    pthread_mutex_lock(&lock);
    struct sbo *b = sbo(handle);
    int ret = -EINVAL;
    if (b) {
        uint64_t in = handle;
        kern_return_t kr = IOConnectCallScalarMethod(conn, 11, &in, 1, NULL, NULL);
        if (kr == KERN_SUCCESS) {
            memset(b, 0, sizeof(*b));
            ret = 0;
        } else ret = kr == kIOReturnTimeout ? -ETIMEDOUT : -EIO;
    }
    pthread_mutex_unlock(&lock);
    return ret;
}

static int
gem_info(struct drm_msm_gem_info *r)
{
	pthread_mutex_lock(&lock);
	struct sbo *b = sbo(r->handle);
	int ret = b ? 0 : -ENOENT;
	if (b) {
		switch (r->info) {
		case MSM_INFO_GET_OFFSET:
			r->value = (uint64_t)r->handle << 32;
			break;
		case MSM_INFO_GET_IOVA:
			r->value = b->iova;
			break;
		case MSM_INFO_SET_NAME:
		case MSM_INFO_SET_METADATA:
			break;
		case MSM_INFO_GET_FLAGS:
			r->value = MSM_BO_WC;
			break;
		default:
			ret = -EINVAL;
		}
	}
	pthread_mutex_unlock(&lock);
	return ret;
}

static int
cpu_prep(struct drm_msm_gem_cpu_prep *r)
{
	pthread_mutex_lock(&lock);
	struct sbo *b = sbo(r->handle);
	uint32_t seq = b ? b->seq : 0;
	int used = b && b->used;
	pthread_mutex_unlock(&lock);
	if (!b) {
		return -ENOENT;
	}
	if (used && !fence_passed(seq)) {
		if (r->op & MSM_PREP_NOSYNC) {
			return -EBUSY;
		}
		int e = fence_wait(seq, timeout_ms(&r->timeout));
		if (e) {
			return e;
		}
	}
	/* the bo is CPU-cached: drop stale lines over what the GPU wrote */
	uint64_t in = r->handle;
	return !used || IOConnectCallScalarMethod(conn, 16, &in, 1, NULL, NULL) == KERN_SUCCESS ? 0 : -EIO;
}

static int
gem_submit(struct drm_msm_gem_submit *r)
{
    if (!r || !r->nr_cmds || r->nr_cmds > 4096 || !r->nr_bos || r->nr_bos > SWEET_DRM_HANDLES ||
        !r->bos || !r->cmds || (r->flags & (MSM_SUBMIT_FENCE_FD_IN | MSM_SUBMIT_FENCE_FD_OUT |
        MSM_SUBMIT_SYNCOBJ_IN | MSM_SUBMIT_SYNCOBJ_OUT | MSM_SUBMIT_FENCE_SN_IN))) return -EINVAL;
    const struct drm_msm_gem_submit_bo *sb = (const void *)(uintptr_t)r->bos;
    const struct drm_msm_gem_submit_cmd *sc = (const void *)(uintptr_t)r->cmds;
    size_t slots = r->nr_cmds < 64 ? r->nr_cmds : 64;
    size_t len = 8 + slots * 16 + (size_t)r->nr_bos * 4;
    if (len > SWEET_DRM_INPUT_MAX) return -EINVAL;
    uint8_t *buf = malloc(len);
    if (!buf) return -ENOMEM;
    uint32_t *hdr = (uint32_t *)buf, nib = 0;
    uint8_t *ib = buf + 8;
    int ret = 0;
    pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < r->nr_cmds && !ret; ++i) {
        if (sc[i].type == MSM_SUBMIT_CMD_IB_TARGET_BUF) continue;
        struct sbo *b = sc[i].submit_idx < r->nr_bos ? sbo(sb[sc[i].submit_idx].handle) : NULL;
        if (!b || nib == 64 || (sc[i].type != MSM_SUBMIT_CMD_BUF && sc[i].type != MSM_SUBMIT_CMD_CTX_RESTORE_BUF) ||
            !sc[i].size || sc[i].size / 4 > 0x400000 || sc[i].nr_relocs || (sc[i].size & 3) ||
            (sc[i].submit_offset & 3) || sc[i].submit_offset > b->size || sc[i].size > b->size - sc[i].submit_offset) {
            ret = -EINVAL;
            break;
        }
        uint64_t va = b->iova + sc[i].submit_offset;
        uint32_t words[4] = {(uint32_t)va, (uint32_t)(va >> 32), sc[i].size / 4, 0};
        memcpy(ib + 16 * nib++, words, sizeof(words));
    }
    uint32_t *handles = (uint32_t *)(ib + 16 * nib);
    for (uint32_t i = 0; i < r->nr_bos && !ret; ++i) {
        if (!sbo(sb[i].handle)) ret = -EINVAL;
        handles[i] = sb[i].handle;
    }
    if (!ret && !nib) ret = -EINVAL;
    uint64_t sequence = 0;
    uint32_t n = 1;
    if (!ret) {
        hdr[0] = nib;
        hdr[1] = r->nr_bos;
        kern_return_t kr = IOConnectCallMethod(conn, 13, NULL, 0, buf, 8 + nib * 16 + (size_t)r->nr_bos * 4,
            &sequence, &n, NULL, NULL);
        /* A timed-out published submission still owns every listed BO. */
        if (n == 1 && sequence && sequence <= UINT32_MAX) {
            for (uint32_t i = 0; i < r->nr_bos; ++i) {
                struct sbo *b = sbo(sb[i].handle);
                b->seq = (uint32_t)sequence;
                b->used = 1;
            }
            r->fence = (uint32_t)sequence;
        } else if (kr == KERN_SUCCESS) ret = -EIO;
        if (kr != KERN_SUCCESS) {
            sweet_drm_log("sweetgl: scatter submit (%u IBs, %u bos) failed: %#x seq=%llu",
                nib, r->nr_bos, kr, (unsigned long long)sequence);
            ret = kr == kIOReturnTimeout ? -ETIMEDOUT : kr == kIOReturnNotReady ? -ENODEV : -EIO;
        }
    }
    pthread_mutex_unlock(&lock);
    free(buf);
    return ret;
}

static int
command(unsigned long index, void *data)
{
	if (!conn) {
		return -ENODEV;
	}
	switch (index) {
	case DRM_MSM_GET_PARAM:
		return get_param(data);
	case DRM_MSM_SET_PARAM:
		return 0;
	case DRM_MSM_GEM_NEW:
		return gem_new(data);
	case DRM_MSM_GEM_INFO:
		return gem_info(data);
	case DRM_MSM_GEM_CPU_PREP:
		return cpu_prep(data);
	case DRM_MSM_GEM_CPU_FINI:
		return 0;
	case DRM_MSM_GEM_SUBMIT:
		return gem_submit(data);
	case DRM_MSM_WAIT_FENCE: {
		struct drm_msm_wait_fence *r = data;
		return fence_wait(r->fence, timeout_ms(&r->timeout));
	}
	case DRM_MSM_GEM_MADVISE:
		((struct drm_msm_gem_madvise *)data)->retained = 1;
		return 0;
	case DRM_MSM_SUBMITQUEUE_NEW:
		((struct drm_msm_submitqueue *)data)->id = ++queues;
		return 0;
	case DRM_MSM_SUBMITQUEUE_CLOSE:
		return 0;
	case DRM_MSM_SUBMITQUEUE_QUERY: {
		struct drm_msm_submitqueue_query *q = data;
		if (q->param != MSM_SUBMITQUEUE_PARAM_FAULTS || q->len < 4) {
			return -EINVAL;
		}
		*(uint32_t *)(uintptr_t)q->data = 0;
		return 0;
	}
	default:
		sweet_drm_log("sweetgl: unhandled msm ioctl %lu", index);
		return -EINVAL;
	}
}

/* libdrm: 0 or -errno, errno set too */
int
drmCommandWrite(int fd, unsigned long index, void *data, unsigned long size)
{
	(void)fd;
	(void)size;
	int ret = command(index, data);
	if (ret) {
		errno = -ret;
	}
	return ret;
}

int
drmCommandWriteRead(int fd, unsigned long index, void *data, unsigned long size)
{
	return drmCommandWrite(fd, index, data, size);
}

int
drmIoctl(int fd, unsigned long request, void *arg)
{
	(void)fd;
	int ret;
	if (request == DRM_IOCTL_GEM_CLOSE) {
		ret = gem_close(((struct drm_gem_close *)arg)->handle);
	} else if (((request >> 8) & 0xff) == DRM_IOCTL_BASE && (request & 0xff) >= DRM_COMMAND_BASE &&
	    (request & 0xff) < DRM_COMMAND_END) {
		ret = command((request & 0xff) - DRM_COMMAND_BASE, arg);
	} else {
		sweet_drm_log("sweetgl: unhandled drm ioctl %lx", request);
		ret = -EINVAL;
	}
	if (ret) {
		errno = -ret;
		return -1;
	}
	return 0;
}

void *
sweet_drm_bo_map(int fd, uint64_t offset)
{
	(void)fd;
	pthread_mutex_lock(&lock);
	struct sbo *b = sbo((uint32_t)(offset >> 32));
	void *p = b && b->map ? (void *)(uintptr_t)b->map : (void *)-1;
	pthread_mutex_unlock(&lock);
	return p;
}

int
sweet_drm_import_iosurface(uint32_t surface_id, uint64_t map, uint32_t *handle, uint64_t *size)
{
    if (!conn) return -ENODEV;
    if (!surface_id || !handle || !size) return -EINVAL;
    uint64_t in = surface_id, out[3] = {0};
    uint32_t n = 3;
    pthread_mutex_lock(&lock);
    kern_return_t kr = IOConnectCallScalarMethod(conn, 12, &in, 1, out, &n);
    int ret = -ENOENT;
    if (kr == KERN_SUCCESS && n == 3 && out[0] && out[0] <= SWEET_DRM_HANDLES) {
        ret = sbo_add((uint32_t)out[0], out[1], map, out[2]);
        if (!ret) {
            *handle = (uint32_t)out[0];
            *size = out[2];
        } else IOConnectCallScalarMethod(conn, 11, out, 1, NULL, NULL);
    } else if (kr == KERN_SUCCESS) {
        ret = -EIO;
        if (out[0] && out[0] <= SWEET_DRM_HANDLES) IOConnectCallScalarMethod(conn, 11, out, 1, NULL, NULL);
    }
    static unsigned import_logged;
    if (import_logged++ < 96) sweet_drm_log("sweetgl: actual surface import id=%u kr=%x count=%u handle=%llu gpu=%#llx bytes=%llu ret=%d",
        surface_id, kr, n, (unsigned long long)out[0], (unsigned long long)out[1],
        (unsigned long long)out[2], ret);
    pthread_mutex_unlock(&lock);
    return ret;
}

static drmVersion version = {
	.version_major = 1, .version_minor = 7, .version_patchlevel = 0,
	.name_len = 3, .name = "msm", .date_len = 0, .date = "", .desc_len = 0, .desc = "",
};

drmVersionPtr
drmGetVersion(int fd)
{
	(void)fd;
	return conn ? &version : NULL;
}

void
drmFreeVersion(drmVersionPtr v)
{
	(void)v;
}

int
drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
	(void)fd;
	(void)capability;
	*value = 0;
	return -EINVAL;
}

int drmPrimeHandleToFD(int fd, uint32_t h, uint32_t f, int *p) { (void)fd; (void)h; (void)f; (void)p; return -ENOSYS; }
int drmPrimeFDToHandle(int fd, int p, uint32_t *h) { (void)fd; (void)p; (void)h; return -ENOSYS; }
int drmSyncobjCreate(int fd, uint32_t f, uint32_t *h) { (void)fd; (void)f; (void)h; return -ENOSYS; }
int drmSyncobjDestroy(int fd, uint32_t h) { (void)fd; (void)h; return -ENOSYS; }
int drmSyncobjHandleToFD(int fd, uint32_t h, int *o) { (void)fd; (void)h; (void)o; return -ENOSYS; }
int drmSyncobjFDToHandle(int fd, int o, uint32_t *h) { (void)fd; (void)o; (void)h; return -ENOSYS; }
int drmSyncobjImportSyncFile(int fd, uint32_t h, int s) { (void)fd; (void)h; (void)s; return -ENOSYS; }
int drmSyncobjExportSyncFile(int fd, uint32_t h, int *s) { (void)fd; (void)h; (void)s; return -ENOSYS; }
int drmSyncobjReset(int fd, const uint32_t *h, uint32_t n) { (void)fd; (void)h; (void)n; return -ENOSYS; }
int drmSyncobjWait(int fd, uint32_t *h, unsigned n, int64_t t, unsigned f, uint32_t *s)
{ (void)fd; (void)h; (void)n; (void)t; (void)f; (void)s; return -ENOSYS; }
int drmSyncobjTimelineWait(int fd, uint32_t *h, uint64_t *p, unsigned n, int64_t t, unsigned f, uint32_t *s)
{ (void)fd; (void)h; (void)p; (void)n; (void)t; (void)f; (void)s; return -ENOSYS; }

int drmSyncobjQuery(int fd, const uint32_t *h, uint64_t *p, uint32_t n)
{ (void)fd; (void)h; for (uint32_t i = 0; i < n; i++) p[i] = 0; return 0; }
int drmSyncobjSignal(int fd, const uint32_t *h, uint32_t n)
{ (void)fd; (void)h; (void)n; return 0; }
int drmSyncobjTimelineSignal(int fd, const uint32_t *h, uint64_t *p, uint32_t n)
{ (void)fd; (void)h; (void)p; (void)n; return 0; }
int drmSyncobjTransfer(int fd, uint32_t d, uint64_t dp, uint32_t s, uint64_t sp, uint32_t f)
{ (void)fd; (void)d; (void)dp; (void)s; (void)sp; (void)f; return 0; }

/* Same existing CPU_PREP fence/cache path, with a bounded five-second deadline. */
int sweet_drm_surface_sync(uint32_t handle)
{
    if (!conn) return -ENODEV;
    if (!handle || handle > SWEET_DRM_HANDLES) return -EINVAL;
    struct timespec now = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < 0 ||
        now.tv_sec > INT64_MAX - 5 || now.tv_nsec < 0 || now.tv_nsec >= 1000000000) return -EIO;
    struct drm_msm_gem_cpu_prep prep = {.handle = handle, .op = MSM_PREP_READ,
        .timeout = {.tv_sec = now.tv_sec + 5, .tv_nsec = now.tv_nsec}};
    return cpu_prep(&prep);
}
