/* Included after AppleCLCD's definition. The USB callback only queues work;
 * capture runs on the display workloop, and paced transfer runs separately.
 * Existing USB DMA, allocator, SCM, PON, RPC and VM implementations are untouched. */
extern "C" int vermeer_loop_live_ready(void);
extern "C" void vermeer_debug_append(const char *, int);

static int sweet_screen_record(const char *format, ...)
{
    char line[512];
    va_list args;
    va_start(args, format);
    /* Each complete record is one ring append, including its leading newline.
     * This keeps concurrent console text out of packet metadata and pixels. */
    line[0] = '\n';
    int n = vsnprintf(line + 1, sizeof(line) - 32, format, args);
    va_end(args);
    if (n <= 0 || (size_t)n >= sizeof(line) - 32) return 0;
    uint32_t crc = sweet_screen_crc((const uint8_t *)line + 1, (size_t)n);
    int tail = snprintf(line + 1 + n, sizeof(line) - 1 - n, " wire_crc=%08x\n", crc);
    if (tail <= 0 || (size_t)tail >= sizeof(line) - 1 - n) return 0;
    vermeer_debug_append(line, 1 + n + tail);
    return 1;
}

void AppleCLCD::screenRequest()
{
    if (!fScreenEnabled || !fScreenPixels || !vermeer_loop_live_ready()) {
        sweet_screen_record("SWEET SCREEN REFUSE token=%s reason=unavailable", fScreenToken);
        return;
    }
    uint32_t expected = 0;
    if (!__atomic_compare_exchange_n(&fScreenState, &expected, 1u, false,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        sweet_screen_record("SWEET SCREEN REFUSE token=%s reason=busy", fScreenToken);
        return;
    }
    if (fScreenID >= SWEET_SCREEN_LIMIT) {
        sweet_screen_record("SWEET SCREEN REFUSE token=%s reason=limit", fScreenToken);
        __atomic_store_n(&fScreenState, 0u, __ATOMIC_RELEASE);
        return;
    }
    ++fScreenID;
    sweet_screen_record("SWEET SCREEN QUEUED token=%s id=%u read_only=1", fScreenToken, fScreenID);
    __atomic_store_n(&fScreenState, 2u, __ATOMIC_RELEASE);
}

static struct sweet_screen_regs sweet_screen_read_regs(AppleCLCD *fb)
{
    struct sweet_screen_regs r = {};
    r.source = fb->fVig0[0x14/4]; r.stride = fb->fVig0[0x24/4];
    r.size = fb->fVig0[0x00/4]; r.image = fb->fVig0[0x04/4];
    r.xy = fb->fVig0[0x08/4]; r.output = fb->fVig0[0x0c/4];
    r.output_xy = fb->fVig0[0x10/4]; r.format = fb->fVig0[0x30/4];
    r.unpack = fb->fVig0[0x34/4]; r.op = fb->fVig0[0x38/4];
    r.qseed = fb->fVig0[0xa04/4]; r.ctl = fb->fCtl0[0x14/4];
    r.start = fb->fCtl0[0x1c/4];
    return r;
}

/* Called on the same workloop as present/probe/fade, after that tick's writes.
 * No display writer runs while this private immutable copy is made. The RGB888
 * boot buffer is separate from all user/GPU four-byte IOSurfaces. */
void AppleCLCD::screenPump()
{
    if (__atomic_load_n(&fScreenState, __ATOMIC_ACQUIRE) != 2u) return;
    __atomic_store_n(&fScreenState, 3u, __ATOMIC_RELEASE);
    const char *failure = NULL;
    IOByteCount length = 0;
    uint64_t mapPA = fPanelMemory ? fPanelMemory->getPhysicalSegment(0, &length, kIOMemoryMapperNone) : 0;
    if (!vermeer_loop_live_ready()) failure = "return_pending";
    else if (!fBootScan || fDirectMode || fScanSurface || fProbe || !sweet_console_muted ||
        !fVig0 || !fCtl0 || !fMap || fMap->getVirtualAddress() != fPixels ||
        fWidth != SWEET_SCREEN_WIDTH || fHeight != SWEET_SCREEN_HEIGHT ||
        fRowBytes != SWEET_SCREEN_STRIDE || mapPA != fPhys || length < SWEET_SCREEN_BYTES)
        failure = "unsupported_source";
    if (!failure) {
        fScreenRegs = sweet_screen_read_regs(this);
        if (!sweet_screen_contract(&fScreenRegs, mapPA, length)) failure = "scan_contract";
    }
    if (!failure) {
        fScreenTime = mach_absolute_time();
        fScreenVsync = fVsyncCount;
        IOLockLock(fLock);
        fScreenPowerOff = fPowerOff;
        IOLockUnlock(fLock);
        memcpy(fScreenPixels, (const void *)fPixels, SWEET_SCREEN_BYTES);
        fScreenHash = sweet_screen_fnv(fScreenPixels, SWEET_SCREEN_BYTES);
        uint64_t liveHash = sweet_screen_fnv((const uint8_t *)fPixels, SWEET_SCREEN_BYTES);
        struct sweet_screen_regs after = sweet_screen_read_regs(this);
        fScreenCopyTicks = mach_absolute_time() - fScreenTime;
        if (liveHash != fScreenHash || !sweet_screen_regs_equal(&fScreenRegs, &after) ||
            fScreenVsync != fVsyncCount) failure = "source_changed";
    }
    if (!failure) {
        thread_t thread;
        if (kernel_thread_start(&AppleCLCD::screenThread, this, &thread) == KERN_SUCCESS) {
            thread_deallocate(thread);
            return;
        }
        failure = "thread";
    }
    sweet_screen_record("SWEET SCREEN ABORT token=%s id=%u reason=%s", fScreenToken, fScreenID, failure);
    __atomic_store_n(&fScreenState, 0u, __ATOMIC_RELEASE);
}

struct sweet_screen_encoder_stats { size_t bytes; uint32_t crc; };
static int sweet_screen_count(void *context, const uint8_t *data, size_t count)
{
    struct sweet_screen_encoder_stats *s = (struct sweet_screen_encoder_stats *)context;
    s->bytes += count;
    s->crc = sweet_screen_crc_update(s->crc, data, count);
    return 1;
}

struct sweet_screen_transfer {
    AppleCLCD *fb;
    uint8_t bytes[SWEET_SCREEN_CHUNK];
    size_t used, offset;
    uint32_t blocks;
};

static int sweet_screen_flush(struct sweet_screen_transfer *s)
{
    if (!s->used) return 1;
    if (!vermeer_loop_live_ready()) return 0;
    static const char hex[] = "0123456789abcdef";
    char payload[SWEET_SCREEN_CHUNK * 2 + 1];
    for (size_t i = 0; i < s->used; ++i) {
        payload[i*2] = hex[s->bytes[i] >> 4]; payload[i*2+1] = hex[s->bytes[i] & 15];
    }
    payload[s->used*2] = 0;
    if (!sweet_screen_record("SWEET SCREEN DATA token=%s id=%u offset=%lu n=%lu crc=%08x hex=%s",
        s->fb->fScreenToken, s->fb->fScreenID, (unsigned long)s->offset,
        (unsigned long)s->used, sweet_screen_crc(s->bytes, s->used), payload)) return 0;
    s->offset += s->used; s->used = 0; ++s->blocks;
    /* Worst-case output is bounded to about 32 KiB encoded pixels/s. The sole
     * USB worker drains the normal ring and accepts commands throughout. */
    if ((s->blocks % 16u) == 0) {
        uint64_t wake;
        clock_interval_to_deadline(64, NSEC_PER_MSEC, &wake);
        (void)assert_wait_deadline((event_t)&s->fb->fScreenState, THREAD_UNINT, wake);
        thread_block(THREAD_CONTINUE_NULL);
    }
    return 1;
}

static int sweet_screen_send(void *context, const uint8_t *data, size_t count)
{
    struct sweet_screen_transfer *s = (struct sweet_screen_transfer *)context;
    while (count) {
        size_t n = SWEET_SCREEN_CHUNK - s->used;
        if (n > count) n = count;
        memcpy(s->bytes + s->used, data, n); s->used += n; data += n; count -= n;
        if (s->used == SWEET_SCREEN_CHUNK && !sweet_screen_flush(s)) return 0;
    }
    return 1;
}

void AppleCLCD::screenThread(void *argument, wait_result_t result)
{
    (void)result;
    AppleCLCD *fb = (AppleCLCD *)argument;
    struct sweet_screen_encoder_stats stats = {0, 0xffffffffu};
    struct sweet_screen_transfer transfer = {fb, {}, 0, 0, 0};
    const struct sweet_screen_regs *r = &fb->fScreenRegs;
    bool complete = vermeer_loop_live_ready() &&
        sweet_screen_encode(fb->fScreenPixels, SWEET_SCREEN_BYTES, sweet_screen_count, &stats);
    stats.crc ^= 0xffffffffu;
    if (complete) complete = sweet_screen_record(
        "SWEET SCREEN BEGIN token=%s id=%u width=720 height=1280 stride=2160 format=BGR888 bytes=%u codec=PRLE1 encoded=%lu raw_fnv=%016llx enc_crc=%08x vsync=%llu timestamp=%llu copy_ticks=%llu power_off=%u source_checked=1 source_match=1",
        fb->fScreenToken, fb->fScreenID, SWEET_SCREEN_BYTES, (unsigned long)stats.bytes,
        fb->fScreenHash, stats.crc, fb->fScreenVsync, fb->fScreenTime,
        fb->fScreenCopyTicks, fb->fScreenPowerOff);
    if (complete) complete = sweet_screen_record(
        "SWEET SCREEN SCAN token=%s id=%u pa=%#x map_pa=%#llx map_bytes=%u size=%#x img=%#x xy=%#x out=%#x outxy=%#x fmt=%#x unpack=%#x op=%#x qop=%#x ctl=%#x start=%#x before_after=1",
        fb->fScreenToken, fb->fScreenID, r->source, fb->fPhys, SWEET_SCREEN_BYTES,
        r->size, r->image, r->xy, r->output, r->output_xy, r->format, r->unpack,
        r->op, r->qseed, r->ctl, r->start);
    if (complete) complete = sweet_screen_encode(fb->fScreenPixels, SWEET_SCREEN_BYTES, sweet_screen_send, &transfer) &&
        sweet_screen_flush(&transfer) && transfer.offset == stats.bytes;
    if (complete) sweet_screen_record(
        "SWEET SCREEN END token=%s id=%u encoded=%lu blocks=%u raw_fnv=%016llx enc_crc=%08x complete=1",
        fb->fScreenToken, fb->fScreenID, (unsigned long)transfer.offset, transfer.blocks, fb->fScreenHash, stats.crc);
    else sweet_screen_record("SWEET SCREEN ABORT token=%s id=%u reason=transfer", fb->fScreenToken, fb->fScreenID);
    __atomic_store_n(&fb->fScreenState, 0u, __ATOMIC_RELEASE);
    thread_terminate(current_thread());
}
