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

class MetalCompilerRunner {
#ifdef __APPLE__
  llvm::ErrorOr<std::string> XCRunPath;
  StringRef TargetSDK;
#elif defined(_WIN32)
  llvm::ErrorOr<std::string> MetalPath;
#else
  llvm::ErrorOr<std::string> WinePath;
  llvm::ErrorOr<std::string> MetalPath;
#endif

public:
  MetalCompilerRunner(HshTarget Target, DiagnosticsEngine &Diags);
  template <typename... Args> int Run(Args... A) const;
};

class MetalStageCompiler : public StageCompiler {
  CompilerInstance &CI;
  DiagnosticsEngine &Diags;
  MetalCompilerRunner Runner;
  bool DebugInfo;

public:
  MetalStageCompiler(HshTarget Target, bool DebugInfo, CompilerInstance &CI,
                     DiagnosticsEngine &Diags);

  StageBinaries doCompile(ArrayRef<std::string> Sources) const override;
};

} // namespace clang::hshgen
