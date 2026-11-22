#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <assert.h>
#include <errno.h>
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
@interface _UIStatusBarVisualProvider_LegacyPhone:NSObject @end
@implementation _UIStatusBarVisualProvider_LegacyPhone @end
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
typedef struct UIEdgeInsets {double top,left,bottom,right;} UIEdgeInsets;
typedef struct SBIconImageInfo {CGSize size;double field2,field3;} SBIconImageInfo;
@interface UIView:NSObject
@property CGRect frame; @property CGRect bounds; @property CGPoint mockWindowOrigin;
@property(retain) NSArray *subviews;
-(CGRect)convertRect:(CGRect)value toView:(id)target;
@end
@implementation UIView
-(CGRect)convertRect:(CGRect)value toView:(id)target {
 assert(target==nil);value.origin.x+=self.mockWindowOrigin.x-self.bounds.origin.x;
 value.origin.y+=self.mockWindowOrigin.y-self.bounds.origin.y;return value;
}
@end
@interface UIWindow:UIView @end
@implementation UIWindow @end
@interface UILabel:UIView @end
@implementation UILabel @end
@interface SBIconListGridLayoutConfiguration:NSObject
@property BOOL nativeRootTest;
@property unsigned long long rows;
-(unsigned long long)numberOfPortraitRows;-(long long)numberOfPortraitColumns;
-(CGSize)listSizeForIconSpacingCalculation;-(UIEdgeInsets)portraitLayoutInsets;
-(SBIconImageInfo)iconImageInfo;-(void)setNumberOfPortraitRows:(unsigned long long)rows;
@end
@implementation SBIconListGridLayoutConfiguration
-(unsigned long long)numberOfPortraitRows {return self.rows?self.rows:6;}
-(void)setNumberOfPortraitRows:(unsigned long long)rows {self.rows=rows;}
-(long long)numberOfPortraitColumns {return 4;}
-(CGSize)listSizeForIconSpacingCalculation {return self.nativeRootTest?CGSizeMake(320,693):CGSizeMake(360,511);}
-(UIEdgeInsets)portraitLayoutInsets {return self.nativeRootTest?(UIEdgeInsets){57,19,155,19}:(UIEdgeInsets){14,19,47,19};}
-(SBIconImageInfo)iconImageInfo {return (SBIconImageInfo){CGSizeMake(56,56),0,0};}
@end
@interface SBIconListGridLayout:NSObject
@property(retain) SBIconListGridLayoutConfiguration *layoutConfiguration;
@end
@implementation SBIconListGridLayout @end
static id expected_location;
@interface SBHDefaultIconListLayoutProvider:NSObject
@property(retain) id offeredLayout;@property(retain) id cachedLayout;@property BOOL throwOriginal;
-(id)makeLayoutForIconLocation:(id)location;-(id)layoutForIconLocation:(id)location;
@end
@implementation SBHDefaultIconListLayoutProvider
-(id)makeLayoutForIconLocation:(id)location {
 assert(location==expected_location);if(self.throwOriginal)@throw [NSException exceptionWithName:@"original" reason:@"original" userInfo:nil];
 errno=EDOM;return self.offeredLayout;
}
-(id)layoutForIconLocation:(id)location {
 assert(location==expected_location);if(!self.cachedLayout)self.cachedLayout=[self makeLayoutForIconLocation:location];
 errno=EDOM;return self.cachedLayout;
}
@end
@interface SBLeafIcon:NSObject
@property(retain) NSString *applicationBundleID;
@property(retain) NSString *leafIdentifier;
-(BOOL)leafIdentifierAndApplicationBundleIDMatches;
@end
@implementation SBLeafIcon
-(BOOL)leafIdentifierAndApplicationBundleIDMatches {return self.applicationBundleID && [self.leafIdentifier isEqualToString:self.applicationBundleID];}
@end
@interface SBApplicationIcon:SBLeafIcon @end
@implementation SBApplicationIcon @end
@interface SBWidgetIcon:SBLeafIcon @end
@implementation SBWidgetIcon @end
@interface SBIconListModel:NSObject
@property(retain) NSArray *icons;
@end
@implementation SBIconListModel @end
@interface SBFolder:NSObject
@property(retain) NSArray *lists;
@property(retain) NSString *uniqueIdentifier;
@end
@implementation SBFolder @end
@interface SBFolderIcon:NSObject
@property(retain) SBFolder *folder;
@end
@implementation SBFolderIcon @end
@interface SBIconListView:UIView
@property(retain) SBIconListGridLayout *layout;
@property(retain) SBIconListModel *model;
-(CGSize)iconSpacing;
@end
@implementation SBIconListView
-(CGSize)iconSpacing {return CGSizeMake(88.5,74.5);}
@end
@interface SBDockIconListView:SBIconListView @end
@implementation SBDockIconListView @end
@interface UIApplication:NSObject
@property(retain) NSArray *windows;
+(UIApplication *)sharedApplication;
@end
@implementation UIApplication
+(UIApplication *)sharedApplication {static UIApplication *app=nil;if(!app)app=[UIApplication new];return app;}
@end
static unsigned wrong_rectangle_calls,wrong_count_calls;
@interface SweetGeoWrongRectangle:NSObject -(CGSize)frame; @end
@implementation SweetGeoWrongRectangle
-(CGSize)frame {++wrong_rectangle_calls;return CGSizeMake(1,2);}
@end
@interface SweetGeoWrongCount:NSObject -(double)numberOfPortraitRows; @end
@implementation SweetGeoWrongCount
-(double)numberOfPortraitRows {++wrong_count_calls;return 6;}
@end
#include "geocompat.mm"
static unsigned char log_capture[64];static size_t log_used;static unsigned log_calls,log_mode;
static ssize_t log_fault_writer(int fd,const void *bytes,size_t count)
{
 assert(fd==17);++log_calls;
 if(log_mode==1 && log_calls<=3 || log_mode==2 && log_calls%2 || log_mode==5){errno=EINTR;return -1;}
 if(log_mode==3)return 0;if(log_mode==4){errno=EACCES;return -1;}if(log_mode==6)return count+1;
 size_t take=log_mode==2 && count>3?3:count;assert(log_used+take<=sizeof(log_capture));memcpy(log_capture+log_used,bytes,take);log_used+=take;return take;
}
int main(void){ @autoreleasepool {
 unsigned char original_bytes[37];for(unsigned i=0;i<37;++i)original_bytes[i]=(unsigned char)(i*7);
 for(log_mode=0;log_mode<3;++log_mode){log_used=log_calls=0;memset(log_capture,0xa5,sizeof(log_capture));errno=EDOM;
  assert(!sweet_geo_log_write_remaining(17,original_bytes,37,log_fault_writer) && errno==EDOM && log_used==37 && !memcmp(original_bytes,log_capture,37));
  for(unsigned i=37;i<sizeof(log_capture);++i)assert(log_capture[i]==0xa5);}
 log_mode=3;errno=EDOM;assert(sweet_geo_log_write_remaining(17,original_bytes,37,log_fault_writer)==-EIO && errno==EDOM);
 log_mode=4;assert(sweet_geo_log_write_remaining(17,original_bytes,37,log_fault_writer)==-EACCES && errno==EDOM);
 log_mode=5;log_calls=0;assert(sweet_geo_log_write_remaining(17,original_bytes,37,log_fault_writer)==-EINTR && log_calls==65 && errno==EDOM);
 log_mode=6;assert(sweet_geo_log_write_remaining(17,original_bytes,37,log_fault_writer)==-EIO);
 assert(sweet_geo_log_write_remaining(17,NULL,1,log_fault_writer)==-EINVAL && sweet_geo_log_write_remaining(17,original_bytes,1,NULL)==-EINVAL);
 puts("PASS: real shared C log writer; partial/EINTR/bounds/errno verified");
 UIScreen *s=[UIScreen mainScreen];s.bounds=CGRectMake(0,0,375,812);s.nativeBounds=CGRectMake(0,0,750,1624);s.scale=s.nativeScale=2;
 expected_info=[NSObject new];
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Fallback class]);
 setenv("GEO_FIX","1",1);install_provider_adapter();
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Split1125 class]);
 assert(s.scale==2 && s.bounds.size.width==375 && s.nativeBounds.size.width==750);
 s.bounds=CGRectMake(0,0,360,640);s.nativeBounds=CGRectMake(0,0,720,1280);
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_LegacyPhone class]);
 assert(s.scale==2 && s.bounds.size.width==360 && s.nativeBounds.size.height==1280);
 s.bounds=CGRectMake(0,0,375,812);s.nativeBounds=CGRectMake(0,0,750,1624);
 s.scale=3;s.nativeBounds=CGRectMake(0,0,1125,2436);
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Split1125 class]);
 s.scale=2;s.bounds=CGRectMake(0,0,320,568);s.nativeBounds=CGRectMake(0,0,640,1136);
 assert([_UIStatusBarVisualProvider_iOS visualProviderSubclassForScreen:s visualProviderInfo:expected_info]==[_UIStatusBarVisualProvider_Fallback class]);
 CGRect rect=CGRectZero;unsigned long long number=0;
 assert(!geo_rectangle([SweetGeoWrongRectangle new],"frame",&rect) && wrong_rectangle_calls==0);
 assert(!geo_count([SweetGeoWrongCount new],"numberOfPortraitRows",&number) && wrong_count_calls==0);
 UIWindow *window=[UIWindow new];window.frame=window.bounds=CGRectMake(0,0,360,640);window.subviews=@[];
 SBIconListView *list=[SBIconListView new];list.frame=CGRectMake(0,0,360,640);list.bounds=CGRectMake(0,0,360,640);list.mockWindowOrigin=CGPointZero;
 SBIconListGridLayout *layout=[SBIconListGridLayout new];layout.layoutConfiguration=[SBIconListGridLayoutConfiguration new];list.layout=layout;
 UILabel *label=[UILabel new];label.frame=CGRectMake(0,148,60,14);label.bounds=CGRectMake(0,0,60,14);label.mockWindowOrigin=CGPointMake(34,200);label.subviews=@[];
 list.subviews=@[label];window.subviews=@[list];[UIApplication sharedApplication].windows=@[window];
 SBApplicationIcon *app1=[SBApplicationIcon new],*app2=[SBApplicationIcon new];app1.applicationBundleID=@"com.apple.Preferences";app2.applicationBundleID=@"com.apple.AppStore";
 app1.leafIdentifier=app1.applicationBundleID;app2.leafIdentifier=app2.applicationBundleID;
 SBWidgetIcon *widget=[SBWidgetIcon new];widget.applicationBundleID=@"com.apple.Preferences";
 widget.leafIdentifier=@"widget:original";
 SBApplicationIcon *folder_app=[SBApplicationIcon new];folder_app.applicationBundleID=@"com.apple.mobiletimer";folder_app.leafIdentifier=folder_app.applicationBundleID;
 SBFolder *folder_model=[SBFolder new];folder_model.uniqueIdentifier=@"original-folder";
 SBIconListModel *folder_list=[SBIconListModel new];folder_list.icons=@[folder_app];folder_model.lists=@[folder_list];
 SBFolderIcon *folder_icon=[SBFolderIcon new];folder_icon.folder=folder_model;
 list.model=[SBIconListModel new];list.model.icons=@[app1,widget,app2,folder_icon];NSArray *original_icons=list.model.icons;
 SBDockIconListView *dock=[SBDockIconListView new];dock.frame=dock.bounds=CGRectMake(0,0,360,80);dock.subviews=@[];
 dock.layout=[SBIconListGridLayout new];dock.layout.layoutConfiguration=[SBIconListGridLayoutConfiguration new];dock.layout.layoutConfiguration.rows=1;
 SBApplicationIcon *dock_app=[SBApplicationIcon new];dock_app.applicationBundleID=@"com.apple.mobilephone";dock_app.leafIdentifier=dock_app.applicationBundleID;
 dock.model=[SBIconListModel new];dock.model.icons=@[dock_app];window.subviews=@[list,dock];
 assert(geo_window_rectangle(label,label.bounds,&rect) && CGRectEqualToRect(rect,CGRectMake(34,200,60,14)));
 SweetGeoLayoutResult empty=inspect_native_layout(0);assert(empty.visited==0);
 s.bounds=CGRectMake(0,0,360,640);s.nativeBounds=CGRectMake(0,0,720,1280);s.scale=2;
 int before=EDOM;errno=before;CGRect originalFrame=list.frame;
 SweetGeoLayoutResult observed=inspect_native_layout(0);
 assert(observed.visited==4 && observed.recorded==4 && observed.grids==2 && observed.refused==0 && observed.truncated==0);
 assert(observed.model_pages==3 && observed.model_icons==6 && observed.applications==4 && observed.write_failures==0 && list.model.icons==original_icons);
 assert(errno==before && CGRectEqualToRect(originalFrame,list.frame));
 window.subviews=@[window];SweetGeoLayoutResult cycle=inspect_native_layout(0);assert(cycle.visited==1 && cycle.refused==1);
 NSMutableArray *many=[NSMutableArray array];for(unsigned i=0;i<2049;++i)[many addObject:label];window.subviews=many;
 SweetGeoLayoutResult bounded=inspect_native_layout(0);assert(bounded.visited==1 && bounded.truncated==1);
 puts("PASS: bounded original layout observer; CGRect window conversion, Q/q grid ABI, invalid getters refused, native gate, cycle and limit");
 SweetGeoLayoutResult model_failure={};SweetGeoModels failed_models={};id original_model=list.model;list.model=(id)[NSObject new];
 assert(!geo_model(list,1,&model_failure,&failed_models) && model_failure.refused==1);list.model=original_model;
 NSMutableArray *model_limit=[NSMutableArray array];for(unsigned i=0;i<129;++i)[model_limit addObject:app1];list.model.icons=model_limit;SweetGeoLayoutResult limited={};
 SweetGeoModels limited_models={};assert(!geo_model(list,1,&limited,&limited_models) && limited.truncated==1);list.model.icons=original_icons;
 SweetGeoModels cycle_models={};SweetGeoLayoutResult folder_cycle={};folder_model.lists=@[list.model];
 assert(geo_model(list,1,&folder_cycle,&cycle_models));geo_models_capture(&cycle_models,&folder_cycle);assert(folder_cycle.refused>0);folder_model.lists=@[folder_list];
 puts("PASS: original model identities; app/widget separation, unavailable model and icon-count bound");
 expected_location=[NSObject new];SBHDefaultIconListLayoutProvider *provider=[SBHDefaultIconListLayoutProvider new];
 SBIconListGridLayout *rootLayout=[SBIconListGridLayout new];SBIconListGridLayoutConfiguration *rootConfig=[SBIconListGridLayoutConfiguration new];
 rootConfig.nativeRootTest=YES;rootLayout.layoutConfiguration=rootConfig;provider.offeredLayout=rootLayout;
 assert([provider layoutForIconLocation:expected_location]==rootLayout && [rootConfig numberOfPortraitRows]*[rootConfig numberOfPortraitColumns]==24);
 install_native_grid_adapter();
 assert([provider layoutForIconLocation:expected_location]==rootLayout && [rootConfig numberOfPortraitRows]*[rootConfig numberOfPortraitColumns]==20 && errno==EDOM);
 assert([rootConfig numberOfPortraitColumns]==4 && [rootConfig portraitLayoutInsets].top==57 && [rootConfig listSizeForIconSpacingCalculation].height==693);
 assert([provider layoutForIconLocation:expected_location]==rootLayout && [rootConfig numberOfPortraitRows]==5);
 SBIconListGridLayoutConfiguration *fresh=[SBIconListGridLayoutConfiguration new];fresh.nativeRootTest=YES;rootLayout.layoutConfiguration=fresh;
 assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==5);
 fresh.rows=6;s.bounds=CGRectMake(0,0,375,812);s.nativeBounds=CGRectMake(0,0,750,1624);
 assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==6);
 s.bounds=CGRectMake(0,0,360,640);s.nativeBounds=CGRectMake(0,0,720,1280);fresh.rows=1;
 assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==1);
 fresh.rows=6;fresh.nativeRootTest=NO;
 assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==6);
 fresh.nativeRootTest=YES;fresh.rows=6;setenv("GEO_GRID","0",1);
 assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==6);
 setenv("GEO_GRID","1",1);assert([provider makeLayoutForIconLocation:expected_location]==rootLayout && [fresh numberOfPortraitRows]==5);unsetenv("GEO_GRID");
 provider.offeredLayout=nil;assert([provider makeLayoutForIconLocation:expected_location]==nil);
 provider.throwOriginal=YES;BOOL thrown=NO;@try{(void)[provider makeLayoutForIconLocation:expected_location];}@catch(NSException *exception){thrown=[exception.name isEqualToString:@"original"];}
 assert(thrown);
 puts("PASS: native root grid factory/cache real ObjC ABI; original setter, capacity, opaque location, errno/exception and other profiles preserved");
 puts("PASS: full root/folder/dock model graph; original identity, nested lists, cycles and density control");
 puts("PASS: real 4-argument ObjC dispatch; opaque info forwarded; other screens unchanged");
} }
