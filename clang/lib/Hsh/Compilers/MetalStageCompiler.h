//===--- MetalStageCompiler.h - Metal hsh stage compiler ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "StageCompiler.h"

namespace clang::hshgen {

class MetalStageCompiler : public StageCompiler {
  CompilerInstance &CI;
  DiagnosticsEngine &Diags;
  llvm::ErrorOr<std::string> XCRunPath;
  StringRef TargetSDK;
  bool DebugInfo;

public:
  MetalStageCompiler(HshTarget Target, bool DebugInfo, CompilerInstance &CI,
                     DiagnosticsEngine &Diags);

  StageBinaries doCompile(ArrayRef<std::string> Sources) const override;
};

} // namespace clang::hshgen
