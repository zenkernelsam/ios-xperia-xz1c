/* Original class/instance dispatch, exact BOOL/double/block ABI and per-API bounds. */
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/QuadRouteObserve.h"
#include <assert.h>
#include <stdarg.h>
static id route_argument, route_answer, route_completion;
static NSException *route_exception;
static bool route_throws;
static unsigned route_originals, route_begins, route_ends, route_limits;
static double route_time = 0x1.23456789abcdep+17;
static SEL route_expected_selector;
static void route_original(id receiver, SEL selector)
{
    assert(receiver && selector == route_expected_selector && errno == 73);
    ++route_originals; errno = 91; if (route_throws) @throw route_exception;
}
@interface CLKUIMetalResourceManager : NSObject
+ (id)sharedDevice;
+ (id)sharedCommandQueue;
@end
@implementation CLKUIMetalResourceManager
+ (id)sharedDevice { assert(self == [CLKUIMetalResourceManager class]); route_original(self, _cmd); return route_answer; }
+ (id)sharedCommandQueue { assert(self == [CLKUIMetalResourceManager class]); route_original(self, _cmd); return route_answer; }
@end
@interface SweetMTLDevice : NSObject
- (id)newCommandQueue;
@end
@implementation SweetMTLDevice
- (id)newCommandQueue { route_original(self, _cmd); return route_answer; }
@end
@interface CLKUIQuadView : NSObject
- (void)addQuad:(id)value;
- (void)addQuadsFromArray:(id)value;
- (void)didMoveToWindow;
- (void)setPaused:(BOOL)value;
- (void)addDisabledRenderingReason:(id)value;
- (void)removeDisabledRenderingReason:(id)value;
- (BOOL)_sync_prepareAndRenderForTime:(double)time inGroup:(id)group checkForDrawable:(BOOL)check completion:(void (^)(void))completion;
@end
@implementation CLKUIQuadView
- (void)addQuad:(id)value { assert(value == route_argument); route_original(self, _cmd); }
- (void)addQuadsFromArray:(id)value { assert(value == route_argument); route_original(self, _cmd); }
- (void)didMoveToWindow { route_original(self, _cmd); }
- (void)setPaused:(BOOL)value { assert(value == YES); route_original(self, _cmd); }
- (void)addDisabledRenderingReason:(id)value { assert(value == route_argument); route_original(self, _cmd); }
- (void)removeDisabledRenderingReason:(id)value { assert(value == route_argument); route_original(self, _cmd); }
- (BOOL)_sync_prepareAndRenderForTime:(double)time inGroup:(id)group checkForDrawable:(BOOL)check completion:(void (^)(void))completion
{ assert(!memcmp(&time, &route_time, sizeof(time)) && group == route_argument && check == YES && (id)completion == route_completion); route_original(self, _cmd); return YES; }
@end
@interface CLKUIMetalQuadView : CLKUIQuadView @end
@implementation CLKUIMetalQuadView @end
@interface CLKUIQuadViewRenderCoordinator : NSObject
- (void)_renderLinkFired:(id)value;
@end
@implementation CLKUIQuadViewRenderCoordinator
- (void)_renderLinkFired:(id)value { assert(value == route_argument); route_original(self, _cmd); }
@end
@interface NUNIQuad : NSObject
- (void)setScene:(id)value;
@end
@implementation NUNIQuad
- (void)setScene:(id)value { assert(value == route_argument); route_original(self, _cmd); }
@end
@interface RouteThrowingReason : NSString @end
@implementation RouteThrowingReason
- (NSUInteger)length { return 1; }
- (unichar)characterAtIndex:(NSUInteger)index { assert(!index); return 'x'; }
- (const char *)UTF8String { errno = 123; @throw route_exception; }
@end
static void route_emit(const char *format, ...)
{
    char line[512]; va_list args; va_start(args, format); int count = vsnprintf(line, sizeof(line), format, args); va_end(args);
    assert(count > 0 && count < 190);
    if (strstr(line, "SWEET QUAD ROUTE BEGIN ")) ++route_begins;
    if (strstr(line, "SWEET QUAD ROUTE END ")) ++route_ends;
    if (strstr(line, "SWEET QUAD ROUTE LIMIT ")) ++route_limits;
    errno = 117;
}
int main(void) { @autoreleasepool {
    route_answer = [NSObject new]; route_argument = [NSObject new]; route_completion = [NSObject new];
    route_exception = [NSException exceptionWithName:@"OriginalRoute" reason:@"same original exception" userInfo:nil];
    errno = 73;
    assert(quad_route_observe([SweetMTLDevice class], [CLKUIMetalResourceManager class], [CLKUIQuadView class],
        [CLKUIQuadViewRenderCoordinator class], [NUNIQuad class], route_emit) == QR_API_COUNT);
    assert(errno == 73);
    SweetMTLDevice *device = [SweetMTLDevice new]; CLKUIMetalQuadView *base = [CLKUIMetalQuadView new];
    CLKUIQuadViewRenderCoordinator *coordinator = [CLKUIQuadViewRenderCoordinator new]; NUNIQuad *scene = [NUNIQuad new];
    const char *selectors[] = { "sharedDevice", "sharedCommandQueue", "newCommandQueue", "addQuad:", "addQuadsFromArray:",
        "didMoveToWindow", "setPaused:", "addDisabledRenderingReason:", "removeDisabledRenderingReason:",
        "_sync_prepareAndRenderForTime:inGroup:checkForDrawable:completion:", "_renderLinkFired:", "setScene:" };
    for (unsigned pass = 0; pass < 3; ++pass) {
        id saved = route_answer; if (pass == 1) route_answer = nil; route_throws = pass == 2;
        for (unsigned api = 0; api < QR_API_COUNT; ++api) {
            route_expected_selector = sel_registerName(selectors[api]); errno = 73; id answer = nil; bool caught = false;
            @try {
                switch (api) {
                case QR_MANAGER_DEVICE: answer = [CLKUIMetalResourceManager sharedDevice]; break;
                case QR_MANAGER_QUEUE: answer = [CLKUIMetalResourceManager sharedCommandQueue]; break;
                case QR_DEVICE_QUEUE: answer = [device newCommandQueue]; break;
                case QR_ADD_QUAD: [base addQuad:route_argument]; break;
                case QR_ADD_QUADS: [base addQuadsFromArray:route_argument]; break;
                case QR_WINDOW: [base didMoveToWindow]; break;
                case QR_PAUSED: [base setPaused:YES]; break;
                case QR_DISABLE: [base addDisabledRenderingReason:route_argument]; break;
                case QR_ENABLE: [base removeDisabledRenderingReason:route_argument]; break;
                case QR_SYNC: assert([base _sync_prepareAndRenderForTime:route_time inGroup:route_argument checkForDrawable:YES completion:(id)route_completion]); break;
                case QR_TICK: [coordinator _renderLinkFired:route_argument]; break;
                default: [scene setScene:route_argument]; break;
                }
            } @catch (id error) { assert(error == route_exception); caught = true; }
            assert(caught == route_throws && errno == 91); if (!route_throws && api <= QR_DEVICE_QUEUE) assert(answer == route_answer);
        }
        route_answer = saved;
    }
    route_throws = false; route_expected_selector = @selector(_renderLinkFired:);
    for (unsigned i = 0; i < 70; ++i) { errno = 73; [coordinator _renderLinkFired:route_argument]; assert(errno == 91); }
    assert(route_originals == 106 && route_begins == 41 && route_ends == 41 && route_limits == 1);
    id previous = route_argument; route_argument = @"paused / not attached"; route_expected_selector = @selector(addDisabledRenderingReason:);
    errno = 73; [base addDisabledRenderingReason:route_argument]; assert(errno == 91);
    route_argument = [RouteThrowingReason new]; errno = 73; [base addDisabledRenderingReason:route_argument]; assert(errno == 91);
    [route_argument release]; route_argument = previous;
    assert(!quad_route_install([CLKUIMetalResourceManager class], true, "CLKUIMetalResourceManager", "sharedDevice",
        QR_MANAGER_DEVICE, (IMP)quad_route_manager_device, @encode(id), NULL, 0));
    assert(!quad_route_install([CLKUIMetalQuadView class], false, "CLKUIMetalQuadView", "addQuad:", QR_ADD_QUAD,
        (IMP)quad_route_add, @encode(void), (const char *const []){@encode(id)}, 1));
    assert(!quad_route_install([SweetMTLDevice class], false, "WrongOwner", "newCommandQueue", QR_DEVICE_QUEUE,
        (IMP)quad_route_device_queue, @encode(id), NULL, 0));
    assert(!quad_route_install([SweetMTLDevice class], false, "SweetMTLDevice", "newCommandQueue", QR_DEVICE_QUEUE,
        (IMP)quad_route_device_queue, @encode(id), NULL, 0));
    assert(!quad_route_install([CLKUIQuadView class], false, "CLKUIQuadView", "setPaused:", QR_PAUSED,
        (IMP)quad_route_paused, @encode(void), (const char *const []){@encode(NSUInteger)}, 1));
    [device release]; [base release]; [coordinator release]; [scene release]; [route_answer release];
    [route_argument release]; [route_completion release];
    puts("Quad route observer PASS: metaclass/owner/ABI rejection, original nil/throw/errno/BOOL/double/block forwarding, reason getter containment, per-API bounds");
} return 0; }
