// SPDX-License-Identifier: BSD-2-Clause
// Included in MetalResources.mm to share the real resource owner checks.
#include "MetalQueueState.h"
#include <dispatch/dispatch.h>
@class SweetMTLCommandBuffer;
@interface SweetMTLCommandQueue : NSObject <MTLCommandQueue> {
@public SweetMTLDevice *_device;dispatch_queue_t _execution;NSString *_label;
    NSCondition *_capacity;NSLock *_reservation;NSUInteger _maximumBuffers,_activeBuffers;
}
- (instancetype)initWithDevice:(SweetMTLDevice *)device;
- (instancetype)initWithDevice:(SweetMTLDevice *)device maximumBuffers:(NSUInteger)maximum;
- (void)reserveBuffer;
- (void)retireBuffer;
@end
@interface SweetMTLCommandBuffer : NSObject <MTLCommandBuffer> {
@public NSCondition *_condition;SweetMTLCommandQueue *_queue;SweetMetal::QueueState _state;
    NSMutableArray *_operations,*_scheduledHandlers,*_completedHandlers;NSError *_error;NSString *_label;
    uint32_t _sequence;NSUInteger _GPUOperations;BOOL _reserved,_capacityReleased;
}
- (instancetype)initWithQueue:(SweetMTLCommandQueue *)queue;
- (BOOL)appendOperation:(NSDictionary *)operation;
- (void)finishEncoder;
- (void)executeOperations;
@end
@interface SweetMTLBlitEncoder : NSObject <MTLBlitCommandEncoder> {
    SweetMTLCommandBuffer *_commands;BOOL _ended;NSString *_label;
}
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands;
@end
static NSError *metal_queue_error(NSString *message,NSInteger code=MTLCommandBufferErrorInvalidResource)
{return [NSError errorWithDomain:MTLCommandBufferErrorDomain code:code userInfo:@{NSLocalizedDescriptionKey:message}];}

@implementation SweetMTLCommandQueue
- (instancetype)initWithDevice:(SweetMTLDevice *)device
{return [self initWithDevice:device maximumBuffers:64];}
- (instancetype)initWithDevice:(SweetMTLDevice *)device maximumBuffers:(NSUInteger)maximum
{
    if(!(self=[super init]))return nil;if(!maximum || maximum>4096){[self release];return nil;}
    _maximumBuffers=maximum;_device=[device retain];_capacity=[NSCondition new];_reservation=[NSLock new];
    _execution=dispatch_queue_create("lilac.metal.commands",DISPATCH_QUEUE_SERIAL);if(!_execution){[self release];return nil;}return self;
}
- (void)dealloc{if(_execution)dispatch_release(_execution);[_label release];[_capacity release];[_reservation release];[_device release];[super dealloc];}
- (void)reserveBuffer{[_capacity lock];while(_activeBuffers>=_maximumBuffers)[_capacity wait];++_activeBuffers;[_capacity unlock];}
- (void)retireBuffer{[_capacity lock];if(_activeBuffers)--_activeBuffers;[_capacity signal];[_capacity unlock];}
- (id<MTLDevice>)device{return _device;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *value=[label copy];[_label release];_label=value;}
- (id<MTLCommandBuffer>)commandBuffer{return [[[SweetMTLCommandBuffer alloc] initWithQueue:self] autorelease];}
- (id<MTLCommandBuffer>)commandBufferWithUnretainedReferences
{
    // Strong backing references are permitted even when callers offer the
    // unretained optimization. Report the actual lifetime policy.
    return [self commandBuffer];
}
@end

@implementation SweetMTLCommandBuffer
- (instancetype)initWithQueue:(SweetMTLCommandQueue *)queue
{
    if(!(self=[super init]))return nil;_queue=[queue retain];[_queue reserveBuffer];_condition=[NSCondition new];
    _operations=[NSMutableArray new];_scheduledHandlers=[NSMutableArray new];_completedHandlers=[NSMutableArray new];return self;
}
- (void)dealloc{if(!_capacityReleased)[_queue retireBuffer];[_condition release];[_queue release];[_operations release];[_scheduledHandlers release];[_completedHandlers release];[_error release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _queue.device;}
- (id<MTLCommandQueue>)commandQueue{return _queue;}
- (BOOL)retainedReferences{return YES;}
- (MTLCommandBufferStatus)status{[_condition lock];auto status=(MTLCommandBufferStatus)_state.status;[_condition unlock];return status;}
- (NSError *)error{[_condition lock];NSError *value=[[_error retain] autorelease];[_condition unlock];return value;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (id<MTLBlitCommandEncoder>)blitCommandEncoder
{
    [_condition lock];bool allowed=_state.beginEncoder();[_condition unlock];
    return allowed?[[[SweetMTLBlitEncoder alloc] initWithCommands:self] autorelease]:nil;
}
- (BOOL)appendOperation:(NSDictionary *)operation
{
    [_condition lock];bool allowed=_state.status==SweetMetal::QueueStatus::Created && _state.encoderOpen && !_state.invalid;
    if(allowed && _operations.count<4096)[_operations addObject:operation];
    else{_state.invalid=true;allowed=false;}
    [_condition unlock];return allowed;
}
- (void)finishEncoder{[_condition lock];if(!_state.endEncoder())_state.invalid=true;[_condition unlock];}
- (void)enqueue
{
    [_queue->_reservation lock];[_condition lock];bool accepted=_state.enqueue();
    if(accepted)_reserved=YES;[_condition unlock];
    if(accepted)dispatch_async(_queue->_execution,^{@autoreleasepool{[self executeOperations];}});
    [_queue->_reservation unlock];
    if(!accepted)[NSException raise:NSInternalInconsistencyException format:@"Metal command buffer cannot enqueue in its current state"];
}
- (void)commit
{
    [_queue->_reservation lock];[_condition lock];bool accepted=_state.commit(),dispatch=accepted && !_reserved;
    if(accepted){_reserved=YES;[_condition broadcast];}[_condition unlock];
    if(dispatch)dispatch_async(_queue->_execution,^{@autoreleasepool{[self executeOperations];}});
    [_queue->_reservation unlock];
    if(!accepted)[NSException raise:NSInternalInconsistencyException format:@"Metal command buffer cannot commit an open/invalid encoder or repeat commit"];
}
- (void)executeOperations
{
    [_condition lock];while(_state.status==SweetMetal::QueueStatus::Enqueued)[_condition wait];bool scheduled=_state.schedule();
    NSArray *handlers=[_scheduledHandlers copy];[_scheduledHandlers removeAllObjects];[_condition broadcast];[_condition unlock];
    if(!scheduled){[handlers release];return;}
    for(id handler in handlers)((MTLCommandBufferHandler)handler)(self);[handlers release];
    NSError *failure=nil;
    for(NSDictionary *operation in _operations) {
        SweetMTLBuffer *dst=operation[@"destination"],*src=operation[@"source"];
        NSUInteger bytes=[operation[@"bytes"] unsignedLongLongValue],destination=[operation[@"destinationOffset"] unsignedLongLongValue];
        NSUInteger source=[operation[@"sourceOffset"] unsignedLongLongValue],fill=[operation[@"fill"] unsignedIntValue];
        if(![dst isKindOfClass:[SweetMTLBuffer class]] || dst.device!=self.device ||
            (src && (![src isKindOfClass:[SweetMTLBuffer class]] || src.device!=self.device))){failure=metal_queue_error(@"Blit resources have another owner");break;}
        uint32_t sequence=0;
        int result=sweetgl_metal_buffer_blit(src?src->_backing.handle:0,dst->_backing.handle,source,destination,bytes,(uint32_t)fill,&sequence);
        if(result){failure=metal_queue_error([NSString stringWithFormat:@"A540 blit failed result=%d sequence=%u",result,sequence],result==-ETIMEDOUT?MTLCommandBufferErrorTimeout:MTLCommandBufferErrorInvalidResource);break;}
        if(!sequence){failure=metal_queue_error(@"A540 blit completed without a hardware fence");break;}
        [_condition lock];_sequence=sequence;++_GPUOperations;[_condition unlock];
    }
    // Retire retained backing after every issued fence and before publishing
    // terminal status to a concurrent waitUntilCompleted caller.
    [_operations removeAllObjects];
    [_queue retireBuffer];_capacityReleased=YES;
    [_condition lock];_error=[failure retain];bool finished=_state.finish(failure==nil);
    handlers=[_completedHandlers copy];[_completedHandlers removeAllObjects];[_condition broadcast];[_condition unlock];
    if(finished)for(id handler in handlers)((MTLCommandBufferHandler)handler)(self);[handlers release];
}
- (void)waitUntilScheduled
{
    [_condition lock];bool committed=(uint32_t)_state.status>=(uint32_t)SweetMetal::QueueStatus::Committed;
    if(committed)while((uint32_t)_state.status<(uint32_t)SweetMetal::QueueStatus::Scheduled)[_condition wait];[_condition unlock];
    if(!committed)[NSException raise:NSInternalInconsistencyException format:@"Metal wait requires a committed command buffer"];
}
- (void)waitUntilCompleted
{
    [_condition lock];bool committed=(uint32_t)_state.status>=(uint32_t)SweetMetal::QueueStatus::Committed;
    if(committed)while(_state.status!=SweetMetal::QueueStatus::Completed && _state.status!=SweetMetal::QueueStatus::Error)[_condition wait];[_condition unlock];
    if(!committed)[NSException raise:NSInternalInconsistencyException format:@"Metal wait requires a committed command buffer"];
}
- (void)addScheduledHandler:(MTLCommandBufferHandler)handler
{
    if(!handler)return;[_condition lock];bool ready=(uint32_t)_state.status>=(uint32_t)SweetMetal::QueueStatus::Scheduled;
    if(!ready)[_scheduledHandlers addObject:[[handler copy] autorelease]];[_condition unlock];if(ready)handler(self);
}
- (void)addCompletedHandler:(MTLCommandBufferHandler)handler
{
    if(!handler)return;[_condition lock];bool ready=_state.status==SweetMetal::QueueStatus::Completed || _state.status==SweetMetal::QueueStatus::Error;
    if(!ready)[_completedHandlers addObject:[[handler copy] autorelease]];[_condition unlock];if(ready)handler(self);
}
- (uint32_t)sweetCompletedSequence{[_condition lock];uint32_t value=_sequence;[_condition unlock];return value;}
- (NSUInteger)sweetGPUOperations{[_condition lock];NSUInteger value=_GPUOperations;[_condition unlock];return value;}
@end

@implementation SweetMTLBlitEncoder
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands{if(!(self=[super init]))return nil;_commands=[commands retain];return self;}
- (void)dealloc{[_commands release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _commands.device;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)endEncoding
{if(_ended)[NSException raise:NSInternalInconsistencyException format:@"Metal encoder already ended"];_ended=YES;[_commands finishEncoder];}
- (void)copyFromBuffer:(id<MTLBuffer>)source sourceOffset:(NSUInteger)sourceOffset toBuffer:(id<MTLBuffer>)destination destinationOffset:(NSUInteger)destinationOffset size:(NSUInteger)size
{
    bool valid=!_ended && [source isKindOfClass:[SweetMTLBuffer class]] && [destination isKindOfClass:[SweetMTLBuffer class]] &&
        source.device==self.device && destination.device==self.device && smb_range(source.length,sourceOffset,size) && smb_range(destination.length,destinationOffset,size);
    if(source==destination && valid){NSUInteger delta=sourceOffset>destinationOffset?sourceOffset-destinationOffset:destinationOffset-sourceOffset;if(delta<size)valid=false;}
    @autoreleasepool {
        if(!valid || ![_commands appendOperation:@{@"source":source,@"destination":destination,@"sourceOffset":@(sourceOffset),@"destinationOffset":@(destinationOffset),@"bytes":@(size),@"fill":@0}])
            [NSException raise:NSInvalidArgumentException format:@"Invalid Metal buffer copy range/owner/alignment"];
    }
}
- (void)fillBuffer:(id<MTLBuffer>)buffer range:(NSRange)range value:(uint8_t)value
{
    @autoreleasepool {
        if(_ended || ![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=self.device || !smb_range(buffer.length,range.location,range.length) ||
            ![_commands appendOperation:@{@"destination":buffer,@"destinationOffset":@(range.location),@"sourceOffset":@0,@"bytes":@(range.length),@"fill":@(value)}])
            [NSException raise:NSInvalidArgumentException format:@"Invalid Metal fill range/owner/alignment"];
    }
}
@end
