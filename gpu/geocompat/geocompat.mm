/* Native display compatibility and bounded original-layout diagnostics. */
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

typedef ssize_t (*SweetGeoLogWriter)(int,const void *,size_t);
extern "C" int sweet_geo_log_write_remaining(int,const void *,size_t,SweetGeoLogWriter);
static unsigned geo_write_failures;
static void emit(const char *line)
{
    if(sweet_geo_log_write_remaining(STDERR_FILENO,line,strlen(line),write)<0)
        __atomic_fetch_add(&geo_write_failures,1,__ATOMIC_RELAXED);
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

typedef struct SweetIconImageInfo {CGSize size;double field2,field3;} SweetIconImageInfo;
typedef id (*SweetLayoutFactory)(id,SEL,id);
static SweetLayoutFactory original_layout_factory,original_layout_cached;
static unsigned native_grid_traces;
static __thread bool native_grid_checking;
static int native_grid_mode(void)
{
    const char *value=getenv("GEO_GRID");
    if(!value || !strcmp(value,"1"))return 1;
    if(!strcmp(value,"0"))return 0;
    return -1;
}
static bool native_root_grid(id layout)
{
    if(native_grid_mode()!=1)return false;
    if(!geo_kind(layout,"SBIconListGridLayout"))return false;
    id config=geo_object(layout,"layoutConfiguration");unsigned long long rows=0,columns=0;
    if(!geo_kind(config,"SBIconListGridLayoutConfiguration") || !geo_count(config,"numberOfPortraitRows",&rows) || rows!=6 ||
       !geo_count(config,"numberOfPortraitColumns",&columns) || columns!=4 || !geo_native_screen())return false;
    CGSize reference;
    if(!geo_size(config,"listSizeForIconSpacingCalculation",&reference) || fabs(reference.width-320)>0.01 || fabs(reference.height-693)>0.01 ||
       !geo_method_type(config,"portraitLayoutInsets","{UIEdgeInsets=dddd}") ||
       !geo_method_type(config,"iconImageInfo","{SBIconImageInfo={CGSize=dd}dd}") ||
       !geo_method_type(config,"setNumberOfPortraitRows:","v",3,"Q"))return false;
    SweetGeoInsets insets;SweetIconImageInfo info;
#if defined(__x86_64__)
    ((void (*)(SweetGeoInsets *,id,SEL))objc_msgSend_stret)(&insets,config,sel_registerName("portraitLayoutInsets"));
    ((void (*)(SweetIconImageInfo *,id,SEL))objc_msgSend_stret)(&info,config,sel_registerName("iconImageInfo"));
#else
    insets=((SweetGeoInsets (*)(id,SEL))objc_msgSend)(config,sel_registerName("portraitLayoutInsets"));
    info=((SweetIconImageInfo (*)(id,SEL))objc_msgSend)(config,sel_registerName("iconImageInfo"));
#endif
    if(!(fabs(insets.top-57)<=0.01 && fabs(insets.left-19)<=0.01 && fabs(insets.bottom-155)<=0.01 && fabs(insets.right-19)<=0.01 &&
         fabs(info.size.width-56)<=0.01 && fabs(info.size.height-56)<=0.01))return false;
    /* R16 original widget image141 / label-origin143.5 / label-height21.
       Keep native objects and text; choose the largest row count that leaves
       8pt between the original two-row widget label and the following icon. */
    const double available=640-insets.top-insets.bottom,widget=141,label_end=143.5+21;
    unsigned long long chosen=rows;double spacing=0,clearance=0;
    for(;chosen>=3;--chosen){
        spacing=(available-chosen*info.size.height)/(chosen-1);
        clearance=2*(info.size.height+spacing)-((2*info.size.height+spacing-widget)/2+label_end);
        if(spacing>=0 && clearance>=8)break;
    }
    if(chosen>=rows || chosen<3)return false;
    ((void (*)(id,SEL,unsigned long long))objc_msgSend)(config,sel_registerName("setNumberOfPortraitRows:"),chosen);
    unsigned long long actual=0;if(!geo_count(config,"numberOfPortraitRows",&actual) || actual!=chosen)return false;
    unsigned trace=__atomic_fetch_add(&native_grid_traces,1,__ATOMIC_RELAXED);
    if(trace<8){char line[190];snprintf(line,sizeof(line),"SWEET GEOGRID APPLY native=360x640 old=%llu rows=%llu cols=4 image=56x56 spacing=%.3f bbox_gap=%.3f original_setter=1\n",rows,chosen,spacing,clearance);emit(line);}
    return true;
}
static id native_layout_return(SweetLayoutFactory original,id receiver,SEL selector,id location)
{
    id result=original(receiver,selector,location);int saved_errno=errno;
    if(!native_grid_checking){
        native_grid_checking=true;
        @try {(void)native_root_grid(result);}
        @catch(id exception){(void)exception;emit("SWEET GEOGRID REFUSE compatibility_getter_exception\n");}
        native_grid_checking=false;
    }
    errno=saved_errno;return result;
}
static id native_make_layout(id receiver,SEL selector,id location){return native_layout_return(original_layout_factory,receiver,selector,location);}
static id native_cached_layout(id receiver,SEL selector,id location){return native_layout_return(original_layout_cached,receiver,selector,location);}
static bool install_native_layout_method(const char *name,SweetLayoutFactory *original,IMP replacement)
{
    Class owner=objc_lookUpClass("SBHDefaultIconListLayoutProvider");if(!owner)return false;
    SEL selector=sel_registerName(name);unsigned count=0;Method *methods=class_copyMethodList(owner,&count);Method found=NULL;
    for(unsigned i=0;i<count;++i)if(method_getName(methods[i])==selector){found=methods[i];break;}
    free(methods);char result[32]={0},arg[32]={0};
    if(!found || method_getNumberOfArguments(found)!=3)return false;
    method_getReturnType(found,result,sizeof(result));method_getArgumentType(found,2,arg,sizeof(arg));
    if(strcmp(result,"@") || strcmp(arg,"@"))return false;
    *original=(SweetLayoutFactory)method_getImplementation(found);if(!*original)return false;
    method_setImplementation(found,replacement);
    char line[190];snprintf(line,sizeof(line),"SWEET GEOGRID INSTALL selector=%s owned=1 ABI=@24@0:8@16\n",name);emit(line);return true;
}
static void install_native_grid_adapter(void)
{
    const char *enabled=getenv("GEO_FIX");if(!enabled || strcmp(enabled,"1"))return;
    if(!original_layout_factory && !install_native_layout_method("makeLayoutForIconLocation:",&original_layout_factory,(IMP)native_make_layout))
        emit("SWEET GEOGRID REFUSE original_factory_ABI_unavailable\n");
    if(!original_layout_cached && !install_native_layout_method("layoutForIconLocation:",&original_layout_cached,(IMP)native_cached_layout))
        emit("SWEET GEOGRID REFUSE original_cached_ABI_unavailable\n");
}
struct SweetGeoLayoutResult {unsigned visited,recorded,grids,refused,truncated,model_pages,model_icons,applications,write_failures;};
struct SweetGeoModelJob {id model,icons;unsigned number,parent,index,list,depth;const char *kind;char folder[65];};
struct SweetGeoModels {SweetGeoModelJob jobs[32];unsigned count,next=2049;char folder_ids[16][65];unsigned folders;};
static bool geo_ascii_identifier(id object,char *output,size_t capacity)
{
    id text=geo_kind(object,"NSString")?object:geo_kind(object,"NSUUID")?geo_object(object,"UUIDString"):nil;
    if(!text || !CFStringGetCString((CFStringRef)text,output,capacity,kCFStringEncodingUTF8) || !output[0])return false;
    for(const unsigned char *p=(const unsigned char *)output;*p;++p)
        if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='.' || *p=='_' || *p=='-'))return false;
    return true;
}
static bool geo_model_add(id model,unsigned number,const char *kind,unsigned parent,unsigned index,unsigned list,
                          const char *folder,unsigned depth,SweetGeoModels *models,SweetGeoLayoutResult *result)
{
    id icons=geo_kind(model,"SBIconListModel")?geo_object(model,"icons"):nil;
    if(!geo_kind(icons,"NSArray")){++result->refused;return false;}
    CFIndex count=CFArrayGetCount((CFArrayRef)icons);
    if(count<0 || count>128 || models->count>=32 || depth>8){result->truncated=1;return false;}
    for(unsigned i=0;i<models->count;++i)if(models->jobs[i].model==model){++result->refused;return false;}
    SweetGeoModelJob &job=models->jobs[models->count++];
    job.model=model;job.icons=icons;job.number=number;job.kind=kind;job.parent=parent;job.index=index;job.list=list;job.depth=depth;
    snprintf(job.folder,sizeof(job.folder),"%s",folder);return true;
}
static bool geo_model(id view,unsigned number,SweetGeoLayoutResult *result,SweetGeoModels *models)
{
    CGRect rectangle;
    if(!geo_kind(view,"SBIconListView") || !geo_rectangle(view,"frame",&rectangle) || fabs(rectangle.size.width-360)>.01)return false;
    const char *kind=fabs(rectangle.size.height-640)<.01?"root":geo_kind(view,"SBDockIconListView")?"dock":NULL;
    if(!kind)return false;
    if(!geo_model_add(geo_object(view,"model"),number,kind,0,0,0,"-",0,models,result)){
        char line[190];snprintf(line,sizeof(line),"SWEET GLAYOUT REFUSE model_unavailable n=%u\n",number);emit(line);return false;
    }
    return true;
}
static void geo_models_capture(SweetGeoModels *models,SweetGeoLayoutResult *result)
{
    char line[190];
    for(unsigned j=0;j<models->count;++j){
        SweetGeoModelJob &job=models->jobs[j];CFIndex count=CFArrayGetCount((CFArrayRef)job.icons);++result->model_pages;
        snprintf(line,sizeof(line),"SWEET GLAYOUT MODEL n=%u kind=%s parent=%u index=%u list=%u folder=%s count=%ld\n",job.number,job.kind,job.parent,job.index,job.list,job.folder,(long)count);emit(line);
        for(CFIndex i=0;i<count;++i){
            id icon=(id)CFArrayGetValueAtIndex((CFArrayRef)job.icons,i);const char *name=icon?class_getName(object_getClass(icon)):"nil";
            char identifier[65]="-",folder_id[65]="-";unsigned application=0,folder_lists=0;
            if(strlen(name)>48)result->truncated=1;
            if(geo_kind(icon,"SBLeafIcon")){
                id bundle=geo_object(icon,"applicationBundleID");
                if(bundle){
                    if(!geo_ascii_identifier(bundle,identifier,sizeof(identifier))){++result->refused;strcpy(identifier,"-");}
                    else{
                        const char *selector="leafIdentifierAndApplicationBundleIDMatches";
                        if(geo_method_type(icon,selector,"B"))application=((bool (*)(id,SEL))objc_msgSend)(icon,sel_registerName(selector))?1:0;
                        else if(geo_method_type(icon,selector,"c"))application=((signed char (*)(id,SEL))objc_msgSend)(icon,sel_registerName(selector))?1:0;
                        else ++result->refused;
                    }
                }
            }
            if(geo_kind(icon,"SBFolderIcon")){
                id folder=geo_object(icon,"folder"),lists=geo_kind(folder,"SBFolder")?geo_object(folder,"lists"):nil;
                if(!geo_kind(lists,"NSArray") || !geo_ascii_identifier(geo_object(folder,"uniqueIdentifier"),folder_id,sizeof(folder_id))){
                    ++result->refused;
                }else{
                    CFIndex length=CFArrayGetCount((CFArrayRef)lists);bool repeated=false;
                    for(unsigned k=0;k<models->folders;++k)if(!strcmp(folder_id,models->folder_ids[k]))repeated=true;
                    if(length<0 || length>8 || models->folders>=16 || repeated){++result->refused;}
                    else{
                        strcpy(models->folder_ids[models->folders++],folder_id);folder_lists=(unsigned)length;
                        for(CFIndex k=0;k<length;++k)
                            if(!geo_model_add((id)CFArrayGetValueAtIndex((CFArrayRef)lists,k),models->next++,"folder",job.number,(unsigned)i,(unsigned)k,folder_id,job.depth+1,models,result))++result->refused;
                    }
                }
            }
            ++result->model_icons;result->applications+=application;
            int written=snprintf(line,sizeof(line),"SWEET GLAYOUT ICON n=%u index=%ld class=%.48s application=%u id=%s\n",job.number,(long)i,name,application,identifier);
            if(written<0 || (size_t)written>=sizeof(line))result->truncated=1;emit(line);
            if(geo_kind(icon,"SBFolderIcon")){
                snprintf(line,sizeof(line),"SWEET GLAYOUT LINK n=%u index=%ld folder=%s lists=%u\n",job.number,(long)i,folder_id,folder_lists);emit(line);
            }
        }
    }
}
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
    SweetGeoLayoutResult result={};int saved_errno=errno;unsigned writes_before=__atomic_load_n(&geo_write_failures,__ATOMIC_RELAXED);
    const char *enabled=getenv("GEO_FIX");
    if(!enabled || strcmp(enabled,"1") || !geo_native_screen()){errno=saved_errno;return result;}
    int grid_mode=native_grid_mode();if(grid_mode<0){emit("SWEET GLAYOUT REFUSE grid_mode\n");return result;}
    SweetGeoModels models={};models.next=2049;
    char line[190];snprintf(line,sizeof(line),"SWEET GLAYOUT BEGIN rev=3 after_ctor=%u points=360,640 pixels=720,1280 scale=2 grid=%d\n",after_ctor_seconds,grid_mode);emit(line);
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
                (void)geo_model(view,i+1,&result,&models);
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
        geo_models_capture(&models,&result);
    }@catch(id exception){(void)exception;++result.refused;emit("SWEET GLAYOUT REFUSE getter_exception\n");}
    snprintf(line,sizeof(line),"SWEET GLAYOUT MODELS pages=%u icons=%u applications=%u\n",result.model_pages,result.model_icons,result.applications);emit(line);
    result.write_failures=__atomic_load_n(&geo_write_failures,__ATOMIC_RELAXED)-writes_before;
    snprintf(line,sizeof(line),"SWEET GLAYOUT END rev=3 visited=%u recorded=%u grids=%u refused=%u truncated=%u write_failures=%u\n",result.visited,result.recorded,result.grids,result.refused,result.truncated,result.write_failures);emit(line);
    errno=saved_errno;return result;
}
__attribute__((constructor)) static void start_geometry_reader(void)
{
    install_provider_adapter();
    install_native_grid_adapter();
    {char line[96];snprintf(line,sizeof(line),"SWEET GEOGRID MODE value=%d\n",native_grid_mode());emit(line);}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { inspect_geometry(); }
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,20*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { inspect_geometry(); }
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,120*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        @autoreleasepool { (void)inspect_native_layout(120); }
    });
}
