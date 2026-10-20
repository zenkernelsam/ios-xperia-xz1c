import re, os

pd = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'xnu', 'patches')

def patch(name, subs):
    p = os.path.join(pd, name)
    raw = open(p, 'rb').read()
    for a, b in subs:
        a, b = a.encode(), b.encode()
        if a not in raw:
            print(f'  MISS in {name}: {a[:60]}')
        raw = raw.replace(a, b)
    open(p, 'wb').write(raw)
    print(f'{name} done')

# 0012: context lines must match new 0010 output
patch('0012-sweet-keep-debug-console-after-ttbr0.patch', [
    (' #define EARLY_WIDTH 1080u', ' #define EARLY_WIDTH 720u'),
    (' #define EARLY_HEIGHT 2400u', ' #define EARLY_HEIGHT 1280u'),
    (' #define EARLY_STRIDE (4320u / 4)', ' #define EARLY_STRIDE (2880u / 4)'),
])

# 0046: PPI 395 (6.67in 1080x2400) -> 319 (4.6in 720x1280)
patch('0046-sweet-iosurface-iomfb.patch', [
    ('#define SWEET_FB_DOT_PITCH              395     /* ppi, 6.67" 1080x2400 */',
     '#define SWEET_FB_DOT_PITCH              319     /* ppi, 4.6" 720x1280 lilac */'),
])

# 0058: touch scale -> lilac dims
patch('0058-sweet-hid-event-services.patch', [
    ('/ 1080);', '/ 720);'),
    ('/ 2400);', '/ 1280);'),
])

# 0060: printf string
patch('0060-sweet-keys-touch-drivers.patch', [
    ('1080x2400, poll=10ms', '720x1280, poll=10ms'),
])

# 0061: both old-side (context) and new-side dims -> lilac
patch('0061-sweet-iphone-x-display-size.patch', [
    ('fLogicalW = native ? 1080 : 1125;', 'fLogicalW = native ? 720 : 1125;'),
    ('fLogicalH = native ? 2400 : 2436;', 'fLogicalH = native ? 1280 : 2436;'),
    ('fOffX = (fLogicalW - 1080) / 2;', 'fOffX = (fLogicalW - 720) / 2;'),
    ('fOffY = (fLogicalH - 2400) / 2;', 'fOffY = (fLogicalH - 1280) / 2;'),
    ('/ 1080);', '/ 720);'),
    ('/ 2400);', '/ 1280);'),
])

# sweep: any remaining sweet FB/display constants in patch series
for f in sorted(os.listdir(pd)):
    if not f.endswith('.patch'):
        continue
    s = open(os.path.join(pd, f), 'rb').read().decode('utf-8', 'replace')
    for tok in ('0x9c000000', '/ 1080)', '/ 2400)', 'EARLY_WIDTH 1080',
                'EARLY_HEIGHT 2400', 'EARLY_STRIDE (4320'):
        if tok in s:
            print(f'  STALE {tok!r} still in {f}')
