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
static NSInteger route_mode_value = -1;
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
static double route_scalar(void) { uint64_t bits = UINT64_C(0x7ff8000000001234); double value; memcpy(&value, &bits, 8); return value; }
static CGSize route_size(void) { return (CGSize){route_time, -0.0}; }
@interface PRPosterEnvironmentImpl : NSObject
- (BOOL)isPreview;
- (BOOL)isSnapshot;
- (BOOL)isLowLuminance;
- (double)backlightProgress;
- (double)linearBacklightProgress;
- (double)unlockProgress;
- (CGSize)minimumContentSize;
- (CGSize)desiredContentSize;
@end
@implementation PRPosterEnvironmentImpl
- (BOOL)isPreview { route_original(self, _cmd); return YES; }
- (BOOL)isSnapshot { route_original(self, _cmd); return NO; }
- (BOOL)isLowLuminance { route_original(self, _cmd); return YES; }
- (double)backlightProgress { route_original(self, _cmd); return route_scalar(); }
- (double)linearBacklightProgress { route_original(self, _cmd); return route_scalar(); }
- (double)unlockProgress { route_original(self, _cmd); return route_scalar(); }
- (CGSize)minimumContentSize { route_original(self, _cmd); return route_size(); }
- (CGSize)desiredContentSize { route_original(self, _cmd); return route_size(); }
@end
@interface NUNIAstronomyVistaView : NSObject
- (void)startAnimation;
- (void)stopAnimation;
- (void)_updateAnimation;
- (id)snapshotImage;
@end
@implementation NUNIAstronomyVistaView
- (void)startAnimation { route_original(self, _cmd); }
- (void)stopAnimation { route_original(self, _cmd); }
- (void)_updateAnimation { route_original(self, _cmd); }
- (id)snapshotImage { route_original(self, _cmd); return route_answer; }
@end
@interface _TtC11AegirPoster24AegirRenderingController : NSObject
- (void)renderer:(id)renderer didInitializeWithEnvironment:(id)environment;
- (void)renderer:(id)renderer didUpdateEnvironment:(id)environment withTransition:(id)transition;
@end
@implementation _TtC11AegirPoster24AegirRenderingController
- (void)renderer:(id)renderer didInitializeWithEnvironment:(id)environment
{ assert(renderer == route_completion && environment == route_argument); route_original(self, _cmd); }
- (void)renderer:(id)renderer didUpdateEnvironment:(id)environment withTransition:(id)transition
{ assert(renderer == route_completion && environment == route_argument && transition == route_completion); route_original(self, _cmd); }
@end
@interface PRRenderer : NSObject
- (id)environment;
@end
@implementation PRRenderer
- (id)environment { route_original(self, _cmd); return route_answer; }
@end
@interface NUNIAstronomyVistaController : NSObject
- (void)applyMode:(NSInteger)mode;
- (void)animateTransitionToMode:(NSInteger)mode;
- (void)activeMode;
- (void)deactiveMode;
- (void)interactiveMode;
- (void)supplementalMode;
@end
@implementation NUNIAstronomyVistaController
- (void)applyMode:(NSInteger)mode { assert(!memcmp(&mode, &route_mode_value, sizeof(mode))); route_original(self, _cmd); }
- (void)animateTransitionToMode:(NSInteger)mode { assert(!memcmp(&mode, &route_mode_value, sizeof(mode))); route_original(self, _cmd); }
- (void)activeMode { route_original(self, _cmd); }
- (void)deactiveMode { route_original(self, _cmd); }
- (void)interactiveMode { route_original(self, _cmd); }
- (void)supplementalMode { route_original(self, _cmd); }
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
        [CLKUIQuadViewRenderCoordinator class], [NUNIQuad class], [PRPosterEnvironmentImpl class], [NUNIAstronomyVistaView class],
        [_TtC11AegirPoster24AegirRenderingController class], [PRRenderer class], [NUNIAstronomyVistaController class], route_emit) == QR_API_COUNT);
    assert(errno == 73);
    SweetMTLDevice *device = [SweetMTLDevice new]; CLKUIMetalQuadView *base = [CLKUIMetalQuadView new];
    CLKUIQuadViewRenderCoordinator *coordinator = [CLKUIQuadViewRenderCoordinator new]; NUNIQuad *scene = [NUNIQuad new];
    PRPosterEnvironmentImpl *environment = [PRPosterEnvironmentImpl new]; NUNIAstronomyVistaView *vista = [NUNIAstronomyVistaView new];
    _TtC11AegirPoster24AegirRenderingController *controller = [_TtC11AegirPoster24AegirRenderingController new];
    PRRenderer *renderer = [PRRenderer new];
    NUNIAstronomyVistaController *modeController = [NUNIAstronomyVistaController new];
    const char *selectors[] = { "sharedDevice", "sharedCommandQueue", "newCommandQueue", "addQuad:", "addQuadsFromArray:",
        "didMoveToWindow", "setPaused:", "addDisabledRenderingReason:", "removeDisabledRenderingReason:",
        "_sync_prepareAndRenderForTime:inGroup:checkForDrawable:completion:", "_renderLinkFired:", "setScene:",
        "isPreview", "isSnapshot", "isLowLuminance", "backlightProgress", "linearBacklightProgress", "unlockProgress",
        "minimumContentSize", "desiredContentSize", "startAnimation", "stopAnimation", "_updateAnimation", "snapshotImage",
        "renderer:didInitializeWithEnvironment:", "renderer:didUpdateEnvironment:withTransition:", "environment",
        "applyMode:", "animateTransitionToMode:", "activeMode", "deactiveMode", "interactiveMode", "supplementalMode" };
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
                case QR_SCENE: [scene setScene:route_argument]; break;
                case QR_PREVIEW: assert([environment isPreview]); break;
                case QR_SNAPSHOT: assert(![environment isSnapshot]); break;
                case QR_LOW_LIGHT: assert([environment isLowLuminance]); break;
                case QR_BACKLIGHT: case QR_LINEAR_BACKLIGHT: case QR_UNLOCK: {
                    double scalar = api == QR_BACKLIGHT ? [environment backlightProgress] : api == QR_LINEAR_BACKLIGHT ? [environment linearBacklightProgress] : [environment unlockProgress];
                    double expected = route_scalar(); assert(!memcmp(&scalar, &expected, 8)); break; }
                case QR_MIN_SIZE: case QR_DESIRED_SIZE: {
                    CGSize size = api == QR_MIN_SIZE ? [environment minimumContentSize] : [environment desiredContentSize];
                    CGSize expected = route_size(); assert(!memcmp(&size, &expected, sizeof(size))); break; }
                case QR_START: [vista startAnimation]; break;
                case QR_STOP: [vista stopAnimation]; break;
                case QR_UPDATE: [vista _updateAnimation]; break;
                case QR_SNAPSHOT_IMAGE: answer = [vista snapshotImage]; break;
                case QR_ENV_INIT: [controller renderer:route_completion didInitializeWithEnvironment:route_argument]; break;
                case QR_ENV_UPDATE: [controller renderer:route_completion didUpdateEnvironment:route_argument withTransition:route_completion]; break;
                case QR_RENDERER_ENV: answer = [renderer environment]; break;
                case QR_APPLY_MODE: [modeController applyMode:route_mode_value]; break;
                case QR_TRANSITION_MODE: [modeController animateTransitionToMode:route_mode_value]; break;
                case QR_ACTIVE_MODE: [modeController activeMode]; break;
                case QR_DEACTIVE_MODE: [modeController deactiveMode]; break;
                case QR_INTERACTIVE_MODE: [modeController interactiveMode]; break;
                case QR_SUPPLEMENTAL_MODE: [modeController supplementalMode]; break;
                default: assert(!"Unexpected route API"); break;
                }
            } @catch (id error) { assert(error == route_exception); caught = true; }
            assert(caught == route_throws && errno == 91); if (!route_throws && (api <= QR_DEVICE_QUEUE || api == QR_SNAPSHOT_IMAGE || api == QR_RENDERER_ENV)) assert(answer == route_answer);
        }
        route_answer = saved;
    }
    route_throws = false; route_expected_selector = @selector(_renderLinkFired:);
    for (unsigned i = 0; i < 70; ++i) { errno = 73; [coordinator _renderLinkFired:route_argument]; assert(errno == 91); }
    assert(route_originals == 169 && route_begins == 104 && route_ends == 104 && route_limits == 1);
    const NSInteger modeValues[] = {NSIntegerMin, NSIntegerMax, 0, 1, 3};
    for (unsigned i = 0; i < 5; ++i) {
        route_mode_value = modeValues[i]; route_expected_selector = @selector(applyMode:); errno = 73;
        [modeController applyMode:route_mode_value]; assert(errno == 91);
    }
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
    assert(!quad_route_install([NUNIAstronomyVistaController class], false, "NUNIAstronomyVistaController", "applyMode:", QR_APPLY_MODE,
        (IMP)quad_route_apply_mode, @encode(void), (const char *const []){@encode(NSUInteger)}, 1));
    // Test the production name matcher without trying to allocate a runtime
    // class name already reserved by the existing mangled fixture.
    assert(quad_route_owner_name_matches("_TtC11AegirPoster24AegirRenderingController","_TtC11AegirPoster24AegirRenderingController"));
    assert(quad_route_owner_name_matches("AegirPoster.AegirRenderingController","_TtC11AegirPoster24AegirRenderingController"));
    assert(!quad_route_owner_name_matches("AnotherModule.AegirRenderingController","_TtC11AegirPoster24AegirRenderingController"));
    assert(!quad_route_owner_name_matches(NULL,"_TtC11AegirPoster24AegirRenderingController"));
    [device release]; [base release]; [coordinator release]; [scene release]; [environment release]; [vista release]; [controller release]; [renderer release]; [modeController release]; [route_answer release];
    [route_argument release]; [route_completion release];
    puts("Quad route observer PASS: metaclass/owner/ABI rejection, original nil/throw/errno/BOOL/double/block forwarding, reason getter containment, per-API bounds");
} return 0; }
