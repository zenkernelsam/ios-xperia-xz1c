// SPDX-License-Identifier: BSD-2-Clause
// Native device/resource conventions adapted from ios_sagit QcomMTL.mm.
#import "MetalResources.h"
#include "MetalDeviceLimits16.h"
#include "MetalBufferRules.h"
#include "../sweetgl/MetalBackingRules.h"
#include "../sweetgl/MetalBlitPlan.h"
#include <objc/runtime.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
extern "C" bool sweetgl_init(void);

#if defined(SWEET_METAL_HOST_TEST)
#define SWEET_MTL_BASE NSObject
#else
@interface _MTLDevice : NSObject
- (void)initLimits;
- (void)initFeatureQueries;
@end
#define SWEET_MTL_BASE _MTLDevice
#endif
@class SweetMTLBuffer;
@interface SweetMTLDevice : SWEET_MTL_BASE <MTLDevice> {
@public NSLock *_resourceLock;NSUInteger _allocatedBytes;SweetMTLDeviceLimits16 _sweetLimits;
}
- (instancetype)initBackend;
@end
@interface SweetMTLBuffer : NSObject <MTLBuffer> {
@public SweetMTLDevice *_owner;SweetMetalBacking _backing;MTLResourceOptions _options;NSString *_label;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner length:(NSUInteger)length options:(MTLResourceOptions)options;
@end
@class SweetMTLCommandQueue;
@interface SweetMTLDevice (CommandQueues)
- (id<MTLCommandQueue>)newCommandQueue;
- (id<MTLCommandQueue>)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)count;
@end

static bool native_base_available(void)
{
#if defined(SWEET_METAL_HOST_TEST)
    return true;
#else
    Class base=objc_lookUpClass("_MTLDevice");Protocol *spi=objc_getProtocol("MTLDeviceSPI");
    Ivar ivar=base?class_getInstanceVariable(base,"_limits"):NULL;
    Method initializer=base?class_getInstanceMethod(base,sel_registerName("initWithAcceleratorPort:")):NULL;
    char type[16]={0};if(initializer)method_getArgumentType(initializer,2,type,sizeof(type));
    NSUInteger bytes=0,alignment=0;if(ivar)NSGetSizeAndAlignment(ivar_getTypeEncoding(ivar),&bytes,&alignment);
    auto os=NSProcessInfo.processInfo.operatingSystemVersion;
    if(os.majorVersion!=16 || !spi || !ivar || !initializer || method_getNumberOfArguments(initializer)!=3 ||
        strcmp(type,@encode(uint32_t)) || bytes!=264 || alignment!=8)return false;
    Class device=objc_lookUpClass("SweetMTLDevice");
    if(!device || class_getSuperclass(device)!=base)return false;
    if(!class_conformsToProtocol(device,spi))class_addProtocol(device,spi);
    return class_conformsToProtocol(device,spi);
#endif
}

@implementation SweetMTLDevice
- (instancetype)initBackend
{
    if(!(self=[super init]))return nil;
    if(!native_base_available() || !sweetgl_init()){[self release];return nil;}
    _resourceLock=[NSLock new];[self initLimits];
#if !defined(SWEET_METAL_HOST_TEST)
    [super initFeatureQueries];
#endif
    return self;
}
- (void)dealloc{[_resourceLock release];[super dealloc];}
- (void)initLimits
{
    SweetMTLDeviceLimits16 limits={};
    // Resource support exists first. Shader/pipeline limits remain zero until
    // their actual backend is connected; do not claim Apple family support.
    limits.maxBufferLength=UINT64_C(256)<<20;limits.minBufferNoCopyAlignmentBytes=16384;
    limits.minConstantBufferAlignmentBytes=64;_sweetLimits=limits;
}
- (const SweetMTLDeviceLimits16 *)limits{return &_sweetLimits;}
- (NSUInteger)featureProfile{return NSUIntegerMax;}
- (NSString *)name{return @"Adreno 540 (SweetScatter)";}
- (BOOL)hasUnifiedMemory{return YES;}
- (BOOL)isLowPower{return NO;}
- (BOOL)isHeadless{return NO;}
- (BOOL)isRemovable{return NO;}
- (BOOL)supportsFamily:(MTLGPUFamily)family{(void)family;return NO;}
- (BOOL)supportsFeatureSet:(MTLFeatureSet)set{(void)set;return NO;}
- (NSUInteger)maxBufferLength{return _sweetLimits.maxBufferLength;}
- (NSUInteger)currentAllocatedSize{[_resourceLock lock];NSUInteger n=_allocatedBytes;[_resourceLock unlock];return n;}
- (id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options
{return [[SweetMTLBuffer alloc] initWithOwner:self length:length options:options];}
- (id<MTLBuffer>)newBufferWithBytes:(const void *)bytes length:(NSUInteger)length options:(MTLResourceOptions)options
{
    if(!bytes)return nil;
    id<MTLBuffer> buffer=[self newBufferWithLength:length options:options];
    if(buffer){void *target=buffer.contents;if(!target){[buffer release];return nil;}memcpy(target,bytes,length);}
    return buffer;
}
@end

@implementation SweetMTLBuffer
- (instancetype)initWithOwner:(SweetMTLDevice *)owner length:(NSUInteger)length options:(MTLResourceOptions)options
{
    if(!(self=[super init]))return nil;
    if(!owner || !sweet_metal_shared_options(options) || !sweet_metal_backing_span(length)){[self release];return nil;}
    if(sweetgl_metal_buffer_allocate(length,&_backing)){[self release];return nil;}
    if(!sweet_metal_backing_valid(&_backing) || _backing.logical_bytes!=length){
        (void)sweetgl_metal_buffer_release(_backing.handle);memset(&_backing,0,sizeof(_backing));[self release];return nil;}
    _owner=[owner retain];
    // The current mapper provides cached CPU pages. Treat WriteCombined as
    // an optimization request and report the actual DefaultCache backing.
    _options=options&~MTLResourceCPUCacheModeMask;
    [_owner->_resourceLock lock];_owner->_allocatedBytes+=_backing.allocated_bytes;[_owner->_resourceLock unlock];
    return self;
}
- (void)dealloc
{
    if(_backing.handle){int status=sweetgl_metal_buffer_release(_backing.handle);
        if(!status){[_owner->_resourceLock lock];_owner->_allocatedBytes-=_backing.allocated_bytes;[_owner->_resourceLock unlock];}
        else fprintf(stderr,"SWEET METAL resource release failed handle=%u status=%d\n",_backing.handle,status);}
    [_label release];[_owner release];[super dealloc];
}
- (id<MTLDevice>)device{return _owner;}
- (NSUInteger)length{return _backing.logical_bytes;}
- (NSUInteger)allocatedSize{return _backing.allocated_bytes;}
- (uint64_t)gpuAddress{return _backing.gpu_address;}
- (void *)contents{return sweetgl_metal_buffer_synchronize(_backing.handle)?NULL:_backing.contents;}
- (MTLStorageMode)storageMode{return MTLStorageModeShared;}
- (MTLCPUCacheMode)cpuCacheMode{return (MTLCPUCacheMode)(_options&15);}
- (MTLHazardTrackingMode)hazardTrackingMode{return (_options&MTLResourceHazardTrackingModeMask)?(MTLHazardTrackingMode)((_options&MTLResourceHazardTrackingModeMask)>>MTLResourceHazardTrackingModeShift):MTLHazardTrackingModeTracked;}
- (MTLResourceOptions)resourceOptions{return _options;}
- (id<MTLHeap>)heap{return nil;}
- (NSUInteger)heapOffset{return 0;}
- (BOOL)isAliasable{return NO;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
@end

#include "MetalCommandQueue.inc"

@implementation SweetMTLDevice (CommandQueues)
- (id<MTLCommandQueue>)newCommandQueue{return [[SweetMTLCommandQueue alloc] initWithDevice:self];}
- (id<MTLCommandQueue>)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)count
{return [[SweetMTLCommandQueue alloc] initWithDevice:self maximumBuffers:count];}
@end

id<MTLDevice> SweetMTLCreateDevice(NSError **error)
{
    if(error)*error=nil;
    id<MTLDevice> device=[[SweetMTLDevice alloc] initBackend];
    if(!device && error)*error=[NSError errorWithDomain:MTLCommandBufferErrorDomain code:MTLCommandBufferErrorInvalidResource
        userInfo:@{NSLocalizedDescriptionKey:@"Native iOS16 Metal ABI or SweetScatter backend unavailable"}];
    return device;
}

BOOL SweetMTLCommandResult(id<MTLCommandBuffer> commands,uint32_t *sequence,uint32_t *operations)
{
    if(sequence)*sequence=0;if(operations)*operations=0;
    if(!sequence || !operations || ![commands isKindOfClass:[SweetMTLCommandBuffer class]] ||
        commands.status!=MTLCommandBufferStatusCompleted || commands.error)return NO;
    auto buffer=(SweetMTLCommandBuffer *)commands;
    *sequence=[buffer sweetCompletedSequence];*operations=(uint32_t)[buffer sweetGPUOperations];
    return *sequence && *operations;
}
