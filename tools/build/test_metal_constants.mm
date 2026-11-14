/* Actual original MTLFunctionConstantValues, snapshot/reflection and AIR lowering. */
#define SWEET_METAL_HOST_TEST 1
#import "../../gpu/metal/MetalResources.mm"
#include <assert.h>
#include <map>
#include <string>
static std::map<uint32_t,SweetMetalBacking> backing;static uint32_t nextHandle=1,dispatched;
struct SweetMetalProgram {std::string source;};
extern "C" bool sweetgl_init(void){return true;}
extern "C" int sweetgl_metal_buffer_allocate(uint64_t bytes,SweetMetalBacking *result){void *memory=nullptr;auto span=sweet_metal_backing_span(bytes);if(!span || posix_memalign(&memory,16384,span))return -ENOMEM;
    *result={bytes,span,UINT64_C(0x40000000)+uint64_t(nextHandle)*65536,memory,nextHandle++,0};backing[result->handle]=*result;return 0;}
extern "C" int sweetgl_metal_buffer_release(uint32_t handle){assert(backing.count(handle));free(backing[handle].contents);backing.erase(handle);return 0;}
extern "C" int sweetgl_metal_buffer_synchronize(uint32_t handle){return backing.count(handle)?0:-EINVAL;}
extern "C" int sweetgl_metal_surface_import(uint32_t,uint64_t,uint64_t,SweetMetalBacking *){return -ENOTSUP;}
extern "C" int sweetgl_metal_buffer_blit(uint32_t,uint32_t,uint64_t,uint64_t,uint64_t,uint32_t,uint32_t *){return -EINVAL;}
extern "C" SweetMetalProgram *sweetgl_metal_compute_create(const char *source,uint32_t buffers,uint32_t writes,char *error,size_t bytes)
{if(error && bytes)error[0]=0;assert(buffers==1 && writes==1 && strstr(source,"+13u") && !strstr(source,"+99u"));return new SweetMetalProgram{source};}
extern "C" int sweetgl_metal_compute_dispatch(SweetMetalProgram *program,const SweetShaderBufferBinding *bindings,uint32_t count,
    const uint32_t groups[3],const uint32_t local[3],const uint32_t grid[3],uint32_t *sequence)
{assert(program && bindings && count==1 && groups[0]==1 && local[0]==64 && grid[0]==64 && backing.count(bindings[0].handle));*sequence=++dispatched;return 0;}
extern "C" void sweetgl_metal_program_destroy(SweetMetalProgram *program){delete program;}
extern "C" SweetMetalProgram *sweetgl_metal_render_create(const char *,const char *,const SweetRenderPipelineConfig *,char *,size_t){return nullptr;}
extern "C" int sweetgl_metal_render_draw(SweetMetalProgram *,const SweetRenderPass *,const SweetRenderDraw *,const SweetShaderBufferBinding *,uint32_t,const SweetShaderTextureBinding *,uint32_t,const SweetShaderSamplerBinding *,uint32_t,uint32_t *){return -EINVAL;}
extern "C" int sweetgl_metal_render_clear(const SweetRenderPass *,uint32_t *){return -EINVAL;}
extern "C" int sweetgl_metal_texture_mipmaps(const SweetShaderTextureBinding *,uint32_t *){return -EINVAL;}
int main(int argc,char **argv){assert(argc==2);@autoreleasepool{
    assert(native_constant_abi());NSError *error=nil;id<MTLDevice> device=SweetMTLCreateDevice(&error);assert(device && !error);
    assert(!SweetMTLCreateRegisteredDevice(&error) && error);error=nil;
    assert([device supportsVertexAmplificationCount:1] && ![device supportsVertexAmplificationCount:0] && ![device supportsVertexAmplificationCount:2]);
    NSData *bytes=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];assert(bytes);
    id<MTLLibrary> fileLibrary=[device newLibraryWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]] error:&error];assert(fileLibrary && !error);[fileLibrary release];
    assert(![device newLibraryWithURL:[NSURL URLWithString:@"https://invalid.local/input.metallib"] error:&error] && error);error=nil;
    assert(![device newDefaultLibraryWithBundle:[NSBundle mainBundle] error:&error] && error);error=nil;
    dispatch_data_t data=dispatch_data_create(bytes.bytes,bytes.length,NULL,DISPATCH_DATA_DESTRUCTOR_DEFAULT);id<MTLLibrary> library=[device newLibraryWithData:data error:&error];dispatch_release(data);assert(library && !error);
    assert(![library newFunctionWithName:@"add_constant"]);
    MTLFunctionConstantValues *values=[MTLFunctionConstantValues new];assert(![library newFunctionWithName:@"add_constant" constantValues:values error:&error] && error);error=nil;
    uint32_t step=13;[values setConstantValue:&step type:MTLDataTypeUInt withName:@"step"];
    id<MTLFunction> function=[library newFunctionWithName:@"add_constant" constantValues:values error:&error];assert(function && !error);
    MTLFunctionConstant *declaration=function.functionConstantsDictionary[@"step"];assert(declaration && declaration.index==7 && declaration.type==MTLDataTypeUInt && declaration.required);
    auto object=(SweetMTLFunction *)function;assert(object->_constants.size()==1 && object->_constants[0].index==7);uint32_t actual=0;memcpy(&actual,object->_constants[0].bytes.data(),4);assert(actual==13);
    [values reset];step=99;[values setConstantValue:&step type:MTLDataTypeUInt atIndex:7];memcpy(&actual,object->_constants[0].bytes.data(),4);assert(actual==13);
    std::vector<GPU::Compiler::FunctionConstantValue> snapshot;std::string diagnostic;assert(snapshot_constants(values,snapshot,diagnostic) && snapshot.size()==1 && snapshot[0].index==7);
    [values reset];float wrong=13;[values setConstantValue:&wrong type:MTLDataTypeFloat atIndex:7];assert(![library newFunctionWithName:@"add_constant" constantValues:values error:&error] && error);error=nil;
    [values reset];uint32_t array[3]={3,5,7};[values setConstantValues:array type:MTLDataTypeUInt withRange:NSMakeRange(7,3)];assert(snapshot_constants(values,snapshot,diagnostic) && snapshot.size()==3);[values release];
    id<MTLComputePipelineState> pipeline=[device newComputePipelineStateWithFunction:function error:&error];assert(pipeline && !error);
    id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLBuffer> buffer=[device newBufferWithLength:320 options:0];id<MTLCommandBuffer> commands=[[queue commandBuffer] retain];id<MTLComputeCommandEncoder> encoder=[commands computeCommandEncoder];
    [encoder setComputePipelineState:pipeline];[encoder setBuffer:buffer offset:0 atIndex:0];[encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];[encoder endEncoding];
    [pipeline release];[function release];[library release];[buffer release];[commands commit];[commands waitUntilCompleted];assert(commands.status==MTLCommandBufferStatusCompleted && !commands.error && dispatched==1 && device.currentAllocatedSize==0);
    [commands release];[queue release];[device release];assert(backing.empty());
    puts("actual native Metal constants: name/index/range/reset, required/type rejection, tight snapshot, original reflection, immutable AIR specialization and command lifetime PASS");
}return 0;}
