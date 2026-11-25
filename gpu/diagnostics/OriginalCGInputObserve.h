/* SPDX-License-Identifier: BSD-2-Clause
 * Inspect one already returned original CIF10 CGImage before its PNG encoding. */
#pragma once
#include "SnapshotReadbackObserve.h"
#include <CoreGraphics/CoreGraphics.h>

static atomic_bool original_cg_input_observed;
static bool snapshot_cg_input_once(CGImageRef image,unsigned pid,uint64_t call,SweetReadbackEmit emit)
{
    int incoming=errno;CFDataRef data=NULL;bool success=false;
    if(!image || !emit || !pid || !call || CGImageGetBitsPerComponent(image)!=10 ||
        CGImageGetBitsPerPixel(image)!=32 || CGImageGetBitmapInfo(image)!=0x42000) {
        errno=incoming;return false;
    }
    size_t width=CGImageGetWidth(image),height=CGImageGetHeight(image),pitch=CGImageGetBytesPerRow(image);
    if(!width || !height || width>SWEET_READBACK_MAX_PIXELS/height || pitch<width*4 || pitch>65536 ||
        height>(4u<<20)/pitch) {errno=incoming;return false;}
    bool expected=false;
    if(!atomic_compare_exchange_strong_explicit(&original_cg_input_observed,&expected,true,memory_order_acq_rel,memory_order_relaxed)) {
        errno=incoming;return false;
    }
    @try {
        CGDataProviderRef provider=CGImageGetDataProvider(image);
        data=provider?CGDataProviderCopyData(provider):NULL;
        if(data && CFDataGetLength(data)>0 && (size_t)CFDataGetLength(data)>=pitch*height &&
            (size_t)CFDataGetLength(data)<=(4u<<20)) {
            success=snapshot_pixel_bytes(CFDataGetBytePtr(data),pitch,MTLRegionMake2D(0,0,width,height),0,0,555,
                pid,call,emit,"CGBYTES","original_CGImage");
        }
        if(!success)emit("SWEET CGBYTES REFUSED pid=%u call=%llu original_CGImage=1",pid,(unsigned long long)call);
    } @catch(id error) {
        (void)error;emit("SWEET CGBYTES REFUSED pid=%u call=%llu original_CGImage=1",pid,(unsigned long long)call);
    } @finally {
        if(data)CFRelease(data);errno=incoming;
    }
    return success;
}
