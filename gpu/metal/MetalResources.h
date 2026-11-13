// SPDX-License-Identifier: BSD-2-Clause
#pragma once
#import <Metal/Metal.h>
#ifdef __cplusplus
extern "C" {
#endif
// Explicit development factory. It does not replace MTLCreateSystemDefaultDevice
// or advertise a MetalPluginName before the command/shader/render implementation.
__attribute__((visibility("default"))) id<MTLDevice> SweetMTLCreateDevice(NSError **error) NS_RETURNS_RETAINED;
#ifdef __cplusplus
}
#endif
