/* SPDX-License-Identifier: BSD-2-Clause
 * Inspect the bytes already returned by one original XR getBytes call.
 * No texture read, draw, conversion or pixel write is added. */
#pragma once
#include "SnapshotRenderQueueObserve.h"
#import <Metal/Metal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

typedef void (*SweetReadbackEmit)(const char *, ...);
static atomic_bool snapshot_readback_observed;
enum { SWEET_READBACK_MAX_PIXELS = 1048576 };
static bool snapshot_pixel_bytes(const void *buffer, NSUInteger pitch, MTLRegion region,
    NSUInteger level, NSUInteger slice, NSUInteger format, unsigned pid, uint64_t call, SweetReadbackEmit emit,
    const char *kind,const char *source_tag)
{
    int incoming = errno;
    const uint64_t width=region.size.width,height=region.size.height;
    if(!buffer || !emit || !kind || !source_tag || !pid || !call || level || slice || region.origin.x || region.origin.y ||
        region.origin.z || region.size.depth!=1 || !width || !height || width>16384 || height>16384 ||
        width>SWEET_READBACK_MAX_PIXELS/height || pitch<width*4 || pitch>65536 ||
        width*4>UINTPTR_MAX-(uintptr_t)buffer ||
        (height-1)> (UINTPTR_MAX-(uintptr_t)buffer-width*4)/pitch ||
        (format!=MTLPixelFormatBGR10_XR && format!=MTLPixelFormatBGR10_XR_sRGB)) {
        errno=incoming;return false;
    }
    uint64_t hash=UINT64_C(14695981039346656037),black=0,nonblack=0,zero=0;
    uint32_t minimum[3]={1023,1023,1023},maximum[3]={0,0,0};
    const unsigned char *source=buffer;
    for(uint64_t y=0;y<height;++y) {
        const unsigned char *row=source+y*pitch;
        for(uint64_t x=0;x<width;++x) {
            uint32_t word=0;memcpy(&word,row+x*4,4);
            zero+=word==0;
            uint32_t channels[3]={(word>>20)&1023,(word>>10)&1023,word&1023};
            bool is_black=true;
            for(unsigned c=0;c<3;++c) {
                if(channels[c]<minimum[c])minimum[c]=channels[c];
                if(channels[c]>maximum[c])maximum[c]=channels[c];
                is_black&=channels[c]==384;
            }
            black+=is_black;nonblack+=!is_black;
            for(unsigned b=0;b<4;++b)hash=(hash^row[x*4+b])*UINT64_C(1099511628211);
        }
    }
    emit("SWEET %s BEGIN pid=%u call=%llu revision=1 width=%llu height=%llu row=%lu format=%lu %s=1",
        kind,pid,(unsigned long long)call,(unsigned long long)width,(unsigned long long)height,(unsigned long)pitch,(unsigned long)format,source_tag);
    emit("SWEET %s COUNT pid=%u call=%llu pixels=%llu black=%llu nonblack=%llu zero_words=%llu",
        kind,pid,(unsigned long long)call,(unsigned long long)(width*height),(unsigned long long)black,
        (unsigned long long)nonblack,(unsigned long long)zero);
    emit("SWEET %s RANGE pid=%u call=%llu rmin=%u rmax=%u gmin=%u gmax=%u bmin=%u bmax=%u",
        kind,pid,(unsigned long long)call,minimum[0],maximum[0],minimum[1],maximum[1],minimum[2],maximum[2]);
    const uint32_t coordinates[3][2]={{0,0},{(uint32_t)(width/2),(uint32_t)(height/2)},{(uint32_t)(width-1),(uint32_t)(height-1)}};
    for(unsigned i=0;i<3;++i) {
        uint32_t word=0;memcpy(&word,source+coordinates[i][1]*pitch+coordinates[i][0]*4,4);
        emit("SWEET %s PIXEL pid=%u call=%llu index=%u x=%u y=%u word=%08x",
            kind,pid,(unsigned long long)call,i,coordinates[i][0],coordinates[i][1],word);
    }
    emit("SWEET %s END pid=%u call=%llu logical_bytes=%llu fnv=%016llx original_bytes_not_changed=1",
        kind,pid,(unsigned long long)call,(unsigned long long)(width*height*4),(unsigned long long)hash);
    errno=incoming;return true;
}
static bool snapshot_readback_bytes(const void *buffer,NSUInteger pitch,MTLRegion region,NSUInteger level,
    NSUInteger slice,NSUInteger format,unsigned pid,uint64_t call,SweetReadbackEmit emit)
{
    return snapshot_pixel_bytes(buffer,pitch,region,level,slice,format,pid,call,emit,"RBYTES","original_getBytes");
}
static void snapshot_readback_once(id object,const void *bytes,NSUInteger row,MTLRegion region,
    NSUInteger level,NSUInteger slice,unsigned pid,uint64_t call,SweetReadbackEmit emit)
{
    int incoming=errno;
    @try {
        if(!object || strcmp(object_getClassName(object),"SweetMTLTexture"))return;
        Class owner=object_getClass(object);
        if(!render_queue_getter(owner,@selector(pixelFormat),@encode(NSUInteger)))return;
        NSUInteger format=[(id<MTLTexture>)object pixelFormat];
        if(format!=MTLPixelFormatBGR10_XR && format!=MTLPixelFormatBGR10_XR_sRGB)return;
        bool expected=false;
        if(!atomic_compare_exchange_strong_explicit(&snapshot_readback_observed,&expected,true,memory_order_acq_rel,memory_order_relaxed))return;
        if(!snapshot_readback_bytes(bytes,row,region,level,slice,format,pid,call,emit))
            emit("SWEET RBYTES REFUSED pid=%u call=%llu original_getBytes=1",pid,(unsigned long long)call);
    } @catch(id error) {
        (void)error;
        emit("SWEET RBYTES REFUSED pid=%u call=%llu original_getBytes=1",pid,(unsigned long long)call);
    } @finally {errno=incoming;}
}
