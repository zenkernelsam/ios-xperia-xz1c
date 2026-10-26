#!/usr/bin/env python3
import sys
p = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/iokit/Kernel/arm/SweetFramebuffer.cpp'
s = open(p).read()
n = 0

def sub(old, new):
    global s, n
    assert s.count(old) == 1, (s.count(old), old[:60])
    s = s.replace(old, new)
    n += 1

sub("\tuint32_t      fWidth, fHeight, fRowBytes;\n",
    "\tuint32_t      fWidth, fHeight, fRowBytes;\n\tuint32_t      fSurfRowBytes;   /* 4-byte IOSurface stride */\n")

sub("""\tPE_current_console(&video);
\tfPhys = video.v_baseAddr;
\tfWidth = (uint32_t)video.v_width;
\tfHeight = (uint32_t)video.v_height;
\tfRowBytes = (uint32_t)video.v_rowBytes;
\tif (fPhys == 0 || fWidth == 0 || fHeight == 0 || fRowBytes < fWidth * 4) {
\t\tIOLog("SWEET FB: no boot framebuffer\\n");
\t\treturn false;
\t}
""",
"""\tPE_current_console(&video);
\tfPhys = video.v_baseAddr;
\tfWidth = (uint32_t)video.v_width;
\tfHeight = (uint32_t)video.v_height;
\tfRowBytes = (uint32_t)video.v_rowBytes;
\tIOLog("SWEET FB: boot video 0x%llx %ux%u stride %u depth %u display %u\\n",
\t    fPhys, fWidth, fHeight, fRowBytes, (uint32_t)video.v_depth,
\t    (uint32_t)video.v_display);
\tif (fPhys == 0 || fWidth == 0 || fHeight == 0 || fRowBytes < fWidth * 3) {
\t\tIOLog("SWEET FB: no boot framebuffer\\n");
\t\treturn false;
\t}
\t/* The lilac splash console is RGB888 (3 bytes/px), but IOSurface and
\t * IOMobileFramebuffer clients work in 4-byte elements. Publish a
\t * 4-byte view of the panel memory (the splash reservation is 32 MB);
\t * the MDP keeps scanning the console stride until a client surface
\t * is presented. */
\tfSurfRowBytes = fRowBytes >= fWidth * 4 ? fRowBytes : fWidth * 4;
""")

sub("\tfSurface = sweet_surface_create_physical(fPhys, fWidth, fHeight, fRowBytes, SWEET_FB_FORMAT_BGRA);\n",
    "\tfSurface = sweet_surface_create_physical(fPhys, fWidth, fHeight, fSurfRowBytes, SWEET_FB_FORMAT_BGRA);\n")

sub("""\tfOffX = (int32_t)(fLogicalW - fWidth) / 2;
\tfOffY = (int32_t)(fLogicalH - fHeight) / 2;
""",
"""\tfOffX = (int32_t)(fLogicalW - fWidth) / 2;
\tfOffY = (int32_t)(fLogicalH - fHeight) / 2;
\t/* iomfb.framework reads the canvas from the service's provider. */
\tprovider->setProperty("canvas-width", fLogicalW, 32);
\tprovider->setProperty("canvas-height", fLogicalH, 32);
""")

sub("""\tIOLog("SWEET FB: AppleCLCD %ux%u stride %u at 0x%llx, default surface %u, presented as %ux%u\\n",
\t    fWidth, fHeight, fRowBytes, fPhys, fSurface->id, fLogicalW, fLogicalH);
""",
"""\tIOLog("SWEET FB: AppleCLCD %ux%u stride %u (surface %u) at 0x%llx, default surface %u, presented as %ux%u\\n",
\t    fWidth, fHeight, fRowBytes, fSurfRowBytes, fPhys, fSurface->id, fLogicalW, fLogicalH);
""")

sub("\t\t\tvolatile uint32_t *row = (volatile uint32_t *)(fb->fPixels + (uint64_t)y * fb->fRowBytes);\n",
    "\t\t\tvolatile uint32_t *row = (volatile uint32_t *)(fb->fPixels + (uint64_t)y * fb->fSurfRowBytes);\n")

sub("\t\t\t\tmemcpy((void *)(fPixels + (uint64_t)(dy + y) * fRowBytes + (uint64_t)dx * 4),\n",
    "\t\t\t\tmemcpy((void *)(fPixels + (uint64_t)(dy + y) * fSurfRowBytes + (uint64_t)dx * 4),\n")

open(p, 'w').write(s)
print("patched %d hunks" % n)
