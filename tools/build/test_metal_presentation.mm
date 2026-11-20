/* Real native queue/IOSurface objects; drawable is an explicit host fixture.
 * No host fixture callback proves QuartzCore display completion on a phone. */
#define main original_render_fixture_main
#include "test_metal_render.mm"
#undef main
#include <cmath>
#include <vector>
@interface MTLTextureDescriptor (PresentationHostFlags)
- (void)setIsDrawable:(BOOL)value;
- (void)setFramebufferOnly:(BOOL)value;
- (void)setRotation:(NSUInteger)value;
- (void)setProtectionOptions:(NSUInteger)value;
@end
static unsigned hostPresentCalls,hostDeallocations;
static bool hostPresentThrow,hostWrongTexture;
static std::vector<unsigned> hostModes;
static std::vector<double> hostTimes;
@interface PresentationHostDrawable : NSObject <MTLDrawable> {
@public id<MTLTexture> _texture;unsigned _clearBefore;uint64_t _sequenceBefore;NSDictionary *_expectedOptions;
}
- (instancetype)initWithTexture:(id<MTLTexture>)texture;
- (void)presentWithOptions:(NSDictionary *)options;
@end
@implementation PresentationHostDrawable
- (instancetype)initWithTexture:(id<MTLTexture>)texture{if(!(self=[super init]))return nil;_texture=[texture retain];_clearBefore=clears;_sequenceBefore=sequence;return self;}
- (void)dealloc{++hostDeallocations;[_texture release];[_expectedOptions release];[super dealloc];}
- (id<MTLTexture>)texture{return hostWrongTexture?nil:_texture;}
- (void)checkPresent:(unsigned)mode time:(double)time options:(NSDictionary *)options
{assert(clears>_clearBefore && sequence>_sequenceBefore && _texture && _texture.iosurface && backing.count([(SweetMTLTexture *)_texture sweetBacking].handle));
    assert(!_expectedOptions || [_expectedOptions isEqual:options]);++hostPresentCalls;hostModes.push_back(mode);hostTimes.push_back(time);
    if(hostPresentThrow)[NSException raise:NSInternalInconsistencyException format:@"Original host drawable present failure"];
}
- (void)present{[self checkPresent:0 time:0 options:nil];}
- (void)presentAtTime:(CFTimeInterval)time{[self checkPresent:1 time:time options:nil];}
- (void)presentAfterMinimumDuration:(CFTimeInterval)time{[self checkPresent:2 time:time options:nil];}
- (void)presentWithOptions:(NSDictionary *)options{[self checkPresent:3 time:0 options:options];}
- (CFTimeInterval)presentedTime{return 0;}
- (NSUInteger)drawableID{return (NSUInteger)(uintptr_t)self;}
- (void)addPresentedHandler:(MTLDrawablePresentedHandler)handler{(void)handler;assert(!"Adapter must not manufacture a presented event");}
@end
@interface PresentationWrongABI : NSObject <MTLDrawable>
- (id)texture;
- (void)presentAtTime:(unsigned)time;
@end
@implementation PresentationWrongABI
- (id)texture{return nil;}
- (void)presentAtTime:(unsigned)time{(void)time;assert(!"Wrong ABI must not be called");}
@end
static IOSurfaceRef makeHostSurface(void)
{NSDictionary *properties=@{(id)kIOSurfaceWidth:@17,(id)kIOSurfaceHeight:@9,(id)kIOSurfaceBytesPerElement:@4,(id)kIOSurfaceBytesPerRow:@128,(id)kIOSurfaceAllocSize:@1152,(id)kIOSurfacePixelFormat:@0x42475241};
    return IOSurfaceCreate((CFDictionaryRef)properties);}
static SweetMTLTexture *drawableTexture(id<MTLDevice> device,IOSurfaceRef surface)
{auto d=descriptor(17,9);d.usage=MTLTextureUsageRenderTarget;[d setIsDrawable:YES];[d setFramebufferOnly:YES];
    auto texture=(SweetMTLTexture *)[device newTextureWithDescriptor:d iosurface:surface plane:0];[d release];return texture;}
static void encodeHostClear(id<MTLCommandBuffer> commands,id<MTLTexture> texture)
{auto pass=[MTLRenderPassDescriptor new];pass.colorAttachments[0].texture=texture;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    auto encoder=[commands renderCommandEncoderWithDescriptor:pass];assert(encoder);[encoder endEncoding];[pass release];}
int main(void){@autoreleasepool{
    NSError *error=nil;auto device=SweetMTLCreateDevice(&error);assert(device);auto queue=[device newCommandQueue];
    for(unsigned mode=0;mode<4;++mode){@autoreleasepool{
        auto surface=makeHostSurface();assert(surface);auto texture=drawableTexture(device,surface);assert(texture && texture.isFramebufferOnly && texture->_drawable && texture.usage==MTLTextureUsageRenderTarget);
        unsigned char bytes[80]={0};unsigned char *readback=bytes;exception(^{[texture getBytes:readback bytesPerRow:68 fromRegion:MTLRegionMake2D(0,0,17,1) mipmapLevel:0];});
        auto drawable=[[PresentationHostDrawable alloc] initWithTexture:texture];auto commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];
        encodeHostClear(commands,texture);unsigned before=hostPresentCalls;NSMutableDictionary *options=[@{@"mode":@0,@"tag":@7} mutableCopy];
        unsigned oldClears=clears;uint32_t oldSequence=sequence;
        unsigned oldSeed=IOSurfaceGetSeed(surface);
        [commands addScheduledHandler:^(id<MTLCommandBuffer> c){assert(c.status>=MTLCommandBufferStatusScheduled && clears>oldClears && sequence>oldSequence && IOSurfaceGetSeed(surface)!=oldSeed);}];
        if(mode==0){[commands enqueue];[commands presentDrawable:drawable];}else if(mode==1)[commands presentDrawable:drawable atTime:17.125];else if(mode==2)[commands presentDrawable:drawable afterMinimumDuration:0.0625];
        else{drawable->_expectedOptions=[options copy];[commands presentDrawable:drawable options:options];options[@"tag"]=@8;}
        exception(^{[commands presentDrawable:drawable];});assert(hostPresentCalls==before && commands->_presentations.count==1);[options release];
        [drawable release];[texture release];CFRelease(surface);[commands commit];[commands waitUntilCompleted];
        assert(commands.status==MTLCommandBufferStatusCompleted && !commands.error && commands.sweetGPUOperations==1 && hostPresentCalls==before+1);
        assert(commands->_presentations.count==0 && !device.currentAllocatedSize && backing.empty());[commands release];
    }}
    assert(hostModes==std::vector<unsigned>({0,1,2,3}) && hostTimes==std::vector<double>({0,17.125,0.0625,0}));
    @autoreleasepool{auto surface=makeHostSurface();auto texture=drawableTexture(device,surface);auto drawable=[[PresentationHostDrawable alloc] initWithTexture:texture];
        auto commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];encodeHostClear(commands,texture);
        unsigned before=hostPresentCalls,old=clears,seed=IOSurfaceGetSeed(surface);[commands commit];[commands waitUntilScheduled];assert(clears>old && IOSurfaceGetSeed(surface)!=seed);
        [drawable present];[commands waitUntilCompleted];assert(hostPresentCalls==before+1 && commands.status==MTLCommandBufferStatusCompleted);
        [commands release];[drawable release];[texture release];CFRelease(surface);}
    auto surface=makeHostSurface();auto texture=drawableTexture(device,surface);auto drawable=[[PresentationHostDrawable alloc] initWithTexture:texture];
    auto commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];
    exception(^{[commands presentDrawable:drawable atTime:NAN];});exception(^{[commands presentDrawable:drawable afterMinimumDuration:-1];});
    auto wrong=[PresentationWrongABI new];exception(^{[commands presentDrawable:wrong atTime:1];});[wrong release];
    assert(commands->_presentations.count==0);[commands release];
    commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];auto failedPass=[MTLRenderPassDescriptor new];failedPass.colorAttachments[0].texture=texture;
    failedPass.colorAttachments[0].loadAction=MTLLoadActionLoad;failedPass.colorAttachments[0].storeAction=MTLStoreActionStore;
    auto badEncoder=[commands renderCommandEncoderWithDescriptor:failedPass];assert(badEncoder);[badEncoder endEncoding];[failedPass release];
    // Force an actual invalid retained draw ABI; no driver operation or native
    // presentation may run, even after command state reaches Scheduled.
    commands->_state.beginEncoder();assert([commands appendOperation:@{@"renderPipeline":@1}]);[commands finishEncoder];[commands presentDrawable:drawable];
    unsigned noPresent=hostPresentCalls;[commands commit];[commands waitUntilCompleted];
    assert(commands.status==MTLCommandBufferStatusError && commands.sweetGPUOperations==0 && hostPresentCalls==noPresent && !commands->_presentations.count);[commands release];
    // Native descriptor flags are immutable snapshots and do not turn every
    // IOSurface into a drawable; rotated/protected/swizzled requests reject.
    auto d=descriptor(17,9);[d setIsDrawable:YES];[d setFramebufferOnly:YES];[d setRotation:1];assert(![device newTextureWithDescriptor:d iosurface:surface plane:0]);[d setRotation:0];
    [d setProtectionOptions:1];assert(![device newTextureWithDescriptor:d iosurface:surface plane:0]);[d setProtectionOptions:0];
    d.swizzle=MTLTextureSwizzleChannelsMake(MTLTextureSwizzleBlue,MTLTextureSwizzleGreen,MTLTextureSwizzleRed,MTLTextureSwizzleAlpha);assert(![device newTextureWithDescriptor:d iosurface:surface plane:0]);
    d.swizzle=MTLTextureSwizzleChannelsDefault;assert(![device newTextureWithDescriptor:d]);[d setIsDrawable:NO];[d setFramebufferOnly:NO];[d release];
    unsigned before=hostPresentCalls;
    commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];encodeHostClear(commands,texture);[commands presentDrawable:drawable];hostPresentThrow=true;[commands commit];[commands waitUntilCompleted];hostPresentThrow=false;
    assert(commands.status==MTLCommandBufferStatusError && commands.error && hostPresentCalls==before+1 && commands->_presentations.count==0);[commands release];
    commands=(SweetMTLCommandBuffer *)[[queue commandBuffer] retain];encodeHostClear(commands,texture);[commands presentDrawable:drawable];hostWrongTexture=true;[commands commit];[commands waitUntilCompleted];hostWrongTexture=false;
    assert(commands.status==MTLCommandBufferStatusError && commands.error && hostPresentCalls==before+1 && commands->_presentations.count==0);[commands release];
    [drawable release];[texture release];CFRelease(surface);[queue release];assert(!device.currentAllocatedSize && backing.empty());[device release];
    puts("Native presentation objects PASS: original IOSurface texture flags, real queue ordering, exact times/options, caller release, duplicate/ABI rejection, native exception/change failure retirement; host drawable fixture only");
}return 0;}
