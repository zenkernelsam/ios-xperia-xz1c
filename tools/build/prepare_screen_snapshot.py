"""Prepare a read-only v730 display snapshot delta from the exact v729 source."""
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sha = lambda data: hashlib.sha256(data).hexdigest()
SOURCE = 'iokit/Kernel/arm/SweetFramebuffer.cpp'
HEADERS = ('SweetFramebufferScreen.h', 'SweetFramebufferScreenRuntime.h')


def changed_source(before):
    text = before.decode('utf-8')

    def replace(old, new):
        nonlocal text
        assert text.count(old) == 1, old
        text = text.replace(old, new)

    replace('#include "BootEvidenceUtils.h"', '#include "BootEvidenceUtils.h"\n#include "SweetFramebufferScreen.h"')
    replace('#include <kern/clock.h>', '#include <kern/clock.h>\n#include <kern/thread.h>')
    replace('\tvoid shot();', '''\tvoid shot();
    int fScreenEnabled;
    uint32_t fScreenState, fScreenID;
    char fScreenToken[17];
    uint8_t *fScreenPixels;
    struct sweet_screen_regs fScreenRegs;
    uint64_t fScreenHash, fScreenVsync, fScreenTime, fScreenCopyTicks;
    bool fScreenPowerOff;
    void screenRequest();
    void screenPump();
    static void screenThread(void *, wait_result_t);''')
    replace('\tfBootScan = fKick = fProbe = 0;', '''\tfBootScan = fKick = fProbe = 0;
    fScreenEnabled = 0; fScreenState = 0; fScreenID = 0; fScreenPixels = NULL;
    memset(fScreenToken, 0, sizeof(fScreenToken));
    PE_parse_boot_argn("sfscreen", &fScreenEnabled, sizeof(fScreenEnabled));
    PE_parse_boot_argn("sweetloop", fScreenToken, sizeof(fScreenToken));
    bool validScreenToken = strlen(fScreenToken) == 16;
    for (unsigned i = 0; i < 16 && validScreenToken; ++i)
        validScreenToken = (fScreenToken[i] >= '0' && fScreenToken[i] <= '9') ||
            (fScreenToken[i] >= 'a' && fScreenToken[i] <= 'f');
    fScreenEnabled = fScreenEnabled == 1 && validScreenToken;''')
    replace('\tif (fShotTick != 0) {', '\tif (fShotTick != 0 || fScreenEnabled) {')
    replace('\tmemset((void *)fPixels,0,(size_t)fHeight*fRowBytes);', '''    if (fScreenEnabled) {
        fScreenPixels = (uint8_t *)IOMallocData(SWEET_SCREEN_BYTES);
        if (fScreenPixels) IOLog("SWEET SCREEN READY proto=1 token=%s bytes=%u read_only=1\\n", fScreenToken, SWEET_SCREEN_BYTES);
        else IOLog("SWEET SCREEN UNAVAILABLE reason=memory\\n");
    }
\tmemset((void *)fPixels,0,(size_t)fHeight*fRowBytes);''')
    replace('\t\tfb->fNextVsync = sweet_fb_next_deadline(fb->fNextVsync, fb->fVsyncInterval, mach_absolute_time());',
            '\t\tfb->screenPump();\n\t\tfb->fNextVsync = sweet_fb_next_deadline(fb->fNextVsync, fb->fVsyncInterval, mach_absolute_time());')
    replace('\tif (fb->fShotTick != 0 && (count % fb->fShotTick) == 0 &&',
            '\tfb->screenPump();\n\tif (fb->fShotTick != 0 && (count % fb->fShotTick) == 0 &&')
    replace('\t\tgSweetFB->shot();', '\t\tif (gSweetFB->fScreenEnabled) gSweetFB->screenRequest();\n\t\telse gSweetFB->shot();')
    replace('void\nAppleCLCD::addClient(IOMobileFramebufferUserClient *client)',
            '#include "SweetFramebufferScreenRuntime.h"\n\nvoid\nAppleCLCD::addClient(IOMobileFramebufferUserClient *client)')
    return text.encode('utf-8')


def main():
    stage = ROOT / 'build/scanout-snapshot-v730'
    candidate = stage / 'candidate'
    assert not candidate.exists(), 'Keep prior generated outputs; use a fresh candidate'
    before_actual = (stage / 'before/SweetFramebuffer.cpp').read_bytes()
    lock = json.loads((ROOT / 'reproducibility/source-lock.json').read_bytes())
    assert sha(before_actual) == lock['audited_xnu_files'][SOURCE]
    before = before_actual.replace(b'\r\n', b'\n')
    after = changed_source(before)
    candidate.mkdir()
    (candidate / 'SweetFramebuffer.cpp').write_bytes(after)
    delta = ''.join(difflib.unified_diff(before.decode().splitlines(True), after.decode().splitlines(True),
        fromfile='a/' + SOURCE, tofile='b/' + SOURCE))
    overrides = dict(json.loads((ROOT / 'reproducibility/surface-diag-v729.json').read_bytes())['overrides'])
    overrides[SOURCE] = sha(after)
    changed = [SOURCE]
    for name in HEADERS:
        data = (ROOT / 'xnu/support' / name).read_bytes()
        (candidate / name).write_bytes(data)
        path = 'iokit/Kernel/arm/' + name
        delta += ''.join(difflib.unified_diff([], data.decode().splitlines(True), fromfile='/dev/null', tofile='b/' + path))
        overrides[path] = sha(data)
        changed.append(path)
    patch = ROOT / 'xnu/lilac-scanout-snapshot.patch'
    manifest = ROOT / 'reproducibility/surface-diag-v730.json'
    assert not patch.exists() and not manifest.exists()
    patch.write_bytes(delta.encode())
    report = dict(version=730, base_version=729, base_sources={SOURCE: sha(before)},
        prior_actual_sources={SOURCE: sha(before_actual)}, changed_sources=changed,
        overrides=overrides, patch_sha256=sha(delta.encode()), read_only_scanout_snapshot=True,
        USB_worker_only_queues=True, capture_on_display_workloop=True, immutable_transfer_buffer=True,
        per_record_and_payload_CRC32=True, raw_FNV64=True, format='BGR888', width=720, height=1280, stride=2160,
        GPU_allocator_SCM_PON_RPC_VM_code_unchanged=True, hardware_acceptance_pending=True)
    manifest.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
