/* Read-only display answers, deferred until SpringBoard has initialized. */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGGeometry.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

static void emit(const char *line)
{
    (void)write(STDERR_FILENO, line, strlen(line));
}
static CGRect screen_rect(id object,SEL selector)
{
#if defined(__x86_64__)
    CGRect value;
    ((void (*)(CGRect *,id,SEL))objc_msgSend_stret)(&value,object,selector);
    return value;
#else
    return ((CGRect (*)(id,SEL))objc_msgSend)(object,selector);
#endif
}

typedef Class (*provider_picker)(id,SEL,id,id);
static provider_picker original_ios,original_split;
static Class compatible_provider;
static unsigned provider_traces;
static Class choose_provider(provider_picker original,id receiver,SEL selector,id screen,id info)
{
    Class answer=original(receiver,selector,screen,info);
    const char *name=answer?class_getName(answer):"nil";
    Class type=screen?object_getClass(screen):Nil;
    SEL bounds=sel_registerName("bounds"),scale=sel_registerName("scale"),native=sel_registerName("nativeBounds");
    bool eligible=false;
    if (type && class_getInstanceMethod(type,bounds) && class_getInstanceMethod(type,scale) && class_getInstanceMethod(type,native)) {
        CGRect b=screen_rect(screen,bounds),n=screen_rect(screen,native);
        double s=((CGFloat (*)(id,SEL))objc_msgSend)(screen,scale);
        eligible=fabs(b.size.width-375)<0.01 && fabs(b.size.height-812)<0.01 && fabs(s-2)<0.01 &&
                 fabs(n.size.width-750)<0.01 && fabs(n.size.height-1624)<0.01;
    }
    unsigned trace=__atomic_fetch_add(&provider_traces,1,__ATOMIC_RELAXED);
    if (eligible && (!answer || strstr(name,"Unsupported") || strstr(name,"Fallback")) && compatible_provider) {
        if (trace<16) { char line[256];snprintf(line,sizeof(line),"SWEET GEOCOMPAT: from=%s to=%s bounds=375x812 scale=2 abi=4\n",name,class_getName(compatible_provider));emit(line); }
        return compatible_provider;
    }
    if (trace<16) { char line[256];snprintf(line,sizeof(line),"SWEET GEOCOMPAT: keep=%s eligible=%u abi=4\n",name,(unsigned)eligible);emit(line); }
    return answer;
}
static Class select_ios(id receiver,SEL selector,id screen,id info) { return choose_provider(original_ios,receiver,selector,screen,info); }
static Class select_split(id receiver,SEL selector,id screen,id info) { return choose_provider(original_split,receiver,selector,screen,info); }
static bool hook_picker(const char *name,provider_picker *original,IMP replacement)
{
    Class base=objc_lookUpClass(name);
    SEL selector=sel_registerName("visualProviderSubclassForScreen:visualProviderInfo:");
    Method method=base?class_getClassMethod(base,selector):NULL;
    char result[16]={0},screen[16]={0},info[16]={0};
    if (method) { method_getReturnType(method,result,sizeof(result));method_getArgumentType(method,2,screen,sizeof(screen));method_getArgumentType(method,3,info,sizeof(info)); }
    if (!method || method_getNumberOfArguments(method)!=4 || result[0]!='#' || screen[0]!='@' || info[0]!='@') return false;
    *original=(provider_picker)method_getImplementation(method);
    if (!*original) return false;
    method_setImplementation(method,replacement);
    char line[256];snprintf(line,sizeof(line),"SWEET GEOCOMPAT: hooked %s abi=#32@0:8@16@24\n",name);emit(line);
    return true;
}
static void install_provider_adapter(void)
{
    const char *enabled=getenv("GEO_FIX");
    if (!enabled || strcmp(enabled,"1") || original_ios || original_split) return;
    compatible_provider=objc_lookUpClass("_UIStatusBarVisualProvider_Split1125");
    if (!compatible_provider) { emit("SWEET GEOCOMPAT: REFUSE missing Split1125\n");return; }
    bool ios=hook_picker("_UIStatusBarVisualProvider_iOS",&original_ios,(IMP)select_ios);
    bool split=hook_picker("_UIStatusBarVisualProvider_Split",&original_split,(IMP)select_split);
    if (!ios && !split) emit("SWEET GEOCOMPAT: REFUSE no matching 4-argument ABI\n");
}

static void inspect_geometry(void)
{
    char line[640];
    Class ui = objc_lookUpClass("UIScreen");
    SEL main = sel_registerName("mainScreen");
    if (ui && class_getClassMethod(ui, main)) {
        id screen = ((id (*)(id, SEL))objc_msgSend)((id)ui, main);
        Class type = screen ? object_getClass(screen) : Nil;
        SEL bounds = sel_registerName("bounds"), scale = sel_registerName("scale");
        SEL nativeBounds = sel_registerName("nativeBounds"), nativeScale = sel_registerName("nativeScale");
        if (type && class_getInstanceMethod(type, bounds) && class_getInstanceMethod(type, scale)) {
            CGRect b = screen_rect(screen,bounds);
            double s = ((CGFloat (*)(id, SEL))objc_msgSend)(screen, scale);
            CGRect n = CGRectZero;
            double ns = 0;
            if (class_getInstanceMethod(type, nativeBounds)) n = screen_rect(screen,nativeBounds);
            if (class_getInstanceMethod(type, nativeScale)) ns = ((CGFloat (*)(id, SEL))objc_msgSend)(screen, nativeScale);
            snprintf(line, sizeof(line), "SWEET GEO: pid=%d bounds=%g,%g,%g,%g scale=%g native=%g,%g,%g,%g nativeScale=%g\n",
                     getpid(), b.origin.x,b.origin.y,b.size.width,b.size.height,s,
                     n.origin.x,n.origin.y,n.size.width,n.size.height,ns);
            emit(line);
        }
    } else {
        snprintf(line, sizeof(line), "SWEET GEO: pid=%d UIScreen unavailable\n", getpid());
        emit(line);
    }
    typedef CFTypeRef (*copy_answer)(CFStringRef);
    copy_answer copy = (copy_answer)dlsym(RTLD_DEFAULT, "MGCopyAnswer");
    if (!copy) { emit("SWEET GEO: MGCopyAnswer unavailable\n"); return; }
    const char *keys[] = {"main-screen-width", "main-screen-height", "main-screen-scale", "DisplayPixelDensity", "ArtworkDeviceSubType"};
    for (const char *key : keys) {
        CFStringRef question = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
        CFTypeRef value = question ? copy(question) : NULL;
        double number = 0;
        if (value && CFGetTypeID(value)==CFNumberGetTypeID() && CFNumberGetValue((CFNumberRef)value,kCFNumberDoubleType,&number))
            snprintf(line,sizeof(line),"SWEET GEO: MG %s=%g\n",key,number);
        else
            snprintf(line,sizeof(line),"SWEET GEO: MG %s type=%lu\n",key,value?(unsigned long)CFGetTypeID(value):0ul);
        emit(line);
        if (value) CFRelease(value);
        if (question) CFRelease(question);
    }
}
__attribute__((constructor)) static void start_geometry_reader(void)
{
    install_provider_adapter();
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { inspect_geometry(); }
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,20*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { inspect_geometry(); }
    });
}
