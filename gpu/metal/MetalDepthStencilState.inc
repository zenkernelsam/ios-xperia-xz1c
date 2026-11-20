// SPDX-License-Identifier: BSD-2-Clause
// Native descriptor semantics follow ios_sagit/QcomMTL (898998cf); Metal16 ABI.
#include "../sweetgl/MetalDepthAttachment.h"
#include "../sweetgl/MetalStencilAttachment.h"
@interface SweetMTLDepthStencilState : NSObject <MTLDepthStencilState> {
@public SweetMTLDevice *_owner;SweetDepthStencilState _state;NSString *_label;
}
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLDepthStencilDescriptor *)descriptor;
@end
static bool depth_stencil_face(MTLStencilDescriptor *descriptor,SweetStencilFace *face)
{
    if(!descriptor){*face={0};return true;}
    if(descriptor.stencilCompareFunction>MTLCompareFunctionAlways || descriptor.stencilFailureOperation>MTLStencilOperationDecrementWrap ||
        descriptor.depthFailureOperation>MTLStencilOperationDecrementWrap || descriptor.depthStencilPassOperation>MTLStencilOperationDecrementWrap)return false;
    *face={1,(uint32_t)descriptor.stencilCompareFunction,(uint32_t)descriptor.stencilFailureOperation,(uint32_t)descriptor.depthFailureOperation,
        (uint32_t)descriptor.depthStencilPassOperation,descriptor.readMask&255u,descriptor.writeMask&255u};return true;
}
@implementation SweetMTLDepthStencilState
- (instancetype)initWithOwner:(SweetMTLDevice *)owner descriptor:(MTLDepthStencilDescriptor *)descriptor
{
    if(!(self=[super init]))return nil;
    _state=sweet_depth_stencil_default();
    if(!owner || !descriptor || descriptor.depthCompareFunction>MTLCompareFunctionAlways ||
        !depth_stencil_face(descriptor.frontFaceStencil,&_state.front) || !depth_stencil_face(descriptor.backFaceStencil,&_state.back)){
        [self release];return nil;}
    _state.compare=(uint32_t)descriptor.depthCompareFunction;_state.write=descriptor.depthWriteEnabled;
    _owner=[owner retain];_label=[descriptor.label copy];return self;
}
- (void)dealloc{[_owner release];[_label release];[super dealloc];}
- (id<MTLDevice>)device{return _owner;}
- (NSString *)label{return _label;}
@end
@implementation SweetMTLDevice (DepthStencilStates)
- (id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor *)descriptor
{return [[SweetMTLDepthStencilState alloc] initWithOwner:self descriptor:descriptor];}
@end
#define SWEET_COMPARE_ABI(name,value) static_assert(uint32_t(MTLCompareFunction##name)==value,"Metal compare ABI differs")
SWEET_COMPARE_ABI(Never,0);SWEET_COMPARE_ABI(Less,1);SWEET_COMPARE_ABI(Equal,2);SWEET_COMPARE_ABI(LessEqual,3);
SWEET_COMPARE_ABI(Greater,4);SWEET_COMPARE_ABI(NotEqual,5);SWEET_COMPARE_ABI(GreaterEqual,6);SWEET_COMPARE_ABI(Always,7);
#undef SWEET_COMPARE_ABI
static_assert(uint32_t(MTLPixelFormatDepth32Float)==252,"Metal depth32 format ABI differs");
static_assert(uint32_t(MTLPixelFormatStencil8)==253,"Metal stencil8 format ABI differs");
