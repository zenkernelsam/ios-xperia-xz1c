/* Original argument/return/error/exception semantics; no device or GPU work. */
#define LILAC_POSTDRAW_TEST 1
#include "../../gpu/diagnostics/SnapshotMethodObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static id answer, expected_argument;
static NSError *answer_error;
static NSError **expected_error;
static NSException *answer_exception;
static bool should_throw;
static unsigned original_calls, begin_records, end_records;
static CGRect expected_rectangle;
static MTLRegion expected_region;
static void *expected_bytes;
static NSUInteger expected_row, expected_image, expected_level, expected_slice;

NSData *UIImagePNGRepresentation(id image) { (void)image; return nil; }
static void original_begin(void)
{ assert(errno == 73); ++original_calls; errno = 91; if (should_throw) @throw answer_exception; }
@interface SnapshotDeviceHost : NSObject
- (id)newRenderPipelineStateWithDescriptor:(id)argument error:(NSError **)error;
- (id)newLibraryWithURL:(id)argument error:(NSError **)error;
- (id)newDefaultLibraryWithBundle:(id)argument error:(NSError **)error;
@end
@implementation SnapshotDeviceHost
- (id)newRenderPipelineStateWithDescriptor:(id)argument error:(NSError **)error
{ assert(argument == expected_argument && error == expected_error); original_begin(); if(error)*error=answer_error; return answer; }
- (id)newLibraryWithURL:(id)argument error:(NSError **)error
{ assert(argument == expected_argument && error == expected_error); original_begin(); if(error)*error=answer_error; return answer; }
- (id)newDefaultLibraryWithBundle:(id)argument error:(NSError **)error
{ assert(argument == expected_argument && error == expected_error); original_begin(); if(error)*error=answer_error; return answer; }
@end
@interface SnapshotQuadHost : NSObject
- (id)snapshotInRect:(CGRect)rectangle scale:(CGFloat)scale time:(double)time;
@end
@implementation SnapshotQuadHost
- (id)snapshotInRect:(CGRect)rectangle scale:(CGFloat)scale time:(double)time
{ assert(!memcmp(&rectangle,&expected_rectangle,sizeof(rectangle)) && scale==2.0 && time==3.75); original_begin(); return answer; }
@end
@interface SnapshotCommandsHost : NSObject
- (void)waitUntilCompleted;
- (void)executeOperations;
@end
@implementation SnapshotCommandsHost
- (void)waitUntilCompleted { original_begin(); }
- (void)executeOperations { original_begin(); }
@end
@interface SnapshotTextureHost : NSObject
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)row fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level;
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)row bytesPerImage:(NSUInteger)image fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice;
@end
@implementation SnapshotTextureHost
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)row fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level
{ assert(bytes==expected_bytes && row==expected_row && level==expected_level && !memcmp(&region,&expected_region,sizeof(region))); original_begin(); memset(bytes,0x45,8); }
- (void)getBytes:(void *)bytes bytesPerRow:(NSUInteger)row bytesPerImage:(NSUInteger)image fromRegion:(MTLRegion)region mipmapLevel:(NSUInteger)level slice:(NSUInteger)slice
{ assert(bytes==expected_bytes && row==expected_row && image==expected_image && level==expected_level && slice==expected_slice && !memcmp(&region,&expected_region,sizeof(region))); original_begin(); memset(bytes,0x67,8); }
@end
@interface SnapshotWrongScaleHost : NSObject
- (id)snapshotInRect:(CGRect)rectangle scale:(float)scale time:(double)time;
@end
@implementation SnapshotWrongScaleHost
- (id)snapshotInRect:(CGRect)rectangle scale:(float)scale time:(double)time
{ (void)rectangle; (void)scale; (void)time; assert(!"Invalid ABI must not be called"); return nil; }
@end
@interface SnapshotInheritedHost : SnapshotQuadHost @end
@implementation SnapshotInheritedHost @end

static void emit(const char *format, ...)
{
    char line[512]; va_list args; va_start(args,format); int size=vsnprintf(line,sizeof(line),format,args); va_end(args);
    assert(size>0 && size<250);
    if(strstr(line,"SWEET POSTDRAW BEGIN "))++begin_records;
    if(strstr(line,"SWEET POSTDRAW END "))++end_records;
    puts(line); errno=117;
}
int main(void) { @autoreleasepool {
    assert(postdraw_observe(emit));
    assert(snapshot_methods_observe([SnapshotDeviceHost class],[SnapshotQuadHost class],[SnapshotCommandsHost class],[SnapshotTextureHost class])==8);
    SnapshotDeviceHost *device=[SnapshotDeviceHost new]; SnapshotQuadHost *quad=[SnapshotQuadHost new];
    SnapshotCommandsHost *commands=[SnapshotCommandsHost new]; SnapshotTextureHost *texture=[SnapshotTextureHost new];
    expected_argument=[NSObject new]; id original_answer=[NSObject new]; answer=original_answer;
    answer_error=[NSError errorWithDomain:@"original" code:77 userInfo:nil]; NSError *error=nil; expected_error=&error;
    expected_rectangle=CGRectMake(-1.25,3.5,750,1624); expected_region=MTLRegionMake3D(7,8,9,10,11,12);
    expected_row=1600; expected_image=123456; expected_level=2; expected_slice=3;
    uint8_t bytes[24]; memset(bytes,0xa7,sizeof(bytes)); expected_bytes=bytes+8;
    for(unsigned pass=0;pass<10;++pass) {
        errno=73; assert([device newRenderPipelineStateWithDescriptor:expected_argument error:&error]==answer && error==answer_error && errno==91);
        errno=73; assert([device newLibraryWithURL:expected_argument error:&error]==answer && error==answer_error && errno==91);
        errno=73; assert([device newDefaultLibraryWithBundle:expected_argument error:&error]==answer && error==answer_error && errno==91);
        errno=73; assert([quad snapshotInRect:expected_rectangle scale:2 time:3.75]==answer && errno==91);
        errno=73; [commands waitUntilCompleted]; assert(errno==91);
        errno=73; [commands executeOperations]; assert(errno==91);
        errno=73; [texture getBytes:expected_bytes bytesPerRow:expected_row fromRegion:expected_region mipmapLevel:expected_level]; assert(errno==91);
        for(unsigned i=0;i<8;++i) assert(bytes[i]==0xa7 && bytes[8+i]==0x45 && bytes[16+i]==0xa7);
        errno=73; [texture getBytes:expected_bytes bytesPerRow:expected_row bytesPerImage:expected_image fromRegion:expected_region mipmapLevel:expected_level slice:expected_slice]; assert(errno==91);
        for(unsigned i=0;i<8;++i) assert(bytes[i]==0xa7 && bytes[8+i]==0x67 && bytes[16+i]==0xa7);
        assert(!postdraw_depth && !postdraw_root);
    }
    answer=nil; expected_error=NULL; errno=73;
    assert([device newRenderPipelineStateWithDescriptor:expected_argument error:NULL]==nil && errno==91);
    errno=73; assert([quad snapshotInRect:expected_rectangle scale:2 time:3.75]==nil && errno==91);
    should_throw=true; answer_exception=[NSException exceptionWithName:@"OriginalSnapshotThrow" reason:@"kept" userInfo:nil];
    for(unsigned pass=0;pass<3;++pass) {
        bool caught=false; errno=73;
        @try {
            if(pass==0)(void)[device newLibraryWithURL:expected_argument error:NULL];
            if(pass==1)[commands waitUntilCompleted];
            if(pass==2)[texture getBytes:expected_bytes bytesPerRow:expected_row fromRegion:expected_region mipmapLevel:expected_level];
        } @catch(NSException *exception) { assert(exception==answer_exception); caught=true; }
        assert(caught && errno==91 && !postdraw_depth && !postdraw_root);
    }
    const char *arguments[]={@encode(CGRect),@encode(CGFloat),@encode(double)};
    IMP refused=NULL, original_bad=class_getMethodImplementation([SnapshotWrongScaleHost class],sel_registerName("snapshotInRect:scale:time:"));
    assert(!snapshot_method_install([SnapshotWrongScaleHost class],"snapshotInRect:scale:time:","snapshot",(IMP)snapshot_quad,&refused,@encode(id),arguments,3));
    assert(!refused && original_bad==class_getMethodImplementation([SnapshotWrongScaleHost class],sel_registerName("snapshotInRect:scale:time:")));
    assert(!snapshot_method_install([SnapshotInheritedHost class],"snapshotInRect:scale:time:","snapshot",(IMP)snapshot_quad,&refused,@encode(id),arguments,3));
    const char *factory_arguments[]={@encode(id),@encode(NSError **)};
    assert(!snapshot_method_install([SnapshotDeviceHost class],"newRenderPipelineStateWithDescriptor:error:","pipeline",(IMP)snapshot_pipeline,&refused,@encode(id),factory_arguments,2));
    assert(!refused && begin_records==64 && end_records==64 && original_calls==85);
    [original_answer release]; [expected_argument release]; [device release]; [quad release]; [commands release]; [texture release];
    puts("snapshot method observer PASS: eight exact ABIs/arguments/returns/errors/errno/throws, original read bytes/guards, invalid/inherited/repeated hook rejection, 64-record bound; no GPU work");
} return 0; }
