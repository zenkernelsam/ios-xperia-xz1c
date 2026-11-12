#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <assert.h>
@interface UIScreen:NSObject
@property CGRect bounds; @property CGRect nativeBounds;
@property CGFloat scale; @property CGFloat nativeScale;
+(UIScreen *)mainScreen;
@end
@implementation UIScreen
+(UIScreen *)mainScreen { static UIScreen *s=nil; if(!s)s=[UIScreen new];return s; }
@end
@interface _UIStatusBarVisualProvider_Fallback:NSObject @end
@implementation _UIStatusBarVisualProvider_Fallback @end
@interface _UIStatusBarVisualProvider_Split1125:NSObject @end
@implementation _UIStatusBarVisualProvider_Split1125 @end
static id expected_info;
@interface _UIStatusBarVisualProvider_Split:NSObject
+(Class)visualProviderSubclassForScreen:(UIScreen *)screen visualProviderInfo:(id)info;
@end
@implementation _UIStatusBarVisualProvider_Split
+(Class)visualProviderSubclassForScreen:(UIScreen *)screen visualProviderInfo:(id)info {
 assert(info==expected_info);
 return screen.scale==3?[_UIStatusBarVisualProvider_Split1125 class]:[_UIStatusBarVisualProvider_Fallback class];
}
@end
@interface _UIStatusBarVisualProvider_iOS:NSObject
+(Class)visualProviderSubclassForScreen:(UIScreen *)screen visualProviderInfo:(id)info;
@end
@implementation _UIStatusBarVisualProvider_iOS
+(Class)visualProviderSubclassForScreen:(UIScreen *)screen visualProviderInfo:(id)info {
 return [_UIStatusBarVisualProvider_Split visualProviderSubclassForScreen:screen visualProviderInfo:info];
}
@end
#include "geocompat.mm"
int main(void){ @autoreleasepool {
 UIScreen *s=[UIScreen mainScreen];s.bounds=CGRectMake(0,0,375,812);s.nativeBounds=CGRectMake(0,0,750,1624);s.scale=s.nativeScale=2;
 expected_info=[NSObject new];
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Fallback class]);
 setenv("GEO_FIX","1",1);install_provider_adapter();
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Split1125 class]);
 assert(s.scale==2 && s.bounds.size.width==375 && s.nativeBounds.size.width==750);
 s.scale=3;s.nativeBounds=CGRectMake(0,0,1125,2436);
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Split1125 class]);
 s.scale=2;s.bounds=CGRectMake(0,0,320,568);s.nativeBounds=CGRectMake(0,0,640,1136);
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Fallback class]);
 puts("PASS: real 4-argument ObjC dispatch; opaque info forwarded; other screens unchanged");
} }
