/* AegirPoster-only metadata observer. Continue the original exception chain. */
#import <Foundation/Foundation.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "MetalRuntimeObserve.h"
#if defined(SWEET_METAL_RESOURCE_PROBE)
#include "MetalResourceProbe.h"
#endif
#if defined(SWEET_METAL_QUEUE_PROBE)
#include "MetalQueueProbe.h"
#endif
#if defined(SWEET_AIR_COMPUTE_PROBE)
#include "AIRFixture.h"
#include "MetalComputeProbe.h"
#endif

#if defined(LILAC_EXCEPTION_TEST)
struct MetalABIHostLimits { uint32_t fields[63];uint64_t maxBufferLength; };
@protocol MetalABIHostProtocol <NSObject>
- (id)newBufferWithLength:(NSUInteger)length options:(NSUInteger)options;
@end
@interface MetalABIHostFixture : NSObject <MetalABIHostProtocol> { struct MetalABIHostLimits _limits; }
- (id)initWithAcceleratorPort:(uint32_t)port;
- (void)initLimits;
- (void)initFeatureQueries;
- (NSUInteger)featureProfile;
@end
@implementation MetalABIHostFixture
- (id)initWithAcceleratorPort:(uint32_t)port{(void)port;assert(!"metadata must not initialize a device");return nil;}
- (void)initLimits{assert(!"metadata must not query device limits");}
- (void)initFeatureQueries{assert(!"metadata must not initialize device features");}
- (NSUInteger)featureProfile{assert(!"metadata must not query a device");return 0;}
- (id)newBufferWithLength:(NSUInteger)length options:(NSUInteger)options
{(void)length;(void)options;assert(!"metadata must not allocate a buffer");return nil;}
@end
static unsigned uniform_original_calls;
@interface NUNITestDevice : NSObject
- (id)newBufferWithLength:(NSUInteger)length options:(NSUInteger)options;
@end
@implementation NUNITestDevice
- (id)newBufferWithLength:(NSUInteger)length options:(NSUInteger)options
{ (void)length;(void)options;assert(!"observer must not allocate a Metal buffer");return nil; }
@end
@interface NUNIAegirRenderer : NSObject { @public id _device; }
- (id)getOrCreateUniformBufferforFrameBufferIndex:(NSUInteger)index;
@end
@implementation NUNIAegirRenderer
- (id)getOrCreateUniformBufferforFrameBufferIndex:(NSUInteger)index
{ assert(index<3);++uniform_original_calls;return _device; }
@end
#endif

static objc_exception_preprocessor previous;
static atomic_uint observations;
static _Thread_local bool logging_exception;
static int log_fd = -1;
static id (*uniform_original)(id,SEL,NSUInteger);
static Ivar renderer_device;
static atomic_uint uniform_observations;
static void diagnostic(const char *format, ...)
{
    char line[512]; va_list args; va_start(args, format);
    int n = vsnprintf(line, sizeof(line)-1, format, args); va_end(args);
    if (n < 0) return;
    if (n > sizeof(line)-2) n = sizeof(line)-2;
    line[n++]='\n'; if(log_fd>=0) (void)write(log_fd,line,n);
}
static void copy_string(NSString *value, char out[192])
{
    out[0]=0;
    if (![value isKindOfClass:[NSString class]]) return;
    NSData *data=[value dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:YES];
    NSUInteger bytes=MIN([data length],191);
    if(bytes) memcpy(out,[data bytes],bytes);
    out[bytes]=0;
    for(NSUInteger i=0;i<bytes;++i) if((unsigned char)out[i]<32 || out[i]=='"') out[i]=' ';
}
static id observe_uniform(id object,SEL selector,NSUInteger index)
{
    unsigned call=atomic_fetch_add_explicit(&uniform_observations,1,memory_order_relaxed)+1;
    if(call<=32) {
        id device=object_getIvar(object,renderer_device);
        Class type=device?object_getClass(device):Nil;
        bool supports=type && class_getInstanceMethod(type,sel_registerName("newBufferWithLength:options:"));
        diagnostic("SWEET METAL uniform call=%u index=%lu device_nil=%u device_class=%s new_buffer_method=%u metadata_only=1 original_called=1",
            call,(unsigned long)index,device==nil,type?class_getName(type):"nil",supports);
    }
    // Forward every call, including those beyond the diagnostic limit. Do not
    // manufacture a buffer or catch the original renderer's exception.
    return uniform_original(object,selector,index);
}
static void install_uniform_observer(void)
{
    Class renderer=objc_lookUpClass("NUNIAegirRenderer");
    SEL selector=sel_registerName("getOrCreateUniformBufferforFrameBufferIndex:");
    Method method=renderer?class_getInstanceMethod(renderer,selector):NULL;
    Ivar device=renderer?class_getInstanceVariable(renderer,"_device"):NULL;
    char result[16]={0},index[16]={0};
    if(method){method_getReturnType(method,result,sizeof(result));method_getArgumentType(method,2,index,sizeof(index));}
    bool valid=method && device && method_getNumberOfArguments(method)==3 && result[0]=='@' &&
        (index[0]=='Q' || index[0]=='q') && ivar_getTypeEncoding(device)[0]=='@' &&
        ivar_getOffset(device)>=0 && (size_t)ivar_getOffset(device)+sizeof(id)<=class_getInstanceSize(renderer);
    if(!valid){diagnostic("SWEET METAL observer refused renderer_abi metadata_only=1");return;}
    renderer_device=device;uniform_original=(id(*)(id,SEL,NSUInteger))method_getImplementation(method);
    if(!uniform_original){diagnostic("SWEET METAL observer refused original_missing metadata_only=1");return;}
    method_setImplementation(method,(IMP)observe_uniform);
    diagnostic("SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=%c device_object=1 original_forwarded=1",index[0]);
}
static id observe_exception(id exception)
{
    if (!logging_exception) {
        logging_exception=true;
        unsigned call=atomic_fetch_add_explicit(&observations,1,memory_order_relaxed)+1;
        if (call<=16) {
            @try {
                char name[192]={0},reason[192]={0};
                if([exception respondsToSelector:@selector(name)]) copy_string([exception name],name);
                if([exception respondsToSelector:@selector(reason)]) copy_string([exception reason],reason);
                diagnostic("SWEET EXCEPTION begin call=%u class=%s name=\"%s\" reason=\"%s\" observational=1",
                    call,object_getClassName(exception),name,reason);
                void *stack[16];int count=backtrace(stack,16);
                for(int i=0;i<count;++i) {
                    Dl_info info={0};dladdr(stack[i],&info);
                    const char *file=info.dli_fname?strrchr(info.dli_fname,'/'):NULL;
                    diagnostic("SWEET EXCEPTION frame call=%u index=%d pc=%p base=%p image=%s symbol=%s",
                        call,i,stack[i],info.dli_fbase,file?file+1:(info.dli_fname?info.dli_fname:"?"),
                        info.dli_sname?info.dli_sname:"?");
                }
                diagnostic("SWEET EXCEPTION end call=%u frames=%d original_chain=1",call,count);
            } @catch (id diagnostic_error) {
                (void)diagnostic_error;diagnostic("SWEET EXCEPTION logging-failed call=%u original_chain=1",call);
            }
        }
        logging_exception=false;
    }
    return previous ? previous(exception) : exception;
}
static void install_observer(void)
{
#if defined(LILAC_EXCEPTION_TEST)
    log_fd=STDOUT_FILENO;
#else
    if(strcmp(getprogname(),"AegirPoster")!=0) return;
    log_fd=open("/dev/null",O_WRONLY|O_CLOEXEC);
#endif
    previous=objc_setExceptionPreprocessor(observe_exception);
    diagnostic("SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1");
    install_uniform_observer();
#if defined(LILAC_EXCEPTION_TEST)
    metal_runtime_observe([MetalABIHostFixture class],@protocol(MetalABIHostProtocol),NULL,NULL,NULL,diagnostic);
#else
    metal_runtime_observe(objc_lookUpClass("_MTLDevice"),objc_getProtocol("MTLDeviceSPI"),objc_getProtocol("MTLBuffer"),
        objc_getProtocol("MTLCommandQueue"),objc_getProtocol("MTLCommandBuffer"),diagnostic);
#if defined(SWEET_METAL_RESOURCE_PROBE)
    (void)metal_resource_probe(diagnostic);
#endif
#if defined(SWEET_METAL_QUEUE_PROBE)
    (void)metal_queue_probe(diagnostic);
#endif
#if defined(SWEET_AIR_COMPUTE_PROBE)
    (void)metal_compute_probe(diagnostic);
#endif
#endif
}
#if !defined(LILAC_EXCEPTION_TEST)
__attribute__((constructor)) static void exception_load(void){install_observer();}
#else
static unsigned chain_calls;static id original;
static id prior(id exception){assert(exception==original);++chain_calls;return exception;}
int main(void){@autoreleasepool{
    objc_setExceptionPreprocessor(prior);install_observer();
    original=[NSException exceptionWithName:@"LilacObserverTest" reason:@"line1\nline2" userInfo:nil];
    for(unsigned i=0;i<20;++i){@try{@throw original;}@catch(id caught){assert(caught==original);}}
    assert(chain_calls==20 && atomic_load(&observations)==20);
    NUNIAegirRenderer *renderer=[NUNIAegirRenderer new];id device=[NUNITestDevice new];
    for(unsigned i=0;i<40;++i){renderer->_device=i%2?device:nil;
        assert([renderer getOrCreateUniformBufferforFrameBufferIndex:i%3]==renderer->_device);}
    assert(uniform_original_calls==40 && atomic_load(&uniform_observations)==40);
    puts("renderer ABI, nil/device metadata, exact original return and 32-record bound PASS");
    [renderer release];[device release];
    puts("original exception identity/chain, caught behavior and bounded observation PASS");
}return 0;}
#endif
