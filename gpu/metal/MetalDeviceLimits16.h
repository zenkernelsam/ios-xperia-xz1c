// SPDX-License-Identifier: BSD-2-Clause
// Adapted from ios_sagit drivers/gpu/QcomMTL/NativeMetal.h (898998cf).
// Layout verified against the retained iOS16.7.16/20H392 Metal _limits encoding.+#pragma once
#include <stddef.h>
#include <stdint.h>

struct SweetMTLDeviceLimits16 {
    uint32_t maxFramebufferStorageBits;
    uint32_t linearTextureArrayAlignmentBytes, linearTextureArrayAlignmentSlice;
    uint32_t maxTileBuffers, maxTileTextures, maxTileSamplers, maxTileInlineDataSize, minTilePixels;
    uint32_t maxColorAttachments, maxVertexAttributes, maxVertexBuffers, maxVertexTextures;
    uint32_t maxVertexSamplers, maxVertexInlineDataSize, maxInterpolants;
    uint32_t maxFragmentBuffers, maxFragmentTextures, maxFragmentSamplers, maxFragmentInlineDataSize;
    uint32_t maxComputeBuffers, maxComputeTextures, maxComputeSamplers, maxComputeInlineDataSize;
    uint32_t maxComputeLocalMemorySizes, maxTotalComputeThreadsPerThreadgroup, maxComputeThreadgroupMemory;
    float maxLineWidth, maxPointSize;
    uint32_t maxVisibilityQueryOffset, padmaxBufferLength;
    uint32_t minConstantBufferAlignmentBytes, minBufferNoCopyAlignmentBytes;
    uint32_t maxTextureWidth1D, maxTextureWidth2D, maxTextureHeight2D;
    uint32_t maxTextureWidth3D, maxTextureHeight3D, maxTextureDepth3D, maxTextureDimensionCube, maxTextureLayers;
    uint32_t linearTextureAlignmentBytes, iosurfaceTextureAlignmentBytes, iosurfaceReadOnlyTextureAlignmentBytes;
    uint32_t deviceLinearTextureAlignmentBytes, deviceLinearReadOnlyTextureAlignmentBytes;
    uint32_t maxFunctionConstantIndices, maxComputeThreadgroupMemoryAlignmentBytes, maxInterpolatedComponents;
    uint32_t maxTessellationFactor, maxIndirectBuffers, maxIndirectTextures, maxIndirectSamplers;
    uint32_t maxIndirectSamplersPerDevice, maxFenceInstances, maxViewportCount, maxCustomSamplePositions;
    uint32_t maxVertexAmplificationFactor, maxVertexAmplificationCount, maxTextureBufferWidth;
    uint32_t maxComputeAttributes, maxIOCommandsInFlight, maxPredicatedNestingDepth;
    // iOS16 has no maxAccelerationStructureLevels field here. Byte252 is
    // alignment padding; it must not become maxConstantBufferArguments.
    uint32_t maxConstantBufferArguments;
    uint64_t maxBufferLength;
};
static_assert(sizeof(SweetMTLDeviceLimits16)==264 && alignof(SweetMTLDeviceLimits16)==8,"iOS16 limits ABI");
static_assert(offsetof(SweetMTLDeviceLimits16,maxConstantBufferArguments)==248,"iOS16 constant arguments ABI");
static_assert(offsetof(SweetMTLDeviceLimits16,maxBufferLength)==256,"iOS16 maximum buffer length ABI");
