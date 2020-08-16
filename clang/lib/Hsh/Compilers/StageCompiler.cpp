//===--- StageCompiler.cpp - Base hsh stage compiler ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "StageCompiler.h"
#include "DxcStageCompiler.h"
#include "MetalStageCompiler.h"
#include "UamStageCompiler.h"

#include "llvm/Support/xxhash.h"

namespace clang::hshgen {

void StageBinaries::updateHashes() {
  for (auto &Binary : *this)
    if (!Binary.first.empty())
      Binary.second = llvm::xxHash64(Binary.first);
}

StageBinaries StageCompiler::compile(ArrayRef<std::string> Sources) const {
  auto Binaries = doCompile(Sources);
  Binaries.updateHashes();
  return Binaries;
}

StageBinaries
TextStageCompiler::doCompile(ArrayRef<std::string> Sources) const {
  StageBinaries Binaries;
  auto OutIt = Binaries.begin();
  for (const auto &Stage : Sources) {
    auto &Out = OutIt++->first;
    if (Stage.empty())
      continue;
    Out.resize(Stage.size() + 1);
    std::memcpy(&Out[0], Stage.data(), Stage.size());
  }
  return Binaries;
}

std::unique_ptr<StageCompiler> MakeCompiler(HshTarget Target, bool DebugInfo,
                                            StringRef ResourceDir,
                                            CompilerInstance &CI,
                                            DiagnosticsEngine &Diags,
                                            HshBuiltins &Builtins) {
  switch (Target) {
  default:
  case HT_GLSL:
  case HT_HLSL:
    return std::make_unique<TextStageCompiler>(Target);
  case HT_DXBC:
  case HT_DXIL:
  case HT_VULKAN_SPIRV:
    return DxcStageCompiler::Create(Target, DebugInfo, ResourceDir, Diags,
                                    Builtins);
  case HT_METAL:
    return std::make_unique<TextStageCompiler>(Target);
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
    return std::make_unique<MetalStageCompiler>(Target, DebugInfo, CI, Diags);
  case HT_DEKO3D:
    return std::make_unique<UamStageCompiler>(Target, Diags);
  }
}

} // namespace clang::hshgen
