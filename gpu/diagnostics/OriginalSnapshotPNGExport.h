/* SPDX-License-Identifier: BSD-2-Clause
 * Export one original returned PNG, unchanged and bounded, on a background queue. */
#pragma once
#import <Foundation/Foundation.h>
#include <dispatch/dispatch.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

typedef void (*SweetOriginalPNGEmit)(const char *, ...);
static atomic_bool original_png_export_scheduled;
enum { ORIGINAL_PNG_EXPORT_MAX = 65536, ORIGINAL_PNG_EXPORT_CHUNK = 48 };
static uint64_t original_png_fnv(const uint8_t *data, size_t bytes)
{
    uint64_t value=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes;++i)value=(value^data[i])*UINT64_C(1099511628211);
    return value;
}
static bool original_png_export(NSData *data, unsigned pid, uint64_t call, SweetOriginalPNGEmit emit)
{
    int incoming=errno;
    static const uint8_t signature[]={137,80,78,71,13,10,26,10};
    if(!data || !emit || !pid || !call || data.length<sizeof(signature) || data.length>ORIGINAL_PNG_EXPORT_MAX ||
        memcmp(data.bytes,signature,sizeof(signature))) { errno=incoming;return false; }
    size_t bytes=data.length;const uint8_t *source=data.bytes;uint64_t fnv=original_png_fnv(source,bytes);
    uLongf packed_bytes=compressBound((uLong)bytes);uint8_t *packed=malloc(packed_bytes);
    if(!packed || compress2(packed,&packed_bytes,source,(uLong)bytes,6)!=Z_OK) { free(packed);errno=incoming;return false; }
    uint64_t packed_fnv=original_png_fnv(packed,(size_t)packed_bytes);
    emit("SWEET OPNG BEGIN pid=%u call=%llu revision=2 bytes=%zu fnv=%016llx codec=ZLIB1 original_return_bytes=1",
        pid,(unsigned long long)call,bytes,(unsigned long long)fnv);
    emit("SWEET OPNG PACKED pid=%u call=%llu bytes=%lu fnv=%016llx chunks=48",
        pid,(unsigned long long)call,(unsigned long)packed_bytes,(unsigned long long)packed_fnv);
    for(size_t at=0;at<(size_t)packed_bytes;at+=ORIGINAL_PNG_EXPORT_CHUNK) {
        size_t count=(size_t)packed_bytes-at;if(count>ORIGINAL_PNG_EXPORT_CHUNK)count=ORIGINAL_PNG_EXPORT_CHUNK;
        char hex[2*ORIGINAL_PNG_EXPORT_CHUNK+1]={0};
        for(size_t i=0;i<count;++i)snprintf(hex+2*i,3,"%02x",packed[at+i]);
        emit("SWEET OPNG DATA pid=%u call=%llu offset=%zu bytes=%zu hex=%s",
            pid,(unsigned long long)call,at,count,hex);
        (void)usleep(5000);
    }
    bool unchanged=bytes==data.length && original_png_fnv(data.bytes,bytes)==fnv;
    emit("SWEET OPNG END pid=%u call=%llu bytes=%zu fnv=%016llx unchanged=%u original_return_bytes=1",
        pid,(unsigned long long)call,bytes,(unsigned long long)fnv,unchanged);
    free(packed);
    errno=incoming;return unchanged;
}
static void original_png_export_once(NSData *data, unsigned pid, uint64_t call, SweetOriginalPNGEmit emit)
{
    int incoming=errno;bool expected=false;
    static const uint8_t signature[]={137,80,78,71,13,10,26,10};
    if(!data || data.length<sizeof(signature) || data.length>ORIGINAL_PNG_EXPORT_MAX ||
        memcmp(data.bytes,signature,sizeof(signature)) ||
        !atomic_compare_exchange_strong_explicit(&original_png_export_scheduled,&expected,true,memory_order_acq_rel,memory_order_relaxed)) {
        errno=incoming;return;
    }
    NSData *retained=[data retain];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0), ^{
        @autoreleasepool { (void)original_png_export(retained,pid,call,emit); [retained release]; }
    });
    errno=incoming;
}
