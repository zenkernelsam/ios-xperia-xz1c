/* SPDX-License-Identifier: BSD-2-Clause */
#include "../../gpu/diagnostics/PosterSnapshotGateObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static id expected_configuration, expected_view;
static NSException *expected_exception;
static bool throw_request, throw_reply, synchronous_reply;
static unsigned original_calls, original_replies, begin_records, end_records, reply_records, reply_end_records;
static NSInteger expected_orientation;
static CGRect expected_frame, expected_reply;
static SweetPosterGateReply saved_reply;
static id expected_path, expected_image;

static void original(void)
{
    assert(errno == 73); ++original_calls; errno = 91;
    if (throw_request) @throw expected_exception;
}
static void reply_or_save(SweetPosterGateReply completion)
{
    if (!completion) return;
    if (synchronous_reply) { errno = 81; completion(expected_reply); }
    else { [saved_reply release]; saved_reply = [completion copy]; }
}
@interface SBSWallpaperService : NSObject
- (void)fetchLockScreenContentCutoutBoundsForOrientation:(NSInteger)orientation completionHandler:(SweetPosterGateReply)completion;
- (void)fetchMaximalLockScreenContentCutoutBoundsForOrientation:(NSInteger)orientation completionHandler:(SweetPosterGateReply)completion;
@end
@implementation SBSWallpaperService
- (void)fetchLockScreenContentCutoutBoundsForOrientation:(NSInteger)orientation completionHandler:(SweetPosterGateReply)completion
{
    assert(orientation == expected_orientation); original();
    reply_or_save(completion);
}
- (void)fetchMaximalLockScreenContentCutoutBoundsForOrientation:(NSInteger)orientation completionHandler:(SweetPosterGateReply)completion
{ assert(orientation == expected_orientation); original(); reply_or_save(completion); }
@end
@interface PRRenderer : NSObject
- (id)backgroundView;
- (id)floatingView;
@end
@implementation PRRenderer
- (id)backgroundView { original(); return expected_view; }
- (id)floatingView { original(); return expected_view; }
@end
@interface NUNIAstronomyVistaView : NSObject
- (id)initWithFrame:(CGRect)frame configuration:(id)configuration;
@end
@implementation NUNIAstronomyVistaView
- (id)initWithFrame:(CGRect)frame configuration:(id)configuration
{
    assert(!memcmp(&frame, &expected_frame, sizeof(frame)) && configuration == expected_configuration);
    original(); return self;
}
@end
@interface UIImage : NSObject
- (id)initWithContentsOfFile:(id)path;
@end
@implementation UIImage
- (id)initWithContentsOfFile:(id)path { assert(path == expected_path); original(); return expected_image; }
@end
@interface UIImageView : NSObject
- (void)setImage:(id)image;
@end
@implementation UIImageView
- (void)setImage:(id)image { assert(image == expected_image); original(); }
@end
@interface PosterGateWrongOrientation : NSObject
- (void)fetchLockScreenContentCutoutBoundsForOrientation:(uint32_t)orientation completionHandler:(SweetPosterGateReply)completion;
@end
@implementation PosterGateWrongOrientation
- (void)fetchLockScreenContentCutoutBoundsForOrientation:(uint32_t)orientation completionHandler:(SweetPosterGateReply)completion
{ (void)orientation; (void)completion; assert(!"Wrong orientation ABI was invoked"); }
@end
@interface PosterGateInheritedRenderer : PRRenderer @end
@implementation PosterGateInheritedRenderer @end

static void emit(const char *format, ...)
{
    char line[320]; va_list args; va_start(args, format); int n = vsnprintf(line, sizeof(line), format, args); va_end(args);
    assert(n > 0 && n <= 190);
    begin_records += strstr(line, "SWEET PSG BEGIN ") != NULL;
    end_records += strstr(line, "SWEET PSG END ") != NULL;
    reply_records += strstr(line, "SWEET PSG REPLY ") != NULL;
    reply_end_records += strstr(line, "SWEET PSG REPLY_END ") != NULL;
    puts(line); errno = 117;
}

int main(void)
{
    @autoreleasepool {
        assert(poster_snapshot_gate_observe([SBSWallpaperService class], [PRRenderer class], [NUNIAstronomyVistaView class],
            [UIImage class], [UIImageView class], emit) == 7);
        SBSWallpaperService *service = [SBSWallpaperService new];
        PRRenderer *renderer = [PRRenderer new];
        expected_configuration = [NSObject new];
        expected_exception = [NSException exceptionWithName:@"OriginalPosterGate" reason:@"fixture" userInfo:nil];
        expected_frame = CGRectMake(-0.0, 3.5, 360, 640);
        expected_reply = CGRectMake(1.25, -7.5, 359.5, 123.25);
        expected_orientation = (NSInteger)UINT64_C(0x100000001);
        SweetPosterGateReply completion = ^(CGRect rectangle) {
            assert(errno == 81);
            assert(!memcmp(&rectangle, &expected_reply, sizeof(rectangle)));
            ++original_replies; errno = 83;
            if (throw_reply) @throw expected_exception;
        };
        errno = 73;
        [service fetchLockScreenContentCutoutBoundsForOrientation:expected_orientation completionHandler:completion];
        assert(saved_reply && errno == 91);
        errno = 81; saved_reply(expected_reply); assert(errno == 83 && original_replies == 1);
        // Copies of the wrapper survive the method call and can forward each original invocation.
        SweetPosterGateReply retained = [saved_reply copy];
        [saved_reply release]; saved_reply = nil;
        errno = 81; retained(expected_reply); assert(errno == 83 && original_replies == 2);
        throw_reply = true; errno = 81; bool caught = false;
        @try { retained(expected_reply); } @catch (id exception) { assert(exception == expected_exception); caught = true; }
        assert(caught && errno == 83); throw_reply = false; [retained release];
        synchronous_reply = true; errno = 73;
        [service fetchLockScreenContentCutoutBoundsForOrientation:expected_orientation completionHandler:completion];
        assert(errno == 83 && original_replies == 4);
        errno = 73;
        [service fetchMaximalLockScreenContentCutoutBoundsForOrientation:expected_orientation completionHandler:completion];
        assert(errno == 83 && original_replies == 5);
        errno = 73;
        [service fetchLockScreenContentCutoutBoundsForOrientation:expected_orientation completionHandler:nil];
        assert(errno == 91);
        for (unsigned i = 0; i < 2; ++i) {
            expected_view = i ? nil : expected_configuration;
            errno = 73; assert([renderer backgroundView] == expected_view && errno == 91);
            errno = 73; assert([renderer floatingView] == expected_view && errno == 91);
        }
        NUNIAstronomyVistaView *vista = [NUNIAstronomyVistaView alloc];
        errno = 73; assert([vista initWithFrame:expected_frame configuration:expected_configuration] == vista && errno == 91);
        UIImage *image = [UIImage alloc]; UIImageView *image_view = [UIImageView new];
        expected_path = @"/private/var/mobile/Library/Caches/原snapshot.png";
        for (unsigned i = 0; i < 2; ++i) {
            expected_image = i ? nil : expected_configuration;
            errno = 73; assert([image initWithContentsOfFile:expected_path] == expected_image && errno == 91);
            errno = 73; [image_view setImage:expected_image]; assert(errno == 91);
        }
        throw_request = true;
        for (unsigned api = 0; api < 3; ++api) {
            caught = false; errno = 73;
            @try {
                if (api == 0) [service fetchLockScreenContentCutoutBoundsForOrientation:expected_orientation completionHandler:completion];
                if (api == 1) (void)[renderer backgroundView];
                if (api == 2) (void)[vista initWithFrame:expected_frame configuration:expected_configuration];
            } @catch (id exception) { assert(exception == expected_exception); caught = true; }
            assert(caught && errno == 91);
        }
        throw_request = false;
        unsigned calls = original_calls;
        for (unsigned i = 0; i < 12; ++i) { errno = 73; (void)[renderer backgroundView]; assert(errno == 91); }
        assert(original_calls == calls + 12 && atomic_load(&poster_gate_calls[PSG_BACKGROUND]) == 15);
        const char *request[] = {@encode(NSInteger), @encode(SweetPosterGateReply)};
        assert(!poster_gate_install([PosterGateWrongOrientation class], "PosterGateWrongOrientation",
            "fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:", PSG_NORMAL,
            (IMP)poster_gate_cutout, @encode(void), request, 2));
        assert(!poster_gate_install([PosterGateInheritedRenderer class], "PosterGateInheritedRenderer",
            "backgroundView", PSG_BACKGROUND, (IMP)poster_gate_background, @encode(id), NULL, 0));
        assert(!poster_gate_install([SBSWallpaperService class], "SBSWallpaperService",
            "fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:", PSG_NORMAL,
            (IMP)poster_gate_cutout, @encode(void), request, 2));
        assert(begin_records == end_records && reply_records == reply_end_records && reply_records == 5);
        [vista release]; [renderer release]; [service release]; [expected_configuration release];
        [image release]; [image_view release];
        puts("poster snapshot gate PASS: exact ABI/CGRect/path, original nil/throws/errno, deferred copied callbacks, image load/set, bounds and invalid hooks");
    }
    return 0;
}
