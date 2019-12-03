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

enum HshTarget {
  HT_GLSL,
  HT_HLSL,
  HT_HLSL_BIN,
  HT_METAL,
  HT_METAL_BIN_MAC,
  HT_METAL_BIN_IOS,
  HT_METAL_BIN_TVOS,
  HT_SPIRV,
  HT_DXIL
};

class GenerateAction : public ASTFrontendAction {
  OwningArrayRef<HshTarget> Targets;

public:
  explicit GenerateAction(ArrayRef<HshTarget> Targets) : Targets(Targets) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;
};

} // namespace clang::hshgen
