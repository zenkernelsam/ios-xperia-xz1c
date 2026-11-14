// SPDX-License-Identifier: BSD-2-Clause
// Original AIR parsing/lowering through the pinned OSS frontend and Mesa FD540.
#include "air/Compiler.h"
#include "air/ESSLTarget.h"
#include "../sweetgl/MetalShaderAPI.h"
#include "air/reference/LibraryArchive.h"
#include <CommonCrypto/CommonDigest.h>
#include <vector>
#include <map>
#include "MetalFunctionConstants.inc"
@class SweetMTLFunction,SweetMTLComputePipeline,SweetMTLComputeEncoder;
@interface SweetMTLLibrary : NSObject <MTLLibrary> { @public SweetMTLDevice *_owner;NSData *_bytes;NSArray *_names;NSDictionary *_entries;NSString *_label; }
- (instancetype)initWithOwner:(SweetMTLDevice *)owner bytes:(NSData *)bytes error:(NSError **)error;
@end
@interface SweetMTLFunction : NSObject <MTLFunction> {
@public SweetMTLLibrary *_library;NSString *_name;NSUInteger _stage;NSDictionary *_constantInfo;
    std::vector<GPU::Compiler::FunctionConstantValue> _constants;
}
- (instancetype)initWithLibrary:(SweetMTLLibrary *)library name:(NSString *)name;
- (instancetype)initWithLibrary:(SweetMTLLibrary *)library name:(NSString *)name constantValues:(MTLFunctionConstantValues *)values error:(NSError **)error;
@end
@interface SweetMTLComputePipeline : NSObject <MTLComputePipelineState> {
@public SweetMTLFunction *_function;NSLock *_programLock;std::map<std::array<unsigned,3>,SweetMetalProgram *> _programs;
}
- (instancetype)initWithFunction:(SweetMTLFunction *)function;
- (SweetMetalProgram *)programForLocal:(MTLSize)local error:(NSError **)error;
@end
@interface SweetMTLComputeEncoder : NSObject <MTLComputeCommandEncoder> {
    SweetMTLCommandBuffer *_commands;SweetMTLComputePipeline *_pipeline;NSMutableDictionary *_bindings;BOOL _ended;NSString *_label;
}
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands;
@end
static NSError *metal_compile_error(const std::string &error)
{return [NSError errorWithDomain:MTLLibraryErrorDomain code:MTLLibraryErrorCompileFailure
    userInfo:@{NSLocalizedDescriptionKey:[NSString stringWithUTF8String:error.c_str()]?:@"Unknown AIR compiler error"}];}

@implementation SweetMTLLibrary
- (instancetype)initWithOwner:(SweetMTLDevice *)owner bytes:(NSData *)bytes error:(NSError **)error
{
    if(!(self=[super init]))return nil;
    std::vector<std::string> names;std::string diagnostic;NSMutableDictionary *entries=[NSMutableDictionary new];bool valid=owner && bytes && bytes.length && bytes.length<=16u*1024*1024;
    if(valid && GPU::Compiler::LibraryArchive::container(bytes.bytes,bytes.length)){
        std::vector<GPU::Compiler::LibraryArchive::Function> records;
        valid=GPU::Compiler::LibraryArchive::parse(bytes.bytes,bytes.length,records,diagnostic);
        for(const auto &record:records){if(!valid)break;
            if(record.type>2){valid=false;diagnostic="MTLB invalid function stage";break;}
            const auto *source=(const unsigned char *)bytes.bytes+record.offset;unsigned char hash[32];
            CC_SHA256(source,(CC_LONG)record.size,hash);
            if(memcmp(hash,record.hash.data(),32)){valid=false;diagnostic="MTLB AIR slice SHA differs";break;}
            std::vector<std::string> actual;valid=GPU::Compiler::inspectAIREntries(source,record.size,actual,diagnostic);
            if(!valid || std::find(actual.begin(),actual.end(),record.name)==actual.end()){valid=false;diagnostic="MTLB named function is absent from its AIR slice";break;}
            std::vector<GPU::Compiler::FunctionConstant> constants;unsigned stage=UINT32_MAX;
            if(!GPU::Compiler::inspectAIRConstants(source,record.size,record.name,constants,stage,diagnostic) ||
                stage!=(record.type==2?0:record.type==0?1:2)){valid=false;diagnostic="MTLB declared stage differs from actual AIR metadata";break;}
            NSString *name=[NSString stringWithUTF8String:record.name.c_str()];if(!name){valid=false;break;}
            entries[name]=[NSData dataWithBytes:source length:record.size];names.push_back(record.name);
        }
    }else if(valid){
        valid=GPU::Compiler::inspectAIREntries(bytes.bytes,bytes.length,names,diagnostic);
        for(const auto &name:names){NSString *key=[NSString stringWithUTF8String:name.c_str()];if(!key){valid=false;break;}entries[key]=bytes;}
    }
    if(!valid){[entries release];
        if(error)*error=metal_compile_error(diagnostic);[self release];return nil;}
    _owner=[owner retain];_bytes=[bytes copy];NSMutableArray *array=[NSMutableArray new];
    for(const auto &name:names){NSString *string=[NSString stringWithUTF8String:name.c_str()];if(!string){[entries release];[array release];[self release];return nil;}[array addObject:string];}
    _names=[array copy];_entries=[entries copy];[entries release];[array release];return self;
}
- (void)dealloc{[_owner release];[_bytes release];[_names release];[_entries release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _owner;}
- (NSArray *)functionNames{return _names;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (id<MTLFunction>)newFunctionWithName:(NSString *)name
{return name && [_names containsObject:name]?[[SweetMTLFunction alloc] initWithLibrary:self name:name]:nil;}
- (id<MTLFunction>)newFunctionWithName:(NSString *)name constantValues:(MTLFunctionConstantValues *)values error:(NSError **)error
{
    if(error)*error=nil;if(!name || ![_names containsObject:name]){if(error)*error=metal_compile_error("Named AIR function absent");return nil;}
    return [[SweetMTLFunction alloc] initWithLibrary:self name:name constantValues:values error:error];
}
@end
@implementation SweetMTLFunction
- (instancetype)initWithLibrary:(SweetMTLLibrary *)library name:(NSString *)name
{return [self initWithLibrary:library name:name constantValues:nil error:nullptr];}
- (instancetype)initWithLibrary:(SweetMTLLibrary *)library name:(NSString *)name constantValues:(MTLFunctionConstantValues *)values error:(NSError **)reported
{
    if(!(self=[super init]))return nil;
    std::vector<GPU::Compiler::FunctionConstant> constants;unsigned stage=UINT32_MAX;std::string error;
    NSData *source=library->_entries[name];std::vector<GPU::Compiler::FunctionConstantValue> input;
    bool valid=source && GPU::Compiler::inspectAIRConstants(source.bytes,source.length,name.UTF8String,constants,stage,error) &&
        snapshot_constants(values,input,error) && GPU::Compiler::resolveFunctionConstants(constants,input,_constants,error);
    NSDictionary *reflection=valid?constant_reflection(constants,error):nil;
    if(!valid || !reflection){if(reported)*reported=metal_compile_error(error);[self release];return nil;}
    _library=[library retain];_name=[name copy];_stage=stage;_constantInfo=[reflection copy];return self;
}
- (void)dealloc{[_library release];[_name release];[_constantInfo release];[super dealloc];}
- (id<MTLDevice>)device{return _library.device;}
- (NSString *)name{return _name;}
- (MTLFunctionType)functionType{return _stage==0?MTLFunctionTypeKernel:_stage==1?MTLFunctionTypeVertex:MTLFunctionTypeFragment;}
- (NSDictionary *)functionConstantsDictionary{return _constantInfo;}
- (NSArray *)vertexAttributes{return @[];}
- (NSArray *)stageInputAttributes{return @[];}
- (MTLPatchType)patchType{return MTLPatchTypeNone;}
- (NSInteger)patchControlPointCount{return 0;}
@end
@implementation SweetMTLComputePipeline
- (instancetype)initWithFunction:(SweetMTLFunction *)function
{if(!(self=[super init]))return nil;if(![function isKindOfClass:[SweetMTLFunction class]] || function.functionType!=MTLFunctionTypeKernel){[self release];return nil;}
    _function=[function retain];_programLock=[NSLock new];return self;}
- (void)dealloc{for(auto &pair:_programs)sweetgl_metal_program_destroy(pair.second);[_programLock release];[_function release];[super dealloc];}
- (id<MTLDevice>)device{return _function.device;}
- (NSUInteger)maxTotalThreadsPerThreadgroup{return 1024;}
- (NSUInteger)threadExecutionWidth{return 64;}
- (NSUInteger)staticThreadgroupMemoryLength{return 0;}
- (SweetMetalProgram *)programForLocal:(MTLSize)local error:(NSError **)error
{
    std::array<unsigned,3> key={(unsigned)local.width,(unsigned)local.height,(unsigned)local.depth};
    if(local.width!=key[0] || local.height!=key[1] || local.depth!=key[2])return nullptr;
    [_programLock lock];auto found=_programs.find(key);if(found!=_programs.end()){auto result=found->second;[_programLock unlock];return result;}
    if(_programs.size()>=16){if(error)*error=metal_queue_error(@"Compute local-size variant limit reached");[_programLock unlock];return nullptr;}
    std::vector<GPU::Compiler::Entry> entries;std::string diagnostic,essl;
    NSData *source=_function->_library->_entries[_function->_name];
    bool valid=source && GPU::Compiler::lowerAIR(source.bytes,source.length,entries,diagnostic,_function->_name.UTF8String,_function->_constants) && entries.size()==1;
    if(valid)valid=entries[0].reflection.stage==0 && SweetAIR::computeESSL(entries[0].glsl,key,essl,diagnostic);
    char message[2048]={0};SweetMetalProgram *program=valid?sweetgl_metal_compute_create(essl.c_str(),entries[0].reflection.bufferMask,entries[0].reflection.writeMask,message,sizeof(message)):nullptr;
    if(!program){if(diagnostic.empty())diagnostic=message;if(error)*error=metal_compile_error(diagnostic);}
    else _programs[key]=program;[_programLock unlock];return program;
}
@end
@implementation SweetMTLComputeEncoder
- (instancetype)initWithCommands:(SweetMTLCommandBuffer *)commands
{if(!(self=[super init]))return nil;_commands=[commands retain];_bindings=[NSMutableDictionary new];return self;}
- (void)dealloc{[_commands release];[_pipeline release];[_bindings release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _commands.device;}
- (NSString *)label{return _label;}
- (void)setLabel:(NSString *)label{NSString *next=[label copy];[_label release];_label=next;}
- (void)endEncoding
{
    if(_ended)[NSException raise:NSInternalInconsistencyException format:@"Metal compute encoder already ended"];
    _ended=YES;[_bindings removeAllObjects];[_pipeline release];_pipeline=nil;[_commands finishEncoder];
}
- (void)setComputePipelineState:(id<MTLComputePipelineState>)pipeline
{if(_ended || ![pipeline isKindOfClass:[SweetMTLComputePipeline class]] || pipeline.device!=self.device)[NSException raise:NSInvalidArgumentException format:@"Invalid Metal compute pipeline owner"];
    [pipeline retain];[_pipeline release];_pipeline=(SweetMTLComputePipeline *)pipeline;}
- (void)setBuffer:(id<MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
    if(_ended || index>=8 || (buffer && (![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=self.device || offset>=buffer.length || (offset&3))))
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal compute buffer range/owner"];
    @autoreleasepool{if(buffer)_bindings[@(index)]=@{@"buffer":buffer,@"offset":@(offset)};else [_bindings removeObjectForKey:@(index)];}
}
- (void)dispatchThreadgroups:(MTLSize)groups threadsPerThreadgroup:(MTLSize)local
{
    if(_ended || !_pipeline || !groups.width || !groups.height || !groups.depth || !local.width || !local.height || !local.depth ||
        groups.width>65535 || groups.height>65535 || groups.depth>65535 || local.width>1024 || local.height>1024 || local.depth>1024 ||
        local.width*local.height*local.depth>1024 || groups.width*groups.height*groups.depth>16777216/(local.width*local.height*local.depth))
        [NSException raise:NSInvalidArgumentException format:@"Invalid Metal bounded dispatch dimensions"];
    @autoreleasepool{NSArray *g=@[@(groups.width),@(groups.height),@(groups.depth)],*l=@[@(local.width),@(local.height),@(local.depth)];
        if(![_commands appendOperation:@{@"computePipeline":_pipeline,@"bindings":[NSDictionary dictionaryWithDictionary:_bindings],@"groups":g,@"local":l}])
            [NSException raise:NSInvalidArgumentException format:@"Compute command buffer not writable"];
    }
}
@end

static id<MTLComputeCommandEncoder> create_compute_encoder(SweetMTLCommandBuffer *commands)
{return [[[SweetMTLComputeEncoder alloc] initWithCommands:commands] autorelease];}

static NSError *execute_metal_compute(NSDictionary *operation,id<MTLDevice> device,uint32_t *sequence)
{
    auto pipeline=(SweetMTLComputePipeline *)operation[@"computePipeline"];if(![pipeline isKindOfClass:[SweetMTLComputePipeline class]] || pipeline.device!=device)return metal_queue_error(@"Compute pipeline has another owner");
    NSArray *groups=operation[@"groups"],*local=operation[@"local"];if(groups.count!=3 || local.count!=3)return metal_queue_error(@"Compute dimensions missing");
    uint32_t g[3],l[3],grid[3];for(unsigned i=0;i<3;++i){g[i]=[groups[i] unsignedIntValue];l[i]=[local[i] unsignedIntValue];grid[i]=g[i]*l[i];}
    NSDictionary *bindings=operation[@"bindings"];SweetShaderBufferBinding views[8]={};unsigned count=0;
    for(NSNumber *key in bindings){if(count>=8)return metal_queue_error(@"Compute binding limit");NSDictionary *value=bindings[key];
        auto buffer=(SweetMTLBuffer *)value[@"buffer"];uint64_t offset=[value[@"offset"] unsignedLongLongValue];
        if(![buffer isKindOfClass:[SweetMTLBuffer class]] || buffer.device!=device || offset>=buffer.length)return metal_queue_error(@"Compute buffer owner/range invalid");
        views[count++]={buffer->_backing.handle,key.unsignedIntValue,offset,buffer.length-offset};}
    NSError *error=nil;auto program=[pipeline programForLocal:MTLSizeMake(l[0],l[1],l[2]) error:&error];if(!program)return error?:metal_queue_error(@"AIR pipeline failed");
    int result=sweetgl_metal_compute_dispatch(program,views,count,g,l,grid,sequence);
    return result?metal_queue_error([NSString stringWithFormat:@"AIR compute dispatch failed result=%d",result]):nil;
}
