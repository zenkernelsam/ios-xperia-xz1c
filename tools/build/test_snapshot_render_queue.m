/* SPDX-License-Identifier: BSD-2-Clause; exact ABI and terminal error forwarding. */
#include "../../gpu/diagnostics/SnapshotRenderQueueObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static unsigned originals;
static bool throwing;
static NSException *original_exception;
@interface QueueFunctionHost : NSObject
- (NSString *)name;
@end
@implementation QueueFunctionHost
- (NSString *)name { return @"author_function"; }
@end
@interface QueueDescriptorHost : NSObject {
@public id _function;
}
- (id)vertexFunction;
- (id)fragmentFunction;
@end
@implementation QueueDescriptorHost
- (id)vertexFunction { return _function; }
- (id)fragmentFunction { return _function; }
@end
@interface SweetMTLRenderPipeline : NSObject {
@public MTLRenderPipelineDescriptor *_descriptor;
}
@end
@implementation SweetMTLRenderPipeline
- (void)dealloc { [_descriptor release]; [super dealloc]; }
@end
@interface SweetMTLRenderEncoder : NSObject {
@public NSDictionary *_vertexBuffers, *_fragmentBuffers;
    SweetMTLRenderPipeline *_pipeline;
}
- (void)drawPrimitives:(MTLPrimitiveType)p vertexStart:(NSUInteger)first vertexCount:(NSUInteger)count instanceCount:(NSUInteger)n;
@end
@implementation SweetMTLRenderEncoder
- (void)drawPrimitives:(MTLPrimitiveType)p vertexStart:(NSUInteger)first vertexCount:(NSUInteger)count instanceCount:(NSUInteger)n
{
    assert(errno == 73 && p == MTLPrimitiveTypeTriangleStrip && first == 0 && count == 4 && n == 1);
    ++originals; errno = 91;
    if (throwing) @throw original_exception;
}
@end
@interface SweetMTLCommandBuffer : NSObject
- (MTLCommandBufferStatus)status;
- (NSError *)error;
@end
@implementation SweetMTLCommandBuffer
- (MTLCommandBufferStatus)status { errno = 120; return MTLCommandBufferStatusError; }
- (NSError *)error { errno = 121; return [NSError errorWithDomain:@"original" code:9 userInfo:@{NSLocalizedDescriptionKey: @"AIR render failed result=-22"}]; }
@end
static void emit(const char *format, ...)
{
    char text[400]; va_list arguments; va_start(arguments, format);
    int n = vsnprintf(text, sizeof(text), format, arguments); va_end(arguments);
    assert(n > 0 && n < 190); puts(text); errno = 117;
}
int main(void) { @autoreleasepool {
    puts("SWEET LOOP LIVE proto=1 token=123456789abcdef0");
    errno = 73;
    assert(snapshot_render_queue_observe([SweetMTLRenderEncoder class], [SweetMTLRenderPipeline class], emit) && errno == 73);
    SweetMTLRenderEncoder *encoder = [SweetMTLRenderEncoder new];
    SweetMTLRenderPipeline *pipeline = [SweetMTLRenderPipeline new];
    QueueFunctionHost *function = [QueueFunctionHost new];
    QueueDescriptorHost *descriptor = [QueueDescriptorHost new]; descriptor->_function = function;
    pipeline->_descriptor = (MTLRenderPipelineDescriptor *)descriptor;
    encoder->_vertexBuffers = @{@0:@{}, @1:@{}};
    encoder->_fragmentBuffers = @{@0:@{}};
    encoder->_pipeline = pipeline;
    SweetMTLCommandBuffer *commands = [SweetMTLCommandBuffer new];
    for (unsigned i = 0; i < 12; ++i) {
        errno = 73; [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:1];
        assert(errno == 91);
        errno = 73; snapshot_render_queue_result(commands); assert(errno == 73);
    }
    throwing = true; original_exception = [NSException exceptionWithName:@"Original" reason:@"preserved" userInfo:nil];
    bool caught = false; errno = 73;
    @try { [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:1]; }
    @catch (id exception) { assert(exception == original_exception); caught = true; }
    assert(caught && errno == 91 && originals == 13);
    unsigned mask = 0; assert(render_queue_mask(@{@0:@{}, @7:@{}}, &mask) && mask == 129);
    assert(!render_queue_mask(@{@8:@{}}, &mask));
    assert(!render_queue_mask(@{@"wrong":@{}}, &mask));
    assert(!render_queue_mask(@[], &mask));
    puts("SWEET LOOP ACK FASTBOOT token=123456789abcdef0");
    render_queue_emit = NULL;
    assert(!snapshot_render_queue_observe([SweetMTLRenderEncoder class], [SweetMTLRenderPipeline class], NULL));
    [commands release]; [encoder release]; [pipeline release]; [function release];
    puts("Snapshot render queue PASS: original four scalar arguments/errno/throws, binding masks, terminal error, eight-call bounds and repeated hook rejection; no GPU work");
} return 0; }
