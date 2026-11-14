/* Real method forwarding and exact return/error-pointer/exception identity. */
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/MetalPipelineObserve.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
static id answer;
static NSError *originalError;
static NSError **expectedPointer;
static unsigned originalCalls;
static bool throws;
static NSException *originalException;
@interface SweetPipelineHostFixture : NSObject
- (id)newRenderPipelineStateWithDescriptor:(id)d error:(NSError **)error;
- (id)newLibraryWithURL:(id)url error:(NSError **)error;
- (id)newDefaultLibraryWithBundle:(id)bundle error:(NSError **)error;
@end
@implementation SweetPipelineHostFixture
- (id)newRenderPipelineStateWithDescriptor:(id)d error:(NSError **)error
{(void)d;assert(error==expectedPointer);++originalCalls;if(throws)@throw originalException;if(error)*error=originalError;return answer;}
- (id)newLibraryWithURL:(id)url error:(NSError **)error
{(void)url;assert(error==expectedPointer);++originalCalls;if(throws)@throw originalException;if(error)*error=originalError;return answer;}
- (id)newDefaultLibraryWithBundle:(id)bundle error:(NSError **)error
{(void)bundle;assert(error==expectedPointer);++originalCalls;if(throws)@throw originalException;if(error)*error=originalError;return answer;}
@end
static void emit(const char *format,...)
{char line[512];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);assert(n>0 && n<250);puts(line);}
int main(void){@autoreleasepool{
    assert(metal_pipeline_observe([SweetPipelineHostFixture class],emit));SweetPipelineHostFixture *device=[SweetPipelineHostFixture new];
    id descriptor=[MTLRenderPipelineDescriptor new];((MTLRenderPipelineDescriptor *)descriptor).colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    originalError=[NSError errorWithDomain:@"host-original" code:77 userInfo:@{NSLocalizedDescriptionKey:@"Original compiler rejection"}];answer=[NSObject new];
    NSError *error=nil;expectedPointer=&error;
    for(unsigned i=0;i<36;++i){assert([device newRenderPipelineStateWithDescriptor:descriptor error:&error]==answer && error==originalError);}
    assert(originalCalls==36 && atomic_load(&pipeline_calls)==36);[answer release];answer=nil;error=nil;
    assert(![device newRenderPipelineStateWithDescriptor:descriptor error:&error] && error==originalError);expectedPointer=NULL;
    assert(![device newRenderPipelineStateWithDescriptor:descriptor error:NULL]);
    expectedPointer=&error;answer=[NSObject new];
    assert([device newLibraryWithURL:[NSURL fileURLWithPath:@"/host/default.metallib"] error:&error]==answer && error==originalError);
    assert([device newDefaultLibraryWithBundle:[NSBundle mainBundle] error:&error]==answer && error==originalError);
    throws=true;originalException=[NSException exceptionWithName:@"OriginalFactoryThrow" reason:@"Original throw" userInfo:nil];
    bool caught=false;@try{[device newRenderPipelineStateWithDescriptor:descriptor error:&error];}@catch(NSException *exception){assert(exception==originalException);caught=true;}assert(caught);
    [answer release];[descriptor release];[device release];puts("actual pipeline observer: original return/error-pointer/nil/throw identity and logging bound PASS; no GPU work");
}return 0;}
