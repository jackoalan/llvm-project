//===--- HshGenerator.h - Lambda scanner and codegen for hsh tool ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/Frontend/FrontendAction.h"

namespace clang::hshgen {

/* Keep in sync with targets.def in libhsh!! */
enum HshTarget : int {
  HT_NULL,
  HT_GLSL,
  HT_HLSL,
  HT_DXBC,
  HT_DXIL,
  HT_VULKAN_SPIRV,
  HT_METAL,
  HT_METAL_BIN_MAC,
  HT_METAL_BIN_IOS,
  HT_METAL_BIN_TVOS,
  HT_MAX
};

constexpr StringRef HshTargetToString(HshTarget Target) {
  switch (Target) {
  default:
  case HT_NULL:
    return llvm::StringLiteral("null");
  case HT_GLSL:
    return llvm::StringLiteral("glsl");
  case HT_HLSL:
    return llvm::StringLiteral("hlsl");
  case HT_DXBC:
    return llvm::StringLiteral("dxbc");
  case HT_DXIL:
    return llvm::StringLiteral("dxil");
  case HT_VULKAN_SPIRV:
    return llvm::StringLiteral("vulkan-spirv");
  case HT_METAL:
    return llvm::StringLiteral("metal");
  case HT_METAL_BIN_MAC:
    return llvm::StringLiteral("metal-bin-mac");
  case HT_METAL_BIN_IOS:
    return llvm::StringLiteral("metal-bin-ios");
  case HT_METAL_BIN_TVOS:
    return llvm::StringLiteral("metal-bin-tvos");
  }
}

class GenerateAction : public ASTFrontendAction {
  OwningArrayRef<HshTarget> Targets;
  bool DebugInfo, SourceDump;

public:
  explicit GenerateAction(ArrayRef<HshTarget> Targets, bool DebugInfo = false,
                          bool SourceDump = false)
      : Targets(Targets), DebugInfo(DebugInfo), SourceDump(SourceDump) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;
};

} // namespace clang::hshgen
