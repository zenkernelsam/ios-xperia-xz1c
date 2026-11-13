// SPDX-License-Identifier: BSD-2-Clause
// Frontend-only interface: no ios_sagit private JIT or kernel command ABI.
#pragma once
#include "reference/GPUJITProtocol.h"
#include "reference/FunctionConstants.h"
#include <string>
#include <vector>
namespace GPU { namespace Compiler {
struct Entry {std::string name,glsl;QGPUProgramDescription reflection{};};
bool inspectAIREntries(const void *,size_t,std::vector<std::string> &,std::string &);
bool inspectAIRConstants(const void *,size_t,const std::string &,std::vector<FunctionConstant> &,unsigned &,std::string &);
bool lowerAIR(const void *,size_t,std::vector<Entry> &,std::string &,const std::string &selected={},
    const std::vector<FunctionConstantValue> &constants={});
} }
