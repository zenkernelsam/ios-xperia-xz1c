#!/usr/bin/env python3
"""v154: panel transform sampler for the RGB888 copy path.

Adds boot-arg sweetfb_xf (0..7) applied when converting a client layer:
  0 identity (panel crop, current)
  1 flip x, 2 flip y, 3 flip xy
  4 transpose+scale (panel x <- scene y, panel y <- scene x)
  5..7 transpose with flips
Keeps the boot-arg in one place so orientation can be tried without code churn.
"""
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/SweetFramebuffer.cpp'
s = open(p).read()

# member
old = '\tuint32_t      fSurfRowBytes;   /* 4-byte IOSurface stride */\n'
assert s.count(old) == 1
s = s.replace(old, old + '\tint           fXform;          /* sweetfb_xf: panel transform */\n')

# read boot arg after fOffY setup
old2 = '''\tprovider->setProperty("canvas-width", fLogicalW, 32);'''
if s.count(old2) == 0:
    old2 = '''\t/* The ADT's disp0 node already carries canvas-width/height as 4-byte
\t * CFData (1125x2436); do not shadow them with OSNumber properties -
\t * iomfb.framework calls -length on whatever it finds. */
'''
    new2 = old2 + '''\tfXform = 0;
\tPE_parse_boot_argn("sweetfb_xf", &fXform, sizeof(fXform));
\tif (fXform < 0 || fXform > 7) {
\t\tfXform = 0;
\t}
'''
    assert s.count(old2) == 1
    s = s.replace(old2, new2)
else:
    raise SystemExit('unexpected canvas block')

# replace copy loop 3bpp branch with transform-aware sampler
old3 = '''\t\t\t} else {
\t\t\t\t/* Panel scans RGB888: BGRA -> BGR, panel stride. */
\t\t\t\tfor (int32_t y = 0; y < h; y++) {
\t\t\t\t\tvolatile uint8_t *d = (volatile uint8_t *)(fPixels +
\t\t\t\t\t    (uint64_t)(dy + y) * fRowBytes + (uint64_t)dx * 3);
\t\t\t\t\tconst uint8_t *srow = sbase +
\t\t\t\t\t    (uint64_t)(sy + y) * s->bytesPerRow + (uint64_t)sx * 4;
\t\t\t\t\tfor (int32_t x = 0; x < w; x++) {
\t\t\t\t\t\td[x * 3 + 0] = srow[x * 4 + 0];
\t\t\t\t\t\td[x * 3 + 1] = srow[x * 4 + 1];
\t\t\t\t\t\td[x * 3 + 2] = srow[x * 4 + 2];
\t\t\t\t\t}
\t\t\t\t}
\t\t\t}
'''
assert s.count(old3) == 1
new3 = '''\t\t\t} else if (fXform == 0) {
\t\t\t\t/* Panel scans RGB888: BGRA -> BGR, panel stride. */
\t\t\t\tfor (int32_t y = 0; y < h; y++) {
\t\t\t\t\tvolatile uint8_t *d = (volatile uint8_t *)(fPixels +
\t\t\t\t\t    (uint64_t)(dy + y) * fRowBytes + (uint64_t)dx * 3);
\t\t\t\t\tconst uint8_t *srow = sbase +
\t\t\t\t\t    (uint64_t)(sy + y) * s->bytesPerRow + (uint64_t)sx * 4;
\t\t\t\t\tfor (int32_t x = 0; x < w; x++) {
\t\t\t\t\t\td[x * 3 + 0] = srow[x * 4 + 0];
\t\t\t\t\t\td[x * 3 + 1] = srow[x * 4 + 1];
\t\t\t\t\t\td[x * 3 + 2] = srow[x * 4 + 2];
\t\t\t\t\t}
\t\t\t\t}
\t\t\t} else {
\t\t\t\t/* sweetfb_xf: resample the whole surface onto the panel.
\t\t\t\t * 4 = transpose: panel x follows scene y, panel y follows
\t\t\t\t * scene x; 1/2/3 flip, 5..7 transpose + flips. */
\t\t\t\tint flipx = (fXform == 1 || fXform == 3 || fXform == 5 || fXform == 7);
\t\t\t\tint flipy = (fXform == 2 || fXform == 3 || fXform == 6 || fXform == 7);
\t\t\t\tint swap = (fXform >= 4);
\t\t\t\tuint32_t sw = s->width, sh = s->height;
\t\t\t\tuint32_t pw = fWidth, ph = fHeight;
\t\t\t\tfor (uint32_t py = 0; py < ph; py++) {
\t\t\t\t\tvolatile uint8_t *d = (volatile uint8_t *)(fPixels +
\t\t\t\t\t    (uint64_t)py * fRowBytes);
\t\t\t\t\tfor (uint32_t px = 0; px < pw; px++) {
\t\t\t\t\t\tuint32_t u, v;
\t\t\t\t\t\tif (swap) {
\t\t\t\t\t\t\tu = (uint32_t)((uint64_t)py * sw / ph);
\t\t\t\t\t\t\tv = (uint32_t)((uint64_t)px * sh / pw);
\t\t\t\t\t\t} else {
\t\t\t\t\t\t\tu = (uint32_t)((uint64_t)px * sw / pw);
\t\t\t\t\t\t\tv = (uint32_t)((uint64_t)py * sh / ph);
\t\t\t\t\t\t}
\t\t\t\t\t\tif (flipx) {
\t\t\t\t\t\t\tu = sw - 1 - u;
\t\t\t\t\t\t}
\t\t\t\t\t\tif (flipy) {
\t\t\t\t\t\t\tv = sh - 1 - v;
\t\t\t\t\t\t}
\t\t\t\t\t\tconst uint8_t *sp = sbase +
\t\t\t\t\t\t    (uint64_t)v * s->bytesPerRow + (uint64_t)u * 4;
\t\t\t\t\t\td[px * 3 + 0] = sp[0];
\t\t\t\t\t\td[px * 3 + 1] = sp[1];
\t\t\t\t\t\td[px * 3 + 2] = sp[2];
\t\t\t\t\t}
\t\t\t\t}
\t\t\t}
'''
s = s.replace(old3, new3)
open(p, 'w').write(s)
print('v154 patched')
