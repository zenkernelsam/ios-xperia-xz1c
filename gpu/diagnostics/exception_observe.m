/* AegirPoster-only metadata observer. Continue the original exception chain. */
#import <Foundation/Foundation.h>
#include <objc/objc-exception.h>
#include <stdatomic.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static objc_exception_preprocessor previous;
static atomic_uint observations;
static _Thread_local bool logging_exception;
static int log_fd = -1;
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
    puts("original exception identity/chain, caught behavior and bounded observation PASS");
}return 0;}
#endif
