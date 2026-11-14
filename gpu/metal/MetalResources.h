// SPDX-License-Identifier: BSD-2-Clause
#pragma once
#import <Metal/Metal.h>
#ifdef __cplusplus
extern "C" {
#endif
// Explicit development factory. It does not replace MTLCreateSystemDefaultDevice
// or advertise a MetalPluginName before the command/shader/render implementation.
__attribute__((visibility("default"))) id<MTLDevice> SweetMTLCreateDevice(NSError **error) NS_RETURNS_RETAINED;
/* Read the completed custom queue's actual fence/operation metadata. */
__attribute__((visibility("default"))) BOOL SweetMTLCommandResult(id<MTLCommandBuffer> commands,
    uint32_t *sequence,uint32_t *operations);
/* Diagnostic registration through original Metal.framework entry points.
 * No registry properties or framework functions are replaced. */
__attribute__((visibility("default"))) id<MTLDevice> SweetMTLCreateRegisteredDevice(NSError **error) NS_RETURNS_RETAINED;
#ifdef __cplusplus
}
#endif
