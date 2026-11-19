/* Exact typed ObjC forwarding, including nil/error/throw and the record bound. */
#import <Foundation/Foundation.h>
#include "../../gpu/diagnostics/MetalRequestObserve.h"
#include <assert.h>
#include <stdarg.h>
static id argument,constants_value,answer;static NSError **expected_error;static NSError *answer_error;
static NSException *answer_exception;static bool throws;static unsigned originals,begin_count,end_count;
static CGSize expected_size;
static CGRect expected_frame;static NSUInteger expected_options;static NSInteger expected_color;static id expected_queue;
static void original_begin(void){assert(errno==73);++originals;errno=91;if(throws)@throw answer_exception;}
@interface RequestLibraryHost:NSObject
- (id)newFunctionWithName:(id)name;
- (id)newFunctionWithName:(id)name constantValues:(id)values error:(NSError **)error;
- (id)functionNames;
@end
@implementation RequestLibraryHost
- (id)newFunctionWithName:(id)name{assert(name==argument);original_begin();return answer;}
- (id)newFunctionWithName:(id)name constantValues:(id)values error:(NSError **)error
{assert(name==argument && values==constants_value && error==expected_error);original_begin();if(error)*error=answer_error;return answer;}
- (id)functionNames{original_begin();return answer;}
@end
@interface RequestLayerHost:NSObject
- (id)nextDrawable;
- (void)setDevice:(id)device;
- (void)setDrawableSize:(CGSize)size;
@end
@implementation RequestLayerHost
- (id)nextDrawable{original_begin();return answer;}
- (void)setDevice:(id)device{assert(device==argument);original_begin();}
- (void)setDrawableSize:(CGSize)size{assert(!memcmp(&size,&expected_size,sizeof(size)));original_begin();}
@end
@interface RequestQuadHost:NSObject
- (id)initWithFrame:(CGRect)frame identifier:(id)identifier options:(NSUInteger)options colorSpace:(NSInteger)color;
- (id)initWithFrame:(CGRect)frame identifier:(id)identifier options:(NSUInteger)options colorSpace:(NSInteger)color asyncRenderQueue:(id)queue;
- (void)layoutSubviews;
- (BOOL)_displayAndCheckForDrawable:(BOOL)flag withCompletion:(void (^)(void))completion;
- (BOOL)prewarmWithCompletion:(void (^)(void))completion;
- (CGSize)drawableSize;
- (void)_updateDrawableSizeIfNecessary;
@end
@implementation RequestQuadHost
- (id)initWithFrame:(CGRect)frame identifier:(id)identifier options:(NSUInteger)options colorSpace:(NSInteger)color
{assert(!memcmp(&frame,&expected_frame,sizeof(frame)) && identifier==argument && options==expected_options && color==expected_color);original_begin();return answer;}
- (id)initWithFrame:(CGRect)frame identifier:(id)identifier options:(NSUInteger)options colorSpace:(NSInteger)color asyncRenderQueue:(id)queue
{assert(!memcmp(&frame,&expected_frame,sizeof(frame)) && identifier==argument && options==expected_options && color==expected_color && queue==expected_queue);original_begin();return answer;}
- (void)layoutSubviews{original_begin();}
- (BOOL)_displayAndCheckForDrawable:(BOOL)flag withCompletion:(void (^)(void))completion
{assert(flag==YES && (id)completion==argument);original_begin();return YES;}
- (BOOL)prewarmWithCompletion:(void (^)(void))completion
{assert((id)completion==argument);original_begin();return NO;}
- (CGSize)drawableSize{original_begin();return expected_size;}
- (void)_updateDrawableSizeIfNecessary{original_begin();}
@end
@interface RequestInheritedHost:RequestLayerHost @end
@implementation RequestInheritedHost @end
@interface RequestWrongABIHost:NSObject
- (id)nextDrawable:(id)extra;
@end
@implementation RequestWrongABIHost
- (id)nextDrawable:(id)extra{(void)extra;assert(!"wrong ABI must not be called");return nil;}
@end
static void emit(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>0 && n<190);if(strstr(line,"SWEET METAL REQUEST BEGIN "))++begin_count;
    if(strstr(line,"SWEET METAL REQUEST END "))++end_count;errno=117;
}
int main(void){@autoreleasepool{
    argument=[NSObject new];constants_value=[NSObject new];answer=[NSObject new];answer_error=[NSError errorWithDomain:@"fixture" code:37 userInfo:nil];
    answer_exception=[NSException exceptionWithName:@"RequestHost" reason:@"same exception" userInfo:nil];expected_size=CGSizeMake(750.5,1624.25);
    expected_frame=CGRectMake(1.25,2.5,375.5,812.25);expected_options=UINT64_C(0x8000000100000001);expected_color=-7;expected_queue=constants_value;
    errno=73;assert(metal_request_observe([RequestLibraryHost class],[RequestLayerHost class],[RequestQuadHost class],Nil,emit)==13);assert(errno==73);
    RequestLibraryHost *library=[RequestLibraryHost new];RequestLayerHost *layer=[RequestLayerHost new];
    RequestQuadHost *quad=[RequestQuadHost new];
    for(unsigned pass=0;pass<3;++pass){id saved=answer;if(pass==1)answer=nil;throws=pass==2;
        for(unsigned api=0;api<13;++api){NSError *error=nil;expected_error=pass==1?NULL:&error;errno=73;id returned=nil;bool caught=false;
            @try{switch(api){case 0:returned=[library newFunctionWithName:argument];break;
            case 1:returned=[library newFunctionWithName:argument constantValues:constants_value error:expected_error];break;
            case 2:returned=[library functionNames];break;case 3:returned=[layer nextDrawable];break;
            case 4:[layer setDevice:argument];break;case 5:[layer setDrawableSize:expected_size];break;
            case 6:returned=[quad initWithFrame:expected_frame identifier:argument options:expected_options colorSpace:expected_color];break;
            case 7:returned=[quad initWithFrame:expected_frame identifier:argument options:expected_options colorSpace:expected_color asyncRenderQueue:expected_queue];break;
            case 8:[quad layoutSubviews];break;case 9:assert([quad _displayAndCheckForDrawable:YES withCompletion:(id)argument]);break;
            case 10:assert(![quad prewarmWithCompletion:(id)argument]);break;
            case 11:{CGSize size=[quad drawableSize];assert(!memcmp(&size,&expected_size,sizeof(size)));break;}
            default:[quad _updateDrawableSizeIfNecessary];break;}}
            @catch(id exception){assert(exception==answer_exception);caught=true;}
            assert(caught==throws && errno==91);if(!throws && (api<4 || api==6 || api==7))assert(returned==answer);
            if(!throws && api==1 && expected_error)assert(error==answer_error);
        }answer=saved;
    }
    throws=false;for(unsigned i=0;i<70;++i){errno=73;assert([layer nextDrawable]==answer && errno==91);}
    assert(originals==109 && begin_count==64 && end_count==64);
    IMP unused=NULL;assert(!request_install([RequestInheritedHost class],"nextDrawable","inherited",(IMP)request_drawable,&unused,@encode(id),NULL,0));
    assert(!request_install([RequestLayerHost class],"nextDrawable","repeat",(IMP)request_drawable,&unused,@encode(id),NULL,0));
    assert(!request_install([RequestWrongABIHost class],"nextDrawable","absent",(IMP)request_drawable,&unused,@encode(id),NULL,0));
    [library release];[layer release];[quad release];[argument release];[constants_value release];[answer release];
    puts("Metal request observer PASS: original arguments/returns/errors/throws/errno, thirteen APIs, short records, owned ABI rejection");
}return 0;}
