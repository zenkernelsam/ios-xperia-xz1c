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
#include <errno.h>

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
static Class compatible_provider,native_provider;
static unsigned provider_traces;
static Class choose_provider(provider_picker original,id receiver,SEL selector,id screen,id info)
{
    Class answer=original(receiver,selector,screen,info);
    const char *name=answer?class_getName(answer):"nil";
    Class type=screen?object_getClass(screen):Nil;
    SEL bounds=sel_registerName("bounds"),scale=sel_registerName("scale"),native=sel_registerName("nativeBounds");
    bool eligible=false,nativeEligible=false;CGRect b=CGRectZero,n=CGRectZero;double s=0;
    if (type && class_getInstanceMethod(type,bounds) && class_getInstanceMethod(type,scale) && class_getInstanceMethod(type,native)) {
        b=screen_rect(screen,bounds);n=screen_rect(screen,native);
        s=((CGFloat (*)(id,SEL))objc_msgSend)(screen,scale);
        eligible=fabs(b.size.width-375)<0.01 && fabs(b.size.height-812)<0.01 && fabs(s-2)<0.01 &&
                 fabs(n.size.width-750)<0.01 && fabs(n.size.height-1624)<0.01;
        nativeEligible=fabs(b.size.width-360)<0.01 && fabs(b.size.height-640)<0.01 && fabs(s-2)<0.01 &&
                       fabs(n.size.width-720)<0.01 && fabs(n.size.height-1280)<0.01;
    }
    unsigned trace=__atomic_fetch_add(&provider_traces,1,__ATOMIC_RELAXED);
    Class selected=nativeEligible?native_provider:eligible?compatible_provider:Nil;
    if ((eligible || nativeEligible) && (!answer || strstr(name,"Unsupported") || strstr(name,"Fallback")) && selected) {
        if (trace<16) { char line[256];snprintf(line,sizeof(line),"SWEET GEOCOMPAT: from=%s to=%s bounds=%gx%g scale=%g abi=4\n",name,class_getName(selected),b.size.width,b.size.height,s);emit(line); }
        return selected;
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
    native_provider=objc_lookUpClass("_UIStatusBarVisualProvider_LegacyPhone");
    if (!compatible_provider) { emit("SWEET GEOCOMPAT: REFUSE missing Split1125\n");return; }
    if(!native_provider)emit("SWEET GEOCOMPAT: native LegacyPhone unavailable; keep original native picker result\n");
    bool ios=hook_picker("_UIStatusBarVisualProvider_iOS",&original_ios,(IMP)select_ios);
    bool split=hook_picker("_UIStatusBarVisualProvider_Split",&original_split,(IMP)select_split);
    if (!ios && !split) emit("SWEET GEOCOMPAT: REFUSE no matching 4-argument ABI\n");
}

typedef struct SweetGeoInsets {double top,left,bottom,right;} SweetGeoInsets;
static void inspect_window_safe_area(void)
{
    Class appClass=objc_lookUpClass("UIApplication");SEL shared=sel_registerName("sharedApplication"),windows=sel_registerName("windows"),safe=sel_registerName("safeAreaInsets");
    Method sharedMethod=appClass?class_getClassMethod(appClass,shared):NULL;char ret[64]={0};
    if(sharedMethod)method_getReturnType(sharedMethod,ret,sizeof(ret));
    if(!sharedMethod || method_getNumberOfArguments(sharedMethod)!=2 || ret[0]!='@')return;
    id app=((id (*)(id,SEL))objc_msgSend)((id)appClass,shared);Method getWindows=app?class_getInstanceMethod(object_getClass(app),windows):NULL;
    if(!getWindows || method_getNumberOfArguments(getWindows)!=2)return;method_getReturnType(getWindows,ret,sizeof(ret));if(ret[0]!='@')return;
    id values=((id (*)(id,SEL))objc_msgSend)(app,windows);if(!values || CFGetTypeID((CFTypeRef)values)!=CFArrayGetTypeID())return;
    CFIndex count=CFArrayGetCount((CFArrayRef)values);if(count<0 || count>32)return;
    for(CFIndex i=0;i<count && i<8;++i){id window=(id)CFArrayGetValueAtIndex((CFArrayRef)values,i);Method method=window?class_getInstanceMethod(object_getClass(window),safe):NULL;
        if(!method || method_getNumberOfArguments(method)!=2)continue;method_getReturnType(method,ret,sizeof(ret));if(strcmp(ret,"{UIEdgeInsets=dddd}"))continue;
        SweetGeoInsets insets;
#if defined(__x86_64__)
        ((void (*)(SweetGeoInsets *,id,SEL))objc_msgSend_stret)(&insets,window,safe);
#else
        insets=((SweetGeoInsets (*)(id,SEL))objc_msgSend)(window,safe);
#endif
        char line[240];snprintf(line,sizeof(line),"SWEET GEO: window=%ld safe_area=%g,%g,%g,%g ABI=UIEdgeInsets64\n",(long)i,insets.top,insets.left,insets.bottom,insets.right);emit(line);
    }
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
    inspect_window_safe_area();
}

/* One main-thread, read-only view/grid snapshot. Never request a relayout. */
static const char *geo_rect_type="{CGRect={CGPoint=dd}{CGSize=dd}}";
static bool geo_method_type(id object,const char *name,const char *result,unsigned arguments=2,
                            const char *arg2=NULL,const char *arg3=NULL)
{
    Method method=object?class_getInstanceMethod(object_getClass(object),sel_registerName(name)):NULL;
    if(!method || method_getNumberOfArguments(method)!=arguments)return false;
    char type[128]={0};method_getReturnType(method,type,sizeof(type));
    if(strcmp(type,result))return false;
    if(arg2){method_getArgumentType(method,2,type,sizeof(type));if(strcmp(type,arg2))return false;}
    if(arg3){method_getArgumentType(method,3,type,sizeof(type));if(strcmp(type,arg3))return false;}
    return true;
}
static bool geo_kind(id object,const char *name)
{
    Class wanted=objc_lookUpClass(name);if(!wanted || !object)return false;
    for(Class type=object_getClass(object);type;type=class_getSuperclass(type))if(type==wanted)return true;
    return false;
}
static id geo_object(id object,const char *name)
{
    if(!geo_method_type(object,name,"@"))return nil;
    return ((id (*)(id,SEL))objc_msgSend)(object,sel_registerName(name));
}
static bool geo_rectangle(id object,const char *name,CGRect *value)
{
    if(!geo_method_type(object,name,geo_rect_type))return false;
    *value=screen_rect(object,sel_registerName(name));
    return isfinite(value->origin.x) && isfinite(value->origin.y) && isfinite(value->size.width) && isfinite(value->size.height) &&
           fabs(value->origin.x)<=32768 && fabs(value->origin.y)<=32768 && fabs(value->size.width)<=32768 && fabs(value->size.height)<=32768;
}
static bool geo_window_rectangle(id object,CGRect frame,CGRect *value)
{
    if(!geo_method_type(object,"convertRect:toView:",geo_rect_type,4,geo_rect_type,"@"))return false;
#if defined(__x86_64__)
    ((void (*)(CGRect *,id,SEL,CGRect,id))objc_msgSend_stret)(value,object,sel_registerName("convertRect:toView:"),frame,nil);
#else
    *value=((CGRect (*)(id,SEL,CGRect,id))objc_msgSend)(object,sel_registerName("convertRect:toView:"),frame,nil);
#endif
    return isfinite(value->origin.x) && isfinite(value->origin.y) && isfinite(value->size.width) && isfinite(value->size.height) &&
           fabs(value->origin.x)<=32768 && fabs(value->origin.y)<=32768 && fabs(value->size.width)<=32768 && fabs(value->size.height)<=32768;
}
static bool geo_size(id object,const char *name,CGSize *value)
{
    if(!geo_method_type(object,name,"{CGSize=dd}"))return false;
    *value=((CGSize (*)(id,SEL))objc_msgSend)(object,sel_registerName(name));
    return isfinite(value->width) && isfinite(value->height) && fabs(value->width)<=32768 && fabs(value->height)<=32768;
}
static bool geo_count(id object,const char *name,unsigned long long *value)
{
    SEL selector=sel_registerName(name);
    if(geo_method_type(object,name,"Q"))*value=((unsigned long long (*)(id,SEL))objc_msgSend)(object,selector);
    else if(geo_method_type(object,name,"q")){
        long long number=((long long (*)(id,SEL))objc_msgSend)(object,selector);if(number<0)return false;*value=(unsigned long long)number;
    }else return false;
    return *value<=2048;
}
static bool geo_native_screen(void)
{
    id screen=geo_object((id)objc_lookUpClass("UIScreen"),"mainScreen");CGRect b,n;
    if(!geo_rectangle(screen,"bounds",&b) || !geo_rectangle(screen,"nativeBounds",&n) || !geo_method_type(screen,"scale","d"))return false;
    double s=((double (*)(id,SEL))objc_msgSend)(screen,sel_registerName("scale"));
    return fabs(b.size.width-360)<0.01 && fabs(b.size.height-640)<0.01 && fabs(n.size.width-720)<0.01 && fabs(n.size.height-1280)<0.01 && fabs(s-2)<0.01;
}
struct SweetGeoLayoutResult {unsigned visited,recorded,grids,refused,truncated;};
static bool geo_grid(id view,unsigned number)
{
    if(!geo_kind(view,"SBIconListView"))return false;
    id layout=geo_object(view,"layout");
    if(!geo_kind(layout,"SBIconListGridLayout"))return false;
    id config=geo_object(layout,"layoutConfiguration");
    if(!geo_kind(config,"SBIconListGridLayoutConfiguration"))return false;
    unsigned long long rows=0,columns=0;CGSize spacing={0,0},list={0,0};
    bool count=geo_count(config,"numberOfPortraitRows",&rows) && geo_count(config,"numberOfPortraitColumns",&columns);
    bool sizes=geo_size(view,"iconSpacing",&spacing) && geo_size(config,"listSizeForIconSpacingCalculation",&list);
    char line[190];snprintf(line,sizeof(line),"SWEET GLAYOUT GRID n=%u rows=%llu cols=%llu spacing=%.3f,%.3f list=%.3f,%.3f valid=%u\n",number,rows,columns,spacing.width,spacing.height,list.width,list.height,(unsigned)(count&&sizes));emit(line);
    if(geo_method_type(config,"portraitLayoutInsets","{UIEdgeInsets=dddd}")){
        SweetGeoInsets insets;
#if defined(__x86_64__)
        ((void (*)(SweetGeoInsets *,id,SEL))objc_msgSend_stret)(&insets,config,sel_registerName("portraitLayoutInsets"));
#else
        insets=((SweetGeoInsets (*)(id,SEL))objc_msgSend)(config,sel_registerName("portraitLayoutInsets"));
#endif
        snprintf(line,sizeof(line),"SWEET GLAYOUT INSETS n=%u portrait=%.3f,%.3f,%.3f,%.3f\n",number,insets.top,insets.left,insets.bottom,insets.right);emit(line);
    }
    return count&&sizes;
}
static SweetGeoLayoutResult inspect_native_layout(unsigned after_ctor_seconds)
{
    SweetGeoLayoutResult result={0,0,0,0,0};int saved_errno=errno;
    const char *enabled=getenv("GEO_FIX");
    if(!enabled || strcmp(enabled,"1") || !geo_native_screen()){errno=saved_errno;return result;}
    char line[190];snprintf(line,sizeof(line),"SWEET GLAYOUT BEGIN rev=1 after_ctor=%u points=360,640 pixels=720,1280 scale=2\n",after_ctor_seconds);emit(line);
    @try {
        id app=geo_object((id)objc_lookUpClass("UIApplication"),"sharedApplication"),windows=geo_object(app,"windows");
        struct Node {id object;unsigned parent,depth;} nodes[2048];unsigned queued=0;
        if(!geo_kind(windows,"NSArray"))++result.refused;
        else{
            CFIndex count=CFArrayGetCount((CFArrayRef)windows);
            if(count<0 || count>32)++result.refused;
            else for(CFIndex i=0;i<count;++i)nodes[queued++]={(id)CFArrayGetValueAtIndex((CFArrayRef)windows,i),0,0};
        }
        for(unsigned i=0;i<queued;++i){
            id view=nodes[i].object;++result.visited;if(!geo_kind(view,"UIView")){++result.refused;continue;}
            const char *name=class_getName(object_getClass(view));
            bool relevant=nodes[i].parent==0 || strstr(name,"Icon") || strstr(name,"Widget") || strstr(name,"Label") || strstr(name,"Dock");
            if(relevant){
                if(strlen(name)>64)result.truncated=1;
                CGRect frame,bounds,window;
                if(geo_rectangle(view,"frame",&frame) && geo_rectangle(view,"bounds",&bounds) && geo_window_rectangle(view,bounds,&window)){
                    if(result.recorded>=384)result.truncated=1;
                    else{
                        ++result.recorded;snprintf(line,sizeof(line),"SWEET GLAYOUT VIEW n=%u parent=%u depth=%u class=%.64s\n",i+1,nodes[i].parent,nodes[i].depth,name);emit(line);
                        snprintf(line,sizeof(line),"SWEET GLAYOUT RECT n=%u frame=%.3f,%.3f,%.3f,%.3f screen=%.3f,%.3f,%.3f,%.3f\n",i+1,frame.origin.x,frame.origin.y,frame.size.width,frame.size.height,window.origin.x,window.origin.y,window.size.width,window.size.height);emit(line);
                    }
                }else ++result.refused;
                if(result.grids<8 && geo_grid(view,i+1))++result.grids;
            }
            id children=geo_object(view,"subviews");if(!geo_kind(children,"NSArray")){++result.refused;continue;}
            CFIndex count=CFArrayGetCount((CFArrayRef)children);if(count<0 || count>2048){result.truncated=1;continue;}
            for(CFIndex k=0;k<count;++k){
                id child=(id)CFArrayGetValueAtIndex((CFArrayRef)children,k);bool seen=false;
                for(unsigned j=0;j<queued;++j)if(nodes[j].object==child){seen=true;break;}
                if(seen){++result.refused;continue;}
                if(queued>=2048 || nodes[i].depth>=64){result.truncated=1;continue;}
                nodes[queued++]={child,i+1,nodes[i].depth+1};
            }
        }
    }@catch(id exception){(void)exception;++result.refused;emit("SWEET GLAYOUT REFUSE getter_exception\n");}
    snprintf(line,sizeof(line),"SWEET GLAYOUT END rev=1 visited=%u recorded=%u grids=%u refused=%u truncated=%u\n",result.visited,result.recorded,result.grids,result.refused,result.truncated);emit(line);
    errno=saved_errno;return result;
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
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,135*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { (void)inspect_native_layout(135); }
    });
}
