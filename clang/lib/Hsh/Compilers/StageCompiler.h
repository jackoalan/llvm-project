//===--- StageCompiler.h - Base hsh stage compiler ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/Hsh/HshGenerator.h"

namespace clang::hshgen {
class HshBuiltins;

struct StageBinaries
    : std::array<std::pair<std::vector<uint8_t>, uint64_t>, HshMaxStage> {
  void updateHashes();
};

class StageCompiler {
protected:
  HshTarget Target;
  virtual StageBinaries doCompile(ArrayRef<std::string> Sources) const = 0;

public:
  explicit StageCompiler(HshTarget Target) : Target(Target) {}
  virtual ~StageCompiler() = default;
  StageBinaries compile(ArrayRef<std::string> Sources) const;
};

class TextStageCompiler : public StageCompiler {
protected:
  StageBinaries doCompile(ArrayRef<std::string> Sources) const override;

public:
  using StageCompiler::StageCompiler;
};

std::unique_ptr<StageCompiler> MakeCompiler(HshTarget Target, bool DebugInfo,
                                            StringRef ResourceDir,
                                            CompilerInstance &CI,
                                            DiagnosticsEngine &Diags,
                                            HshBuiltins &Builtins);

} // namespace clang::hshgen
