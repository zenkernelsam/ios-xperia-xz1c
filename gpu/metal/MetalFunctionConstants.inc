// SPDX-License-Identifier: BSD-2-Clause
// Native snapshot conventions adapted from pinned ios_sagit QcomMTL.mm;
// use verified methods on original Metal objects, never their private ivars.
@protocol SweetNativeConstantData <NSObject>
- (NSUInteger)dataType;
- (void *)data;
@end
@protocol SweetNativeIndexedConstantData <SweetNativeConstantData>
- (NSUInteger)index;
@end
@protocol SweetNativeNamedConstantData <SweetNativeConstantData>
- (NSString *)name;
@end
@interface MTLFunctionConstantValues (SweetSnapshot)
- (NSArray *)newIndexedConstantArray;
- (NSArray *)newNamedConstantArray;
@end
@interface MTLFunctionConstant (SweetReflection)
- (instancetype)initWithName:(NSString *)name type:(MTLDataType)type index:(NSUInteger)index required:(BOOL)required;
@end
static bool native_constant_abi(void)
{
    const char *methods[][3]={
        {"MTLFunctionConstantValuesInternal","newIndexedConstantArray","@16@0:8"},
        {"MTLFunctionConstantValuesInternal","newNamedConstantArray","@16@0:8"},
        {"MTLIndexedConstantValue","dataType","Q16@0:8"},{"MTLIndexedConstantValue","index","Q16@0:8"},
        {"MTLIndexedConstantValue","data","^v16@0:8"},{"MTLNamedConstantValue","dataType","Q16@0:8"},
        {"MTLNamedConstantValue","name","@16@0:8"},{"MTLNamedConstantValue","data","^v16@0:8"},
        {"MTLFunctionConstantInternal","initWithName:type:index:required:","@44@0:8@16Q24Q32B40"}};
    for(const auto &row:methods){Class cls=objc_lookUpClass(row[0]);Method method=cls?class_getInstanceMethod(cls,sel_registerName(row[1])):nullptr;
        const char *expected=row[2];
#if defined(SWEET_METAL_HOST_TEST) && defined(__x86_64__)
        if(!strcmp(row[0],"MTLFunctionConstantInternal"))expected="@44@0:8@16Q24Q32c40";
#endif
        if(!method || strcmp(method_getTypeEncoding(method),expected)){
#if defined(SWEET_METAL_HOST_TEST)
            fprintf(stderr,"constant ABI class=%s selector=%s actual=%s expected=%s\n",row[0],row[1],method?method_getTypeEncoding(method):"missing",row[2]);
#endif
            return false;}}
    return true;
}
static bool snapshot_constants(MTLFunctionConstantValues *values,std::vector<GPU::Compiler::FunctionConstantValue> &out,std::string &error)
{
    out.clear();if(!values)return true;
    if(!native_constant_abi() || ![values isKindOfClass:[MTLFunctionConstantValues class]]){error="Unsupported native Metal constant snapshot ABI";return false;}
    MTLFunctionConstantValues *copy=[values copy];NSArray *indexed=[copy newIndexedConstantArray],*named=[copy newNamedConstantArray];
    bool valid=[indexed isKindOfClass:[NSArray class]] && [named isKindOfClass:[NSArray class]] && indexed.count<=GPU::Compiler::MaxConstantValues &&
        named.count<=GPU::Compiler::MaxConstantValues-indexed.count;
    for(unsigned group=0;group<2 && valid;++group)for(id object in group?named:indexed){
        Class cls=objc_lookUpClass(group?"MTLNamedConstantValue":"MTLIndexedConstantValue");
        auto constant=(id<SweetNativeConstantData>)object;
        if(!cls || ![object isKindOfClass:cls]){valid=false;break;}
        NSUInteger type=constant.dataType;unsigned size=type<=UINT32_MAX?GPU::Compiler::constantBytes(uint32_t(type)):0;
        if(!size || size>32 || !constant.data){valid=false;break;}
        GPU::Compiler::FunctionConstantValue value;value.type=(uint32_t)type;
        if(group){NSString *name=[(id<SweetNativeNamedConstantData>)object name];if(!name.length || !name.UTF8String || name.length>1024){valid=false;break;}value.name=name.UTF8String;}
        else{NSUInteger index=[(id<SweetNativeIndexedConstantData>)object index];if(index>65535){valid=false;break;}value.index=(uint32_t)index;}
        memcpy(value.bytes.data(),constant.data,size);out.push_back(std::move(value));
    }
    [indexed release];[named release];[copy release];
    if(!valid){out.clear();error="Invalid bounded native function constant value";}return valid;
}
static NSDictionary *constant_reflection(const std::vector<GPU::Compiler::FunctionConstant> &constants,std::string &error)
{
    if(constants.empty())return @{};
    if(!native_constant_abi()){error="Unsupported native function constant reflection ABI";return nil;}
    Class cls=objc_lookUpClass("MTLFunctionConstantInternal");NSMutableDictionary *info=[NSMutableDictionary new];bool valid=true;
    for(const auto &constant:constants){NSString *name=[[NSString alloc] initWithBytes:constant.name.data() length:constant.name.size() encoding:NSUTF8StringEncoding];
        auto reflected=name?[(MTLFunctionConstant *)[cls alloc] initWithName:name type:(MTLDataType)constant.type index:constant.index required:constant.required]:nil;
        if(!reflected){valid=false;[name release];break;}info[name]=reflected;[name release];[reflected release];}
    NSDictionary *result=valid?[[info copy] autorelease]:nil;[info release];if(!valid)error="Invalid function constant reflection name";return result;
}
