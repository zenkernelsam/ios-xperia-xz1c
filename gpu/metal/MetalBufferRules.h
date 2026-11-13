// SPDX-License-Identifier: BSD-2-Clause
// Resource option bounds adapted from ios_sagit QcomMTL.mm resourceOptions().+#pragma once
#include <stdint.h>
#include <stdbool.h>
static inline bool sweet_metal_shared_options(uint64_t options)
{
    // CPU cache mode0/1; storage modeShared0; hazard tracking0/1/2.
    return !(options&~UINT64_C(0x30f)) && (options&15)<=1 && ((options>>8)&3)<=2;
}
